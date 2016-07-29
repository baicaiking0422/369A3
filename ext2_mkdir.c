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

    if (argc != 3) {
        fprintf(stderr, "Usage: ext2_mkdir <image file name> <abs path>\n");
        exit(1);
    }

    int fd = open(argv[1], O_RDWR);
    // int path_len;
    char *path;

    path = malloc(strlen(argv[2]));
    strcpy(path, argv[2]);

    if (path[0] != '/') {
        fprintf(stderr, "This is not an absolute path!");
        exit(1);
    }

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc * gd = (struct ext2_group_desc *)(disk + 2048);
    void *inodes = disk + 1024 * gd->bg_inode_table;
    int inode_num = get_inode_num(path, inodes, disk);

    if (inode_num != -1) {
        perror("The directory already exists.");
        exit(EEXIST);
    }

    char *parent_path;
    char *file_name;
    get_file_parent_path(path, &parent_path);
    printf("%s\n", parent_path);
    inode_num = get_inode_num(parent_path, inodes, disk);

    if (inode_num == -1) {
        perror("The directory does not exist.");
        exit(ENOENT);
    }

    int count, i;
    char *name;
    struct ext2_inode *inode;
    for (i = 0; i < sb->s_inodes_count; i ++) {
        if (i < EXT2_GOOD_OLD_FIRST_INO && i != EXT2_ROOT_INO - 1) {
            continue;
        }
        inode = (struct ext2_inode *)(disk + 1024 * gd->bg_inode_table + 128 * i);
        if (inode->i_mode & EXT2_S_IFREG) {
            continue;
        }

        if (inode->i_size == 0) {
            printf("%d\n", i);
            break;
        }

    }

    // struct ext2_inode *inode = (struct ext2_inode *) (inodes + sizeof(struct ext2_inode) * (inode_num - 1));

    int inode_block_num;
    // int count;
    // char *name;
    // int check;
    struct ext2_dir_entry_2 *entry;
    inode_block_num = 0;

    for (inode_block_num = 0; inode_block_num < 12; inode_block_num ++) {
        // count = 0;
        // check = 0;
        // if (inode->i_block[inode_block_num] != 0) {
        //     entry = (struct ext2_dir_entry_2*) (disk + 1024 * inode -> i_block[inode_block_num] + count);
        //     count += entry->rec_len;
        // }
        // else
        if (inode->i_block[inode_block_num] == 0) {
            entry = (struct ext2_dir_entry_2 *)(disk + 1024 * inode->i_block[0] + count); // 0 instead of count;
            entry->inode = inode_block_num;
            entry->rec_len = 1024;
            entry->name_len = strlen(file_name);
            entry->file_type |= EXT2_FT_DIR;
            get_file_name(path, &file_name);
            strcpy(file_name, entry->name);
            break;
        }
    }
}

// entry_rec_len = ((7 + entry->name_len) / 4 + 1) * 4;
