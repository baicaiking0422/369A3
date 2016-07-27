//
//  ext2_helper.c
//
//
//  Created by Zhujun Wang on 2016-07-27.
//
//

#include "ext2_helper.h"
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


int get_inode_num(char *path, void *inodes, unsigned char *disk){
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

    if (strcmp(path, "/") == 0){
        return cur_inode_num;
    }

    new_inode_num = cur_inode_num;
    while (token != NULL){
        //printf("%s\n", token);
        inode = (struct ext2_inode *)(inodes + (new_inode_num - 1) * sizeof(struct ext2_inode));
        //inode->i_size?
        if (inode->i_size != 0) {
            count = 0;
            inode_block_num = 0;
            int i;
            for (i = 0; i < 12; i ++) {
                while (count < 1024) {
                    entry = (struct ext2_dir_entry_2*)(disk+1024 * inode->i_block[inode_block_num] + count);
                    count += entry->rec_len;
                    if (entry->file_type == EXT2_FT_DIR){
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

char *get_file_name(char *file_path, char *file_name) {
    int i;
    i = strlen(file_path) - 1;
    while (i >= 0 && file_path[i] != '/') {
        i--;
    }
    strncpy(file_name, file_path + i + 1, strlen(file_path) - i);
    //return file_name;
}


void *get_file_parent_path(char *file_path, char *file_parent_path){
    int i;
    i = strlen(file_path) - 1;
    while (i >= 0 && file_path[i] != '/') {
        i--;
    }
    strncpy(file_parent_path, file_path, i +1);
    //return file_parent_path;
}
