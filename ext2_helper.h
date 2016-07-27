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

#endif /* ext2_helper_h */
