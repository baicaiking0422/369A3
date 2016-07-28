//
//  ext2_helper.h
//
//
//  Created by Zhujun Wang on 2016-07-27.
//
//


#ifndef ext2_helper_h
#define ext2_helper_h

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
 * Get inode number with a path.
 *
 */
int get_inode_num(char *path, void *inodes, unsigned char *disk);

/*
 * Seperate file path and get the file name.
 *
 */
void get_file_name(char *file_path, char **file_name);

/*
 * Seperate file path and get the parent path of this file.
 *
 */
void get_file_parent_path(char *file_path, char **file_parent_path);

int *get_inode_bitmap(void *inode_info);
int *get_block_bitmap(void *block_info);
int *get_free_block(int *block_bitmap, int needed_num_blocks);
void set_inode_bitmap(void *inode_info, int inode_num, int bit);
void set_block_bitmap(void *block_info, int block_num, int bit);
#endif /* ext2_helper_h */
