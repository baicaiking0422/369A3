//
//  ext2_ln.c
//
//
//  Created by Yue Li on 2016-07-28.
//
//

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


// helper functions, better write in ext2_helper but there is a warning when compile it, might relate to C99
// if have time find a way to put it back to helper

unsigned char *disk;

int find_dirn(char* list, char* buf) {
  int i, j;

  if (list[0] != '/') {
    return -1;
  }

  for (i = 1, j = 0; list[i] != '/'; i++, j++) {
    if (list[i] == '\0') {
      break;
    }
    buf[j] = list[i];
  }
  buf[j] = '\0';

  return i;
}

int find_dir_inode(char* query, void* inodes) {

  struct ext2_inode* inode;
  struct ext2_dir_entry_2* dir;

  int n_inode = EXT2_ROOT_INO;
  int n_block;

  char buf[512];

  while (1) {
    inode = (struct ext2_inode*)
      (inodes + (n_inode - 1) * sizeof(struct ext2_inode));
    int total_size = inode->i_size;

    if (strcmp(query, "/") == 0) {
      return n_inode - 1;
    }

    if (strcmp(query, "") == 0) {
      query[1] = '\0';
      query[0] = '/';
    }


    int n = find_dirn(query, buf);

    query += n;
    int new_n_inode = -1;
    int n_blockidx = 0;
    int size = 0;

    n_block = inode->i_block[n_blockidx];
    dir = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * n_block);

    while (size < total_size) {
      size += dir->rec_len;
      if (dir->file_type == EXT2_FT_DIR) {
        if (strlen(buf) == dir->name_len &&
            strncmp(buf, dir->name, dir->name_len) == 0) {
          new_n_inode = dir->inode;
          break;
        }
      }

      dir = (void*)dir + dir->rec_len;

      if (size % EXT2_BLOCK_SIZE == 0) {
        n_blockidx++;
        n_block = inode->i_block[n_blockidx];
        dir = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * n_block);
      }
    }

    if (new_n_inode == -1) {
      return -1;
    } else {
      n_inode = new_n_inode;
    }

  }
}

/*
detach a file
*/
void file_detach(char* path, char* name) {

  int length = strlen(path);
  int counter = length - 1;
  while (path[counter] != '/') {
    counter--;
  }
  int len_n = length - 1 - counter;
  strncpy(name, path + length - len_n, len_n + 1);
  if (counter != 0) {
    path[counter] = '\0';
  } else {
    path[counter + 1] = '\0';
  }
}

int main(int argc, char **argv) {

  if ((argc != 4) && (argc != 5)) {
    fprintf(stderr, "Usage: ext2_ln <image file name> <source path> <target path>\n");
    exit(1);
  }

  if ((argc == 5) && strcmp(argv[2], "-a") != 0) {
    fprintf(stderr, "Wrong command\n");
  }

  int fd = open(argv[1], O_RDWR);

  char* src;
  char* des;
  if (argc == 4) {
    src = argv[2];
    des = argv[3];
  }
  if (argc == 5) {
    src = argv[3];
    des = argv[4];
  }

  src = argv[2];
  des = argv[3]; // how to put these in an if statement???

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

// error checking => absolute path TODO: Is the check necessary?
  if (src[0] != '/' || des[0] != '/') {
    fprintf(stderr, "not absolute path\n");
    exit(1);
  }

  int i;
  char src_n[1024];
  char des_n[1024];
  int src_nlength;
  int des_nlength;
  file_detach(src, src_n);
  file_detach(des, des_n);
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
  int src_inodeidx = find_dir_inode(src, inodes); //TODO: move find_dir_inode
  int des_inodeidx = find_dir_inode(des, inodes); //TODO: move find_dir_inode

// error checking =>
  if (src_inodeidx < 0 || des_inodeidx < 0) {
    errno = ENOENT;
    if (src_inodeidx < 0) {
      perror(src);
    }
    if (des_inodeidx < 0) {
      perror(des);
    }
    exit(errno);
  }

  int desf_inodeidx = -1;
  struct ext2_inode* des_inode = des_inodeidx * sizeof(struct ext2_inode) + inodes;
  int des_num_blocks = des_inode->i_size / EXT2_BLOCK_SIZE;
  int block_pos;
  struct ext2_dir_entry_2* dir;

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
  int req_size = ((des_nlength - 1)/4+1)*4+8;
  for (i = 0; i < des_num_blocks; i++) {
    block_pos = des_inode->i_block[i];
    dir = (void*)disk + block_pos * EXT2_BLOCK_SIZE;
    int size = 0;
    while (size < EXT2_BLOCK_SIZE) {
      size += dir->rec_len;
      if (size == EXT2_BLOCK_SIZE){
        if ((dir->rec_len) >= req_size + 8 + ((dir->name_len - 1)/4+1)*4) {
          exist = 0;
          break;
        }
      }
      dir = (void*)dir + dir->rec_len;
    }
      if (exist == 0) {
        break;
      }
    }
    if (exist == 1) { // not found in this case
      block_bitmap = get_block_bitmap(block_bitmaploc);
      int* free_block = get_free_block(block_bitmap, 1);
      if (free_block == NULL) {
        errno = ENOSPC;
        perror("not enough free block\n");
        exit(errno);
      }
      int idx_temp = *free_block;
      dir = (void*)disk + EXT2_BLOCK_SIZE * (idx_temp + 1);
      dir->inode = srcf_inodeidx + 1;
      dir->file_type = EXT2_FT_REG_FILE;
      dir->rec_len = EXT2_BLOCK_SIZE;
      dir->name_len = des_nlength;
      strncpy((void*)dir + 8, des_n, des_nlength);
      set_block_bitmap(block_bitmaploc, idx_temp, 1);
      des_inode->i_size += EXT2_BLOCK_SIZE;
      des_inode->i_blocks += 2;
      des_inode->i_block[des_num_blocks] = idx_temp + 1;
    } else {
      int prev = dir->rec_len;
      int cls = ((dir->name_len -1)/4+1)*4+8;
      dir->rec_len = cls;
      dir = (void*)dir + cls;
      dir->inode = srcf_inodeidx + 1;
      dir->file_type = EXT2_FT_REG_FILE;
      dir->rec_len = prev - cls;
      dir->name_len = des_nlength;
      strncpy((void*)dir + 8, des_n, des_nlength);
    }

    // dont forget to update the count of links
    struct ext2_inode* inode = inodes + sizeof(struct ext2_inode) * srcf_inodeidx;
    inode->i_links_count++;

    return 0;
}
