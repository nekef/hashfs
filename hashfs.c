#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "hashfs.h"

static int disk_fd = -1;
static HashSuperblock sb;
static HashFileEntry file_table[MAX_FILES];

static long get_block_offset(uint32_t block_index) {
    return sizeof(HashSuperblock) + (sizeof(HashFileEntry) * MAX_FILES) + (block_index * BLOCK_SIZE);
}

static void flush_metadata() {
    pwrite(disk_fd, &sb, sizeof(HashSuperblock), 0);
    pwrite(disk_fd, file_table, sizeof(HashFileEntry) * MAX_FILES, sizeof(HashSuperblock));
}

static int hash_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    const char *name = path + 1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used && strcmp(file_table[i].filename, name) == 0) {
            stbuf->st_mode = S_IFREG | 0644;
            stbuf->st_nlink = 1;
            stbuf->st_size = file_table[i].size;
            return 0;
        }
    }
    return -ENOENT;
}

static int hash_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset; (void) fi; (void) flags;
    if (strcmp(path, "/") != 0) return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used) {
            filler(buf, file_table[i].filename, NULL, 0, 0);
        }
    }
    return 0;
}

static int hash_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) mode; (void) fi;
    const char *name = path + 1;

    if (strlen(name) >= MAX_FILENAME) return -ENAMETOOLONG;

    int free_idx = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used && strcmp(file_table[i].filename, name) == 0) return -EEXIST;
        if (!file_table[i].used && free_idx == -1) free_idx = i;
    }

    if (free_idx == -1) return -ENOSPC;

    // Scan data blocks to find an unallocated block index
    int free_block = -1;
    for (int block = 0; block < DATA_BLOCKS; block++) {
        int block_used = 0;
        for (int j = 0; j < MAX_FILES; j++) {
            if (file_table[j].used && file_table[j].start_block == (uint32_t)block) {
                block_used = 1;
                break;
            }
        }
        if (!block_used) {
            free_block = block;
            break;
        }
    }

    if (free_block == -1) return -ENOSPC;

    file_table[free_idx].used = 1;
    strncpy(file_table[free_idx].filename, name, MAX_FILENAME);
    file_table[free_idx].size = 0;
    file_table[free_idx].start_block = free_block;
    file_table[free_idx].expected_hash = FNV_OFFSET_BASIS;

    sb.file_count++;
    flush_metadata();
    return 0;
}

static int hash_open(const char *path, struct fuse_file_info *fi) {
    fi->direct_io = 1; 
    return 0;
}

static int hash_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    const char *name = path + 1;

    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used && strcmp(file_table[i].filename, name) == 0) {
            if (offset >= (off_t)file_table[i].size) return 0;
            if (offset + size > file_table[i].size) size = file_table[i].size - offset;

            char temp_block[BLOCK_SIZE];
            memset(temp_block, 0, BLOCK_SIZE);
            
            ssize_t bytes_read = pread(disk_fd, temp_block, file_table[i].size, get_block_offset(file_table[i].start_block));
            (void)bytes_read;

            uint32_t current_hash = calculate_fnv1a(temp_block, file_table[i].size);
            if (current_hash != file_table[i].expected_hash) {
                fprintf(stderr, "!!! LIVE CORRUPTION DETECTED on %s !!! Expected Hash: %u, Found: %u\n", 
                        file_table[i].filename, file_table[i].expected_hash, current_hash);
                return -EIO;
            }

            memcpy(buf, temp_block + offset, size);
            return size;
        }
    }
    return -ENOENT;
}

static int hash_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    const char *name = path + 1;

    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used && strcmp(file_table[i].filename, name) == 0) {
            if (offset + size > BLOCK_SIZE) return -ENOSPC;

            char temp_block[BLOCK_SIZE];
            memset(temp_block, 0, BLOCK_SIZE);

            if (offset > 0 || file_table[i].size > 0) {
                ssize_t read_bytes = pread(disk_fd, temp_block, BLOCK_SIZE, get_block_offset(file_table[i].start_block));
                (void)read_bytes;
            }

            memcpy(temp_block + offset, buf, size);
            if (offset + size > file_table[i].size) {
                file_table[i].size = offset + size;
            }

            file_table[i].expected_hash = calculate_fnv1a(temp_block, file_table[i].size);
            pwrite(disk_fd, temp_block, BLOCK_SIZE, get_block_offset(file_table[i].start_block));
            
            flush_metadata();
            return size;
        }
    }
    return -ENOENT;
}

// Production Capability: Handles deletion (rm file)
static int hash_unlink(const char *path) {
    const char *name = path + 1;

    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used && strcmp(file_table[i].filename, name) == 0) {
            // 1. Wipe the raw physical block storage cells with zeros
            char zero_block[BLOCK_SIZE];
            memset(zero_block, 0, BLOCK_SIZE);
            pwrite(disk_fd, zero_block, BLOCK_SIZE, get_block_offset(file_table[i].start_block));

            // 2. Erase allocations in the metadata block layout
            memset(&file_table[i], 0, sizeof(HashFileEntry));
            
            sb.file_count--;
            flush_metadata();
            return 0;
        }
    }
    return -ENOENT;
}

static const struct fuse_operations hash_oper = {
    .getattr = hash_getattr,
    .readdir = hash_readdir,
    .create  = hash_create,
    .open    = hash_open, 
    .read    = hash_read,
    .write   = hash_write,
    .unlink  = hash_unlink, // Registered the unlink hook!
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <disk_image> <mountpoint> [FUSE options]\n", argv[0]);
        return 1;
    }

    char *disk_path = argv[1];
    disk_fd = open(disk_path, O_RDWR);
    if (disk_fd < 0) {
        perror("Error opening storage device node");
        return 1;
    }

    pread(disk_fd, &sb, sizeof(HashSuperblock), 0);
    if (sb.magic != HASHFS_MAGIC) {
        fprintf(stderr, "Invalid File System Target! Missing HashFS signature block identification.\n");
        close(disk_fd);
        return 1;
    }
    pread(disk_fd, file_table, sizeof(HashFileEntry) * MAX_FILES, sizeof(HashSuperblock));

    argv[1] = argv[0];
    return fuse_main(argc - 1, argv + 1, &hash_oper, NULL);
}
