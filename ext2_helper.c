//
//  ext2_helper.c
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

/*
 * Get an inode index with path from disk
 */
int get_inode_num(char *path, void *inodes, unsigned char *disk){
    int cur_inode_num = EXT2_ROOT_INO;
    int new_inode_num;
    int check_exist = -1;
    int count;
    char *name;
    char *token;
    //int inode_block_num;
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
            int i;
            for (i = 0; i < 12; i ++) {
                if (inode->i_block[i] != 0) {
                    while (count < 1024) {
                        entry = (struct ext2_dir_entry_2*)(disk+1024 * inode->i_block[i] + count);
                        count += entry->rec_len;
                        name = malloc(sizeof(char) * (entry->name_len+1));
                        strncpy(name, entry->name, entry->name_len);
                        name[entry->name_len] = '\0';
                        if (strcmp(token, name) == 0) {
                            new_inode_num = entry->inode;
                            check_exist = 0;
                            break;
                        }
                    }
                }
            }
            // indirection
            if (inode->i_block[12] != 0){
	            entry = (struct ext2_dir_entry_2*)(disk + 1024 * inode->i_block[12]);
		        count = 4;
		        while (count < 1024 && (disk[1024 * inode->i_block[12] + count] != 0)){
		            entry = (struct ext2_dir_entry_2*)(disk + 1024 * inode->i_block[12] + count);
		            count += 4;
		            name = malloc(sizeof(char) * (entry->name_len+1));
		            strncpy(name, entry->name, entry->name_len);
		            name[entry->name_len] = '\0';
                    if (strcmp(token, name) == 0){
                        new_inode_num = entry->inode;
                        check_exist = 0;
                        break;
		            }
                }
            }
        }
        free(name);
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

/*
 * Seperate original path to a parent directory path
 */

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

/*
 * Seperate original path to a file path
 */

void get_file_parent_path(char *file_path, char **file_parent_path){

    int i;
    i = strlen(file_path) - 1;
    if (file_path[i] != '/'){
        while (i >= 0 && file_path[i] != '/') {
            i--;
        }

        *file_parent_path = malloc(sizeof(char) * (i + 1 + 1));

        strncpy(*file_parent_path, file_path, i +1);
        (*file_parent_path)[i+1] = '\0';

    }else{
        i--;
        while (i >= 0 && file_path[i] != '/') {
            i--;
        }
        *file_parent_path = malloc(sizeof(char) * (i + 1 + 1));
        strncpy(*file_parent_path, file_path, i + 1);
        (*file_parent_path)[i+1]= '\0';
    }

 }   //return file_parent_path;

/*
 * Get an inode bitmap from disk
 */

int *get_inode_bitmap(void *inode_info){
    int *inode_bitmap = malloc(sizeof(int) * 32);
    int i;
    for (i = 0; i < 32; i++) {
        char *byte = inode_info + (i / 8);
        inode_bitmap[i] = (*byte >> (i % 8)) & 1;
    }
    return inode_bitmap;
}

/*
 * Get a block bitmap from disk
 */

int *get_block_bitmap(void *block_info){
    int *block_bitmap = malloc(sizeof(int) * 128);
    int i;
    for (i = 0; i < 128; i++) {
        char *byte = block_info + (i / 8);
        block_bitmap[i] = (*byte >> (i % 8)) & 1;
    }
    return block_bitmap;
}

/*
 * Get an array of free block index with block bitmap
 */

int *get_free_block(int *block_bitmap, int needed_num_blocks){
    int i, j;
    j = 0;
    int *new_blocks = malloc(sizeof(int) * needed_num_blocks);
    for (i = 0; i < needed_num_blocks; i ++) {
        while (block_bitmap[j] != 0) {
            j++;
            if (j == 128) {
                return NULL;
            }
        }
        new_blocks[i] = j;
        j++;
    }
    return new_blocks;
}

/*
 * Set bit to 0 or 1 on inode bitmap
 */

void set_inode_bitmap(void *inode_info, int inode_num, int bit){
    int byte = inode_num / 8;
    int i_bit = inode_num % 8;
    char *change = inode_info + byte;
    *change = (*change & ~(1 << i_bit)) | (bit << i_bit);
}

/*
 * Set bit to 0 or 1 on block bitmap
 */
void set_block_bitmap(void *block_info, int block_num, int bit){
    int byte = block_num / 8;
    int b_bit = block_num % 8;
    char *change = block_info + byte;
    *change = (*change & ~(1 << b_bit)) | (bit << b_bit);
}

/*
 * Check where inode has already get the local file entry
 */
int check_entry_file(char *lc_file, struct ext2_inode *check_inode, unsigned char *disk){
    int i, count;
    char *name;
    struct ext2_dir_entry_2 *entry;
    
    if (check_inode->i_size != 0) {
        count = 0;
        for (i = 0; i < 12; i++) {
            if (check_inode->i_block[i] != 0) {
                while (count < 1024) {
                    entry = (struct ext2_dir_entry_2*)(disk + 1024 * check_inode->i_block[i] + count);
                    count += entry->rec_len;
                    if (entry->file_type == EXT2_FT_REG_FILE){
                        name = malloc(sizeof(char) * (entry->name_len+1));
                        strncpy(name, entry->name, entry->name_len);
                        name[entry->name_len] = '\0';
                        if (strcmp(lc_file, name) == 0) {
                            return -1;
                        }
                    }
                }
            }
        }

        return 0;
    }
    return 1;
}
