#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdint.h>

#define BLOCK_SIZE 4096
#define MAX_FILES 64

// Structure matching the design of HashFS
struct hashfs_super_block {
    char magic[8];
    uint32_t num_files;
    uint32_t free_blocks;
};

int is_raw_device(const char *device) {
    // If the device path ends in a digit (e.g., sda1, nvme0n1p1), it's a partition
    int len = strlen(device);
    if (len == 0) return 0;
    return !isdigit((unsigned char)device[len - 1]);
}

int check_partition_exists(const char *device) {
    char part_path[256];
    // Check for standard partition naming conventions (e.g., /dev/sda1)
    snprintf(part_path, sizeof(part_path), "%s1", device);
    if (access(part_path, F_OK) == 0) {
        return 1;
    }
    // Check for NVMe style partition naming conventions (e.g., /dev/nvme0n1p1)
    snprintf(part_path, sizeof(part_path), "%sp1", device);
    if (access(part_path, F_OK) == 0) {
        return 1;
    }
    return 0;
}

int create_primary_partition(const char *device) {
    char command[512];
    printf("[HashFS] No partitions detected on %s. Creating a new primary partition using 100%% capacity...\n", device);

    // Create a standard GPT partition table and allocate 100% space to partition 1
    snprintf(command, sizeof(command), "sudo parted -s %s mklabel gpt mkpart primary 2048s 100%%", device);
    if (system(command) != 0) {
        fprintf(stderr, "Error: Failed to partition device using parted.\n");
        return -1;
    }

    // Force the Linux kernel to reload the new partition table rules
    printf("[HashFS] Probing partition table changes across the kernel...\n");
    snprintf(command, sizeof(command), "sudo partprobe %s", device);
    system(command);
    
    // Brief sleep cycle to allow udev rules to settle partition node paths
    sleep(1); 
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <target_device_node>\n", argv[0]);
        return 1;
    }

    char target_device[256];
    strncpy(target_device, argv[1], sizeof(target_device) - 1);

    // Step 1: Detect if user passed a raw device node (like /dev/sda)
    if (is_raw_device(target_device)) {
        // Step 2: Check if partition 1 already exists
        if (!check_partition_exists(target_device)) {
            // Step 3: Automatically partition if completely empty
            if (create_primary_partition(target_device) == 0) {
                // Adjust target destination to point to the newly configured partition layout
                if (strstr(target_device, "nvme") || strstr(target_device, "mmcblk")) {
                    snprintf(target_device, sizeof(target_device), "%sp1", argv[1]);
                } else {
                    snprintf(target_device, sizeof(target_device), "%s1", argv[1]);
                }
                printf("[HashFS] Target updated to automated partition target: %s\n", target_device);
            } else {
                fprintf(stderr, "Aborting execution format routine.\n");
                return 1;
            }
        } else {
            printf("[HashFS] Partitions already exist on raw node. Please target the partition directly (e.g., %s1).\n", target_device);
            return 1;
        }
    }

    // Step 4: Proceed with the standard filesystem structure formatting loop
    printf("[HashFS] Writing structural metadata layouts to target partition node: %s\n", target_device);
    FILE *dev = fopen(target_device, "r+b");
    if (!dev) {
        perror("Error opening target block destination node");
        return 1;
    }

    struct hashfs_super_block sb;
    memcpy(sb.magic, "HASHFS__", 8);
    sb.num_files = 0;
    sb.free_blocks = 1024; // Static placeholder allocation tracking

    // Write superblock blocks down to physical sector zero boundary bounds
    fseek(dev, 0, SEEK_SET);
    if (fwrite(&sb, sizeof(struct hashfs_super_block), 1, dev) != 1) {
        perror("Failed to write system super block data layout");
        fclose(dev);
        return 1;
    }

    printf("[HashFS] System formatted successfully!\n");
    fclose(dev);
    return 0;
}
