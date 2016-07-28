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
        check_exist = -1;
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
                    //if (entry->file_type == EXT2_FT_DIR){
                        name = malloc(sizeof(char) * entry->name_len);
                        strncpy(name, entry->name, entry->name_len);
                        if (strcmp(token, name) == 0) {
                            new_inode_num = entry->inode;
                            check_exist = 0;
                            break;
                        }
                    //}
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

 void get_file_name(char *file_path, char **file_name) {

    int i;
    i = strlen(file_path) - 1;
    if (file_path[i] != '/'){
        while (i >= 0 && file_path[i] != '/') {
        i--;
    }
        *file_name = malloc(sizeof(char) * (strlen(file_path) - i));
        strncpy(*file_name, file_path + i + 1, strlen(file_path) - i);
    } else {
        while (i >= 0 && file_path[i-1] != '/') {
        i--;
    }
        *file_name = malloc(sizeof(char) * (strlen(file_path) - i - 1));
        strncpy(*file_name, file_path + i, strlen(file_path) - i - 1);
    }

    //return file_name;

}


void get_file_parent_path(char *file_path, char **file_parent_path){

    int i;
    i = strlen(file_path) - 1;
    if (file_path[i] != '/'){
        while (i >= 0 && file_path[i] != '/') {
            i--;
        }

        *file_parent_path = malloc(sizeof(char) * (i + 1));

        strncpy(*file_parent_path, file_path, i +1);
    }else{
        while (i >= 0 && file_path[i - 1] != '/') {
            i--;
        }
        *file_parent_path = malloc(sizeof(char) * (i + 1));
        strncpy(*file_parent_path, file_path, i + 1);
    }

 }   //return file_parent_path;

int *get_inode_bitmap(void *inode_info){
    int *inode_bitmap = malloc(sizeof(int) * 32);
    int i;
    for (i = 0; i < 32; i++) {
        char *byte = inode_info + (i / 8);
        inode_bitmap[i] = (*byte >> (i % 8)) & 1;
    }
    return inode_bitmap;
}

int *get_block_bitmap(void *block_info){
    int *block_bitmap = malloc(sizeof(int) * 128);
    int i;
    for (i = 0; i < 128; i++) {
        char *byte = block_info + (i / 8);
        block_bitmap[i] = (*byte >> (i % 8)) & 1;
    }
    return block_bitmap;
}

int *get_free_block(int *block_bitmap, int needed_num_blocks){
    int i, j;
    j = 0;
    int *new_blocks = malloc(sizeof(int) * needed_num_blocks);
    for (i = 0; i < needed_num_blocks; i++) {
        while (block_bitmap[j] != 0) {
            j++;
            if (j == 128) {
                return NULL;
            }
        }
        new_blocks[i] = j;
    }
    return new_blocks;
}