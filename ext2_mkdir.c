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
    
    //record target path
    char *path;
    char *r_path;
    int r_len;
    int path_len;
    path_len = strlen(argv[2]);
    path = malloc(path_len + 1);
    strncpy(path, argv[2], path_len);
    path[path_len] = '\0';
    r_len = strlen(argv[2]);
    r_path = malloc(r_len + 1);
    strncpy(r_path, argv[2], r_len);
    r_path[r_len] = '\0';
    
    
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
    int inode_num = get_inode_num(path, inodes, disk);

    if (inode_num != -1) {
        perror("The directory already exists.\n");
        exit(EEXIST);
    }

    
    char *parent_path;
    char *file_name;
    struct ext2_inode *inode;
    get_file_parent_path(r_path, &parent_path);
    get_file_name(r_path, &file_name);
    // parent dir inode num
    inode_num = get_inode_num(parent_path, inodes, disk);

    if (inode_num == -1) {
        perror("The directory does not exist.\n");
        exit(ENOENT);
    }
    
    // inode for parent path
    inode = (struct ext2_inode *)(disk + 1024 * gd->bg_inode_table + sizeof(struct ext2_inode) * (inode_num - 1));
    
    
    //get inode and block bitmap
    int *inode_bitmap = get_inode_bitmap(disk + 1024 * gd->bg_inode_bitmap);
    int *block_bitmap = get_block_bitmap(disk + 1024 * gd->bg_block_bitmap);
    
    //get a free new inode from inode bitmap
    int new_inode_idx = -1;
    int i;
    for (i = 0; i < 32; i++) {
        if (inode_bitmap[i] == 0) {
            new_inode_idx = i;
            break;
        }
    }
    
    //check new node for copy file
    if  (new_inode_idx == -1){ // no free inode to assign
        fprintf(stderr, "No free inode\n");
        exit(1);
    }
    
    //get one free blocks from block bitmap for new dir
    int *free_blocks = get_free_block(block_bitmap, 1);
    
    //check free_blocks enough
    if (free_blocks == NULL){
        fprintf(stderr, "Not enough blocks in the disk\n");
        exit(1);
    }
    
    set_block_bitmap(disk + 1024 * gd->bg_block_bitmap, *free_blocks, 1);
    gd->bg_free_blocks_count --;
    
    
    
    //assign new node
    struct ext2_inode *n_inode = inodes + sizeof(struct ext2_inode) * new_inode_idx;
    n_inode->i_mode = EXT2_S_IFDIR;
    n_inode->i_size = EXT2_BLOCK_SIZE;
    n_inode->i_links_count = 1;//change lnk?
    n_inode->i_blocks = 2;
    n_inode->i_block[0] = *free_blocks + 1;
    
    set_inode_bitmap(disk + 1024 * gd->bg_inode_bitmap, new_inode_idx, 1);
    gd->bg_free_inodes_count --;

    
    //set to parent entry
    int count;
    struct ext2_dir_entry_2 *n_entry;
    struct ext2_dir_entry_2 *past_entry;
    struct ext2_dir_entry_2 *chk_entry;
    
    int required_rec_len = ((7 + strlen(file_name)) / 4 + 1) * 4;
    int dir_num_blocks = (inode->i_size - 1) / 1024 + 1;
    int empty_rec_len = 0;
    //not indirected
    count = 0;
    if (inode->i_block[dir_num_blocks - 1] != 0) {
        while (count < 1024) {
            chk_entry = (struct ext2_dir_entry_2*)(disk + 1024 * inode->i_block[dir_num_blocks-1] + count);
            count += chk_entry->rec_len;
            if (count == 1024) {
                past_entry = chk_entry;
                int final_entry_rec_len = ((7 + chk_entry->name_len) / 4 + 1) * 4;
                empty_rec_len = chk_entry->rec_len - final_entry_rec_len;
                break;
            }
        }
    }
    
    if ((empty_rec_len > 0) && (empty_rec_len >= required_rec_len)) {
        past_entry->rec_len = ((7 + past_entry->name_len) / 4 + 1) * 4;
        n_entry = (struct ext2_dir_entry_2*)(disk + 1024 * inode->i_block[dir_num_blocks-1] + (1024 - empty_rec_len));
        n_entry->inode = new_inode_idx + 1;
        n_entry->rec_len = empty_rec_len;
        n_entry->name_len = strlen(file_name);
        n_entry->file_type = EXT2_FT_DIR;
        strncpy(n_entry->name, file_name, (int) n_entry->name_len +1);
        n_entry->name[(int) n_entry->name_len +1] = '\0';
    }
    
    // not enough empty_rec_len for this entry, need to open a new block
    if ((empty_rec_len = 0) || ((empty_rec_len > 0) && (empty_rec_len < required_rec_len))) {
        int *n_free_block = get_free_block(block_bitmap, 1);
        if (n_free_block == NULL){
            fprintf(stderr, "No empty block\n");
            exit(1);
        }
        inode->i_block[dir_num_blocks + 1] = *n_free_block;
        n_entry =(struct ext2_dir_entry_2*)(disk + 1024 * inode->i_block[dir_num_blocks+1]);
        n_entry->inode = new_inode_idx + 1;
        n_entry->rec_len = 1024;
        n_entry->name_len = strlen(file_name);
        n_entry->file_type = EXT2_FT_DIR;
        strncpy(n_entry->name, file_name, (int) n_entry->name_len +1);
        n_entry->name[(int) n_entry->name_len +1] = '\0';
        set_block_bitmap(disk + 1024 * gd->bg_block_bitmap, inode->i_block[dir_num_blocks+1], 1);
        gd->bg_free_blocks_count --;
        inode->i_size += 1024;
        inode->i_blocks += 2;
    }
    
    
    
    struct ext2_dir_entry_2 *self_entry;

    // Add .
    self_entry = (struct ext2_dir_entry_2 *)(disk + 1024 * n_inode->i_block[0]);
    self_entry->inode = new_inode_idx + 1;
    self_entry->rec_len = 12;
    self_entry->name_len = 1;
    self_entry->file_type |= EXT2_FT_DIR;
    strcpy(self_entry->name, ".");

    // Add ..
    self_entry = (struct ext2_dir_entry_2 *)(disk + 1024 * n_inode->i_block[0] + 12);
    self_entry->inode = inode_num;
    self_entry->rec_len = 1024 - 12;
    self_entry->name_len = 2;
    self_entry->file_type |= EXT2_FT_DIR;
    strcpy(self_entry->name, "..");
    
    return 0;
}
