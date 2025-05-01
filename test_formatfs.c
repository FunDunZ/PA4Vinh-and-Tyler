#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"
#include "filesystem.c"
#include "softwaredisk.h"
#include "softwaredisk.c"

int main() {
    // Run formatfs to initialize the filesystem
    printf("Formatting filesystem...\n");
    system("./formatfs");
    
    // Read the superblock to verify it was initialized correctly
    Superblock sb;
    if (!read_superblock(&sb)) {
        printf("Failed to read superblock.\n");
        return 1;
    }
    
    // Print superblock information
    printf("Superblock information:\n");
    printf("  Total blocks: %lu\n", sb.total_blocks);
    printf("  Free blocks: %lu\n", sb.free_blocks);
    printf("  Total inodes: %lu\n", sb.total_inodes);
    printf("  Free inodes: %lu\n", sb.free_inodes);
    printf("  Block size: %lu\n", sb.block_size);
    printf("  Inode size: %lu\n", sb.inode_size);
    printf("  Block bitmap start: %lu\n", sb.block_bitmap_start);
    printf("  Inode bitmap start: %lu\n", sb.inode_bitmap_start);
    printf("  Inode table start: %lu\n", sb.inode_table_start);
    printf("  Data blocks start: %lu\n", sb.data_blocks_start);
    
    // Read the data bitmap to verify it was initialized correctly
    unsigned char data_bitmap[SOFTWARE_DISK_BLOCK_SIZE];
    if (!read_sd_block(data_bitmap, DATA_BITMAP_BLOCK)) {
        printf("Failed to read data bitmap.\n");
        return 1;
    }
    
    // Check if the first few bytes of the bitmap are all zeros
    int all_zeros = 1;
    for (int i = 0; i < 10; i++) {
        if (data_bitmap[i] != 0) {
            all_zeros = 0;
            break;
        }
    }
    
    if (all_zeros) {
        printf("Data bitmap initialized correctly (all zeros).\n");
    } else {
        printf("Data bitmap not initialized correctly.\n");
    }
    
    // Read the inode bitmap to verify it was initialized correctly
    unsigned char inode_bitmap[SOFTWARE_DISK_BLOCK_SIZE];
    if (!read_sd_block(inode_bitmap, INODE_BITMAP_BLOCK)) {
        printf("Failed to read inode bitmap.\n");
        return 1;
    }
    
    // Check if the first few bytes of the bitmap are all zeros
    all_zeros = 1;
    for (int i = 0; i < 10; i++) {
        if (inode_bitmap[i] != 0) {
            all_zeros = 0;
            break;
        }
    }
    
    if (all_zeros) {
        printf("Inode bitmap initialized correctly (all zeros).\n");
    } else {
        printf("Inode bitmap not initialized correctly.\n");
    }
    
    printf("Test completed.\n");
    return 0;
} 