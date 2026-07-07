#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashfs.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <disk_device_or_image>\n", argv[0]);
        return 1;
    }

    FILE *disk = fopen(argv[1], "wb+");
    if (!disk) {
        perror("Error opening target device");
        return 1;
    }

    HashSuperblock sb = {
        .magic = HASHFS_MAGIC,
        .total_blocks = DATA_BLOCKS,
        .block_size = BLOCK_SIZE,
        .file_count = 0
    };

    fwrite(&sb, sizeof(HashSuperblock), 1, disk);

    HashFileEntry entries[MAX_FILES];
    memset(entries, 0, sizeof(entries));
    fwrite(entries, sizeof(HashFileEntry), MAX_FILES, disk);

    uint8_t empty_block[BLOCK_SIZE];
    memset(empty_block, 0, BLOCK_SIZE);
    for (int i = 0; i < DATA_BLOCKS; i++) {
        fwrite(empty_block, BLOCK_SIZE, 1, disk);
    }

    printf("HashFS initialized successfully on %s!\n", argv[1]);
    fclose(disk);
    return 0;
}
