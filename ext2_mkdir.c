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

unsigned char *disk;

// Need to add to general
int get_inode_num(char *path, void *inodes) {
    int cur_inode_num = EXT2_ROOT_INO;
    int new_inode_num;
    int check_exist = -1;
    int count;
    char *name;
    char *token;
    int inode_block_num;
    token = strtok(path, "/");
    struct ext2_inode *inode;
    struct ext2_dir_entry_2 *entry;


    if (strcmp(path, "/") == 0) {
        return cur_inode_num;
    }

    new_inode_num = cur_inode_num;
    while (token != NULL) {
        //printf("%s\n", token);
        inode = (struct ext2_inode *)(inodes + (new_inode_num - 1) * sizeof(struct ext2_inode));
        //inode->i_size?
        if (inode->i_size != 0) {
            count = 0;
            inode_block_num = 0;
            int i;
            for (i = 0; i < 12; i++) {
                while(count < 1024) {
                    entry = (struct ext2_dir_entry_2*)(disk+1024 * inode->i_block[inode_block_num] + count);
                    count += entry->rec_len;
                    if (entry->file_type == EXT2_FT_DIR) {
                        name = malloc(sizeof(char) * entry->name_len);
                        strncpy(name, entry->name, entry->name_len);
                        if (strcmp(token, name) == 0) {
                            new_inode_num = entry->inode;
                            check_exist = 0;
                            break;
                        }
                    }
                }
            }
        }
        // check whether this token exists in entries of this inode
        if (check_exist == -1) {
            return -1;
        }
        token = strtok(NULL, "/");
    }
    if (check_exist == 0) {
        return new_inode_num;
    }
    else {
        return -1;
    }
}

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

    struct ext2_group_desc * gd = (struct ext2_group_desc *)(disk + 2048);
    void *inodes = disk + 1024 * gd->bg_inode_table;
    int inode_num = get_inode_num(path, inodes);
    if (inode_num == -1) {
        return ENOENT;
    }
    struct ext2_inode *inode = (struct ext2_inode *)(disk + 1024 * gd->bg_inode_table + sizeof(struct ext2_inode) * (inode_num - 1));
    if (inode -> i_size != 0) {
        // printf("%d", inode -> i_size);
        int inode_block_num;
        int count;
        char *name;
        int check;
        struct ext2_dir_entry_2 *entry;
        inode_block_num = 0;
        
        printf("\n");
    }
}
