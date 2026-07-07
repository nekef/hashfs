#ifndef HASHFS_H
#define HASHFS_H

#include <stdint.h>
#include <stddef.h>

#define HASHFS_MAGIC 0x48415348 // "HASH" in ASCII
#define MAX_FILES 16
#define MAX_FILENAME 64
#define BLOCK_SIZE 512
#define DATA_BLOCKS 64

#define FNV_OFFSET_BASIS 2166136261U
#define FNV_PRIME 16777619U

static inline uint32_t calculate_fnv1a(const char *data, size_t len) {
    uint32_t hash = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint8_t)data[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

typedef struct {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t block_size;
    uint32_t file_count;
} HashSuperblock;

typedef struct {
    char filename[MAX_FILENAME];
    uint32_t size;
    uint32_t start_block;
    uint32_t expected_hash;
    uint8_t used;
} HashFileEntry;

#endif
