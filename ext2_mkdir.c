#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include "ext2.h"
#include "ext2_helper.h"

unsigned char *disk;

int main(int argc, const char * argv[]) {

    /* Check arg */
    if (argc != 3) {
        fprintf(stderr, "Usage: ext2_mkdir <image file name> <abs path>\n");
        exit(1);
    }

    int fd = open(argv[1], O_RDWR);
    char *path;

    path = malloc(strlen(argv[2]));
    strcpy(path, argv[2]);

    if (path[0] != '/') {
        fprintf(stderr, "This is not an absolute path!\n");
        exit(1);
    }

    /* Map to memory */
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    struct ext2_group_desc * gd = (struct ext2_group_desc *)(disk + 2048);
    void *inodes = disk + 1024 * gd->bg_inode_table;
    int inode_num = get_inode_num(path, inodes, disk);

    if (inode_num != -1) {
        perror("The directory already exists.\n");
        exit(EEXIST);
    }

    char *parent_path;
    char *file_name;
    get_file_parent_path(path, &parent_path);
    inode_num = get_inode_num(parent_path, inodes, disk);

    if (inode_num == -1) {
        perror("The directory does not exist.\n");
        exit(ENOENT);
    }

    int i, j, k, l, tmp, set;
    set = 0;
    for (k = 0; k < 16; k ++) {
        tmp = disk[gd->bg_block_bitmap * 1024 + k];
        for (l = 0; l < 8; l ++) {
            if (!(tmp & 1)) {
                // printf("k: %d\tj: %d\n", k, l);
                set_block_bitmap(&disk[gd->bg_block_bitmap * 1024 + k], l, 1);
                gd->bg_free_blocks_count ++;
                set = 1;
                break;
            }
            tmp >>= 1;
        }
        if (set) break;
    }

    set = 0;
    for (i = 0; i < 16; i ++) {
        tmp = disk[gd->bg_inode_bitmap * 1024 + i];
        for (j = 0; j < 8; j ++) {
            if (!(tmp & 1)) {
                set_inode_bitmap(&disk[gd->bg_inode_bitmap * 1024 + i], j, 1);
                gd->bg_free_inodes_count ++;
                set = 1;
                break;
            }
            tmp >>= 1;
        }
        if (set) break;
    }

    struct ext2_inode *inode;
    inode = (struct ext2_inode *)(disk + 1024 * gd->bg_inode_table + 128 * (i * 8 + j));
    inode->i_size = 1;
    inode->i_mode |= EXT2_S_IFDIR;

    int inode_block_num;
    struct ext2_dir_entry_2 *entry;
    inode_block_num = 0;

    // Add .
    inode->i_block[0] = k * 8 + l;
    entry = (struct ext2_dir_entry_2 *)(disk + 1024 * inode->i_block[0]);
    entry->inode = i * 8 + j + 1;
    entry->rec_len = 12;
    entry->name_len = 1;
    entry->file_type |= EXT2_FT_DIR;
    strcpy(entry->name, ".");

    // Add ..
    entry = (struct ext2_dir_entry_2 *)(disk + 1024 * inode->i_block[0] + 12);
    entry->inode = inode_num;
    entry->rec_len = 1024 - 12;
    entry->name_len = 2;
    entry->file_type |= EXT2_FT_DIR;
    strcpy(entry->name, "..");

    // Add Entry in its parent node
    struct ext2_inode *parent_inode;
    parent_inode = (struct ext2_inode *)(disk + 1024 * gd->bg_inode_table + sizeof(struct ext2_inode) * (inode_num - 1));

    int count, timeout;
    count = 0;
    timeout = 0;
    while (count < 1024) {
        entry = (struct ext2_dir_entry_2 *)(disk + 1024 * parent_inode->i_block[0] + count);

        if (count + entry->rec_len >= 1024) {
            // If it is the last one
            entry->rec_len = ((7 + entry->name_len) / 4 + 1) * 4;
            count += entry->rec_len;
            break;
        }
        count += entry->rec_len;
        timeout ++;
        if (timeout > 10) {
            printf("Timeout, break.\n");
            break;
        }
    }

    // Entry for the new directory
    entry = (struct ext2_dir_entry_2 *)(disk + 1024 * parent_inode->i_block[0] + count);
    entry->file_type |= EXT2_FT_DIR;
    entry->rec_len = 1024 - count;
    get_file_name(path, &file_name);
    entry->name_len = strlen(file_name);
    strncpy(entry->name, file_name, entry->name_len);
    entry->inode = i * 8 + j + 1;
}
