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
    char *parent_path;
    char *file_name;
    get_file_parent_path(path, &parent_path);
    get_file_name(path, &file_name);
    int inode_num = get_inode_num(parent_path, inodes, disk);
    // int inode_num = get_inode_num(path, inodes, disk);

    // if (inode_num != -1) {
    //     perror("The directory already exists.\n");
    //     exit(EEXIST);
    // }
    //
    // char *parent_path;
    // char *file_name;
    // get_file_parent_path(path, &parent_path);
    // inode_num = get_inode_num(parent_path, inodes, disk);

    if (inode_num == -1) {
        perror("The file does not exist.\n");
        exit(ENOENT);
    }

    // int inode_num = get_inode_num(path, inodes, disk);

    // int i, j, k, l, tmp, set;
    // set = 0;
    // for (k = 0; k < 16; k ++) {
    //     tmp = disk[gd->bg_block_bitmap * 1024 + k];
    //     for (l = 0; l < 8; l ++) {
    //         if (!(tmp & 1)) {
    //             // printf("k: %d\tj: %d\n", k, l);
    //             set_block_bitmap(&disk[gd->bg_block_bitmap * 1024 + k], l, 1);
    //             set = 1;
    //             break;
    //         }
    //         tmp >>= 1;
    //     }
    //     if (set) break;
    // }
    //
    // set = 0;
    // for (i = 0; i < 16; i ++) {
    //     tmp = disk[gd->bg_inode_bitmap * 1024 + i];
    //     for (j = 0; j < 8; j ++) {
    //         if (!(tmp & 1)) {
    //             set_block_bitmap(&disk[gd->bg_inode_bitmap * 1024 + i], j, 1);
    //             set = 1;
    //             break;
    //         }
    //         tmp >>= 1;
    //     }
    //     if (set) break;
    // }
    //
    struct ext2_inode *inode = (struct ext2_inode *)(disk + 1024 * gd->bg_inode_table + sizeof(struct ext2_inode) * (inode_num - 1));
    // inode->i_size = 1;
    // inode->i_mode |= EXT2_S_IFDIR;
    //
    // int inode_block_num;
    struct ext2_dir_entry_2 *entry;
    // struct ext2_dir_entry_2 *moved_entry;
    struct ext2_dir_entry_2 *next_entry;
    int count, timeout, offset;
    char *name;
    count = 0;
    timeout = 0;
    offset = 0;

    while (count < 1024) {

        entry = (struct ext2_dir_entry_2 *)(disk + 1024 * inode->i_block[0] + count);

        // if first equal ...


        // if last equal ...

        count += entry->rec_len;

        next_entry = (struct ext2_dir_entry_2 *)(disk + 1024 * inode->i_block[0] + count);

        name = malloc(sizeof(char) * next_entry->name_len + 1);
        strncpy(name, next_entry->name, next_entry->name_len);
        name[next_entry->name_len] = '\0';

        printf(
            "Inode: %d rec_len: %d name_len: %d name=%s\n",
            next_entry->inode,
            next_entry->rec_len,
            next_entry->name_len,
            name);

        // If found
        if (strcmp(file_name, name) == 0) {

            // If not a regular file
            if (next_entry->file_type & EXT2_S_IFREG) {
                perror("This is not a regular file.\n");
                exit(ENOENT);
            }

            entry->rec_len += next_entry->rec_len;
            next_entry->rec_len = 0;
            next_entry->name_len = 0;
            next_entry->inode = 0;

        }

        free(name);


        // } else {
        //                 printf("HAHA\n");
        //     moved_entry = (struct ext2_dir_entry_2 *)(disk + 1024 * inode->i_block[0] + count - offset);
        //
        //     moved_entry->file_type = entry->file_type;
        //     moved_entry->rec_len = entry->rec_len;
        //     moved_entry->name_len = entry->name_len;
        //     strcpy(moved_entry->name, entry->name);
        //     moved_entry->inode = entry->inode;
        //
        //     if (count + entry->rec_len >= 1024) {
        //         moved_entry->name_len = 1024 - count;
        //     }
        // }

        timeout ++;
        if (timeout > 10) {
            printf("Timeout, break.\n");
            break;
        }
    }

    // perror("The file does not exist.\n");

}
