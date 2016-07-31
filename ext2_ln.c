//
//  ext2_ln.c
//
//
//  Created by Yue Li on 2016-07-28.
//
// I am stucked in US and cannot collaborate with you guys very well as you know
// pls forgive me = =
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <linux/limits.h>
#include "ext2.h"
#include "ext2_helper.h"

unsigned char *disk;



int main(int argc, char **argv) {

  if ((argc != 4) && (argc != 5)) {
    fprintf(stderr, "Usage: ext2_ln <image file name> <source path> <target path>\n");
    exit(1);
  }

  if ((argc == 5) && strcmp(argv[2], "-a") != 0) {
    fprintf(stderr, "Wrong command\n");
  }

  int fd = open(argv[1], O_RDWR);
  // int src_len = strlen(src);
  // int des_len = strlen(des);

  if (argc == 4) {
    char* src = argv[2];
    char* des = argv[3];
  } else if (argc = 5) {
    char* src = argv[3];
    char* des = argv[4];
  }
// error checking => absolute path TODO: Is the check necessary?
  if (src[0] != '/' || des[0] != '/') {
    fprintf(stderr, "not absolute path\n");
    exit(1);
  }

// error checking => if either location refers to a directory (EISDIR)
  if (src[strlen(src) - 1] == '/' || des[strlen(src) - 1] == '/'){
    errno = EISDIR;
    if (src[strlen(src) - 1] == '/') {
      perror(src);
    } else {
      perror(des);
    }
    exit(errno);
  }

  int i;
  char src_n[1024];
  char des_n[1024];
  int src_nlength;
  int des_nlength;
  get_file_name(src, src_n);
  get_file_name(des, des_n);
  src_nlength = strlen(src_n);
  des_nlength = strlen(des_n);

// map disk to memory
  disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  struct ext2_group_desc* gd = (void*)disk + 1024 + EXT2_BLOCK_SIZE;
  void* inodes = disk + EXT2_BLOCK_SIZE * gd->bg_inode_table;
  void* block_bitmaploc = disk + EXT2_BLOCK_SIZE * gd->bg_block_bitmap;
  int* block_bitmap = get_block_bitmap(block_bitmaploc);
// error checking => in case map Failed
  if (disk == MAP_FAILED) {
    perror("mmap map failed\n");
    exit(1);
  }

  // find src dir, des dir
  int src_inodeidx = find_dir_inode(src, inodes); //TODO: find_dir_inode
  int des_inodeidx = find_dir_inode(des, inodes); //TODO: find_dir_inode
  if (src_inodeidx < 0 || des_inodeidx < 0) {
    errno = ENOENT;
    if (src_inodeidx < 0) {
      perror(src);
    } else {
      perror(des);
    }
    exit(errno);
  }

  int desf_inodeidx = -1;
  struct ext2_inode des_inode = inodes + des_inodeidx * sizeof(struct ext2_inode);
  des_num_blocks = des_inode->i_size / EXT2_BLOCK_SIZE;
  for (i = 0; i < des_num_blocks; i++) {
    block_pos = des_inode->i_block[i];
    dir = (void*)dir + EXT2_BLOCK_SIZE * block_pos;
    int size = 0;
    while (size < EXT2_BLOCK_SIZE) {
      size += dir->rec_len;
      if (des_nlength == dir->name_len && strncmp(des_n, dir->name, des_nlength) == 0) {
        desf_inodeidx = dir->inode - 1;
        break;
      }
      dir = (void*)dir + dir->rec_len;
    }
    if (desf_inodeidx != -1) {
      break;
    }
  }
// error checking => file is already in directory
  if (desf_inodeidx != -1) {
    errno = EEXIST;
    perror(des_n);
    exit(errno);
  }


  // find the source file in source directory
  int srcf_inodeidx = -1;

  struct ext2_inode* src_inode = inodes + src_inodeidx * sizeof(struct ext2_inode);
  int src_num_blocks = src_inode->i_size / EXT2_BLOCK_SIZE;
  int block_pos;
  struct ext2_dir_entry_2* dir;

  for (i = 0; i < src_num_blocks; i++) {
    block_pos = src_inode->i_block[i];
    dir = (void*)disk + EXT2_BLOCK_SIZE * block_pos;
    int size = 0;
    while (size < EXT2_BLOCK_SIZE) {
      size += dir->rec_len;
      if (des_nlength == dir->name_len && strncmp(des_n, dir->name, des_nlength) == 0) {
        if (dir->file_type == EXT2_FT_REG_FILE) {
          srcf_inodeidx = dir->inode - 1;
          break;
        }
      }
      dir = (void*)dir + dir->rec_len;
    }
    if (srcf_inodeidx != -1) {
      break;
    }
  }
// error checking => file not in directory
  if (srcf_inodeidx == -1) {
    errno = ENOENT;
    perror(src_n);
    exit(errno);
  }


  int exist = 1;
  int req_size = ((des_nlength - 1) / 4 + 1 ) * 4 + 8;
  for (i = 0; i < des_num_blocks; i++) {
    block_pos = des_inode->i_block[i];
    dir = (void*)disk + block_pos * EXT2_BLOCK_SIZE;
    int size = 0;
    while (size < EXT2_BLOCK_SIZE) {
      size += dir->rec_len;
      if (size == EXT2_BLOCK_SIZE){
        if ((dir->rec_len) >= req_size + 8 + ((dir->name_len - 1) / 4 + 1) * 4 + 8) {
          exist = 0;
          break;
        }
        dir = (void*)dir + dir->rec_len;
      }
      if (exist == 0) {
        break;
      }
    }
    if (found == 1) { // not found in this case
      block_bitmap = get_block_bitmap(block_bitmaploc);
      int* free_block
    }
  }
}
