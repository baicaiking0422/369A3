//
//  ext2_ln.c
//
//
//  Created by Yue Li on 2016-07-28.
//
// I am stucked in US and cannot collaborate with you guys very well as you know
// pls forgive me = =



unsigned char *disk;

// area for all main functions
void link(char *img, char *src, char *des);


// implementation
void link(char *img, char *src, char *des) {//open image, map to memory,
  int fd = open(img, O_RDWR);
  // map disk to virtual memory
  disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // 128* 1024? where is comes
  // error checking mmap
  if (disk == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }

  int i_src = getSrcInode(src); // get the source inode => it should be absolute path

  int i_pdir; // ?
  char *ln_name = getLinkName(&i_pdir, des) // refer -> getLinkName
  if (ln_name == NULL) {
    fprintf(stderr, "%s\n", "invalid name");
    exit(1);//
  }

  if (!write2ParentInodeAndBlock(i_pdir, ln_name, i_src) || !updateSrcInode(i_src)){
    fprintf(stderr, "%s\n", "Cannot create link");
    exit(1);
  }
}

int getSrcInode(char *src_dir) { // get the source's inode position (an int)
  int i_src = 2; // TODO: why its 2 but not other number?
  char *pch = strtok(src_dir, "/"); // return last token which is the file name
  struct ext2_group_desc *bg = (struct ext2_group_desc *) (disk + 2 * EXT2_BLOCK_SIZE); // TODO: why it takes disk and add 2* block size?
  struct ext2_inode *i_head = (struct ext2_inode *) (disk + bg->inode_table * EXT2_BLOCK_SIZE);

  while (pch != NULL) { // as long as file name is not null
    char pth_matched = 0; // path matched
    struct ext2_inode *inode = i_head + i_src - 1;
    int i;

    for (i = 0; i < 12 && inode->i_block[i] != 0 && !pth_matched; i++) { //TODO: 对应的block不为0，且没有matched path? 不太理解条件
      int acc_len = 0; //?
      int rec_len = 0; //?

      struct ext2_dir_entry_2 *pde = (struct ext2_dir_entry_2 *) (disk + inode->i_block[i] * EXT2_BLOCK_SIZE);

      while (acc_len < EXT2_BLOCK_SIZE) {
        pde = (void *)pde + rec_len;
        if (strncmp(pch, pde->name, pde->name_len) == 0 && strlen(pch) == pde->name_len){ // in case it matches
          pth_matched = 1;
          i_src = pde->inode;
          break;
        }
        rec_len = pde->rec_len;
        acc_len += rec_len;
      }
    }

    if(!pth_matched) {
      fprintf(stderr, "%s\n", "link not found" );
      exit(1);
    }
    pch = strtok(NULL, "/")
  }
  return i_src;
}

char *getLinkName(int *i_pdir, char *des_dir) {
  struct ext2_group_desc *bg = (struct ext2_group_desc *) (disk + 2 * EXT2_BLOCK_SIZE);
  struct ext2_inode *i_head = (struct ext2_inode *) (disk + bg->bg_inode_table)
  int search_inode = 2; // ? why two again?
  char *pch = strtok(des_dir, "/");
  char *next_pch = strtok(NULL, "/");

  while (acc_len < EXT2_BLOCK_SIZE) {

  }
}
