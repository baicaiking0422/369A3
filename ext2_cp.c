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

int main(int argc, const char * argv[]){
    if (argc != 4) {
        fprintf(stderr, "Usage: ext2_cp <image file name> <source file> <target path>\n");
        exit(1);
    }
    //image fd
    int fd = open(argv[1], O_RDWR);
    //local file
    int local_fd = open(argv[2], O_RDONLY);
    if (local_fd == -1) {
        fprintf(stderr, "This file does not exist.\n");
        return ENOENT;
    }

    //local file size and required blocks
    int file_size = lseek(local_fd, 0, SEEK_END);
    int file_block_num = (file_size - 1) / 1024 + 1;

    //record target path
    int path_len;
    int r_path;
    char *record_path;
    char *path;
    path_len = strlen(argv[3]);
    r_path = strlen(argv[3]);
    record_path = malloc(r_path+1);
    path = malloc(path_len+1);
    strncpy(record_path, argv[3],r_path);
    record_path[r_path] = '\0';
    strncpy(path, argv[3],path_len);
    path[path_len] = '\0';

    //record local filename
    int lc_len;
    char *lc_name;
    lc_len = strlen(argv[2]);
    lc_name = malloc(sizeof(char) * (lc_len + 1));
    strncpy(lc_name, argv[2], lc_len);
    lc_name[lc_len] = '\0';


    if (path[0] != '/') {
        fprintf(stderr, "This is not an absolute path!");
        exit(1);
    }

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }


    struct ext2_group_desc * gd = (struct ext2_group_desc *)(disk + 2048);
    void *inodes = disk + 1024* gd->bg_inode_table;

    //get inode and block bitmap
    int *inode_bitmap = get_inode_bitmap(disk + 1024 * gd->bg_inode_bitmap);
    int *block_bitmap = get_block_bitmap(disk + 1024 * gd->bg_block_bitmap);

    struct ext2_inode *inode;
    struct ext2_inode *check_inode;

    // get inode number from absolute path
    //check if already dir
    // /a/c/ /a/c are different
    int inode_num, inode_num_p, type_add;
    char *file_name;
    char *file_parent_path;
    char *new_path;
    type_add = 1;
    //int new_path_len;

    inode_num = get_inode_num(path, inodes, disk);
    //inode for full path
    check_inode = (struct ext2_inode *)(disk +1024 * gd->bg_inode_table +
                                                          sizeof(struct ext2_inode) * (inode_num - 1));
    // inode is file and already exist
    if (inode_num != -1) {
        if (check_inode->i_mode & EXT2_S_IFREG || check_inode->i_mode & EXT2_S_IFLNK ){
            perror("This file name existed");
            exit(ENOENT);
        }
        else if (check_entry_file(lc_name, check_inode, disk) == -1) {
            perror("This file name existed");
            exit(ENOENT);
        }
        type_add = 0;
    }

    // not exist
    //if (inode_num == -1){
    get_file_parent_path(record_path, &file_parent_path);
    get_file_name(record_path, &file_name); //record file name
    new_path = malloc(strlen(file_parent_path));
    strcpy(new_path, file_parent_path); // record new path

    inode_num_p = get_inode_num(new_path, inodes, disk);
    if ((inode_num_p != -1) && (record_path[strlen(record_path) - 1] == '/')) {
        fprintf(stderr, "Illegal target path for copy1\n");
        return ENOENT;
    }
    // inode for parent path
    inode = (struct ext2_inode *)(disk + 1024 * gd->bg_inode_table + sizeof(struct ext2_inode) * (inode_num - 1));
        //check parent path
    //printf("%d\n",inode_num_p);
    if ((inode_num_p == -1) ||
                            ((inode_num_p != -1) && (inode->i_mode & EXT2_S_IFREG))) {
        fprintf(stderr, "Illegal target path for copy2\n");
        return ENOENT;
    }



        //else{ keyi
            //get a free new inode from inode bitmap
            int new_inode = -1;
            int i;
            for (i = 0; i < 32; i++) {
                if (inode_bitmap[i] == 0) {
                    new_inode = i;
                    break;
                }
            }
            //printf("%d\n",new_inode);
            //check new node for copy file
            if  (new_inode == -1){ // no free inode to assign
                fprintf(stderr, "No free inode\n");
                exit(1);
            }

            //get free blocks from block bitmap
            int *free_blocks = get_free_block(block_bitmap, file_block_num);
            //check free_blocks enough
            if (free_blocks == NULL){
                fprintf(stderr, "Not enough blocks in the disk\n");
                exit(1);
            }

            //copy local file
            unsigned char* local_file = mmap(NULL, file_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, local_fd, 0);
            if(local_file == MAP_FAILED) {
                perror("mmap");
                exit(1);
            }
            // copy local file  to disk block
            int j;
            int total_size = file_size;
            int record = 0;
            for (j = 0; j < file_block_num; j++){
                set_block_bitmap(disk + 1024 * gd->bg_block_bitmap, free_blocks[i], 1);
                if (total_size < 1024){
                    memcpy(disk + 1024 * (free_blocks[i] +1), local_file + record, total_size);
                    break;
                }else{
                    memcpy(disk + 1024 * (free_blocks[i] +1), local_file + record, 1024);
                    total_size -= 1024;
                    record += 1024;
                }
            }
            //assign new node
            struct ext2_inode *n_inode = inodes + sizeof(struct ext2_inode) * new_inode;
            //new_inode num = new_inode + 1
            n_inode->i_mode = EXT2_S_IFREG;
            n_inode->i_size = file_size;
            n_inode->i_links_count = 1;
            n_inode->i_blocks = file_block_num * 2;
            for (j = 0; j < 12; j++){
                n_inode->i_block[i] = free_blocks[i] + 1;
            }
            set_inode_bitmap(disk + 1024 * gd->bg_inode_bitmap, new_inode, 1);

            //new entry for new file in parent dir
            int count;
            int check;
            struct ext2_dir_entry_2 *n_entry;
            //ƒdoint num_blocks_parent = inode->i_size / 1024;
            for (j = 0; j < 12; j++){
                count = 0;
                check = 0;
                //find first empty i_block position
                if (inode->i_block[j] == 0){
                    int *n_free_block = get_free_block(block_bitmap, 1);
                    if (n_free_block == NULL){
                        fprintf(stderr, "No emplty block\n");
                        exit(1);
                    }
                    inode->i_block[j] = *n_free_block;
                    n_entry = (struct ext2_dir_entry_2*)(disk + 1024 * (inode->i_block[j]+1));
                    n_entry->inode = new_inode + 1;
                    n_entry->rec_len = 1024;
                    n_entry->name_len = strlen(file_name);
                    n_entry->file_type = EXT2_FT_REG_FILE;
                    strncpy((void *)n_entry + 8, file_name, strlen(file_name));
                    set_block_bitmap(disk + 1024 * gd->bg_block_bitmap, inode->i_block[j], 1);

                    inode->i_size += 1024;
                    inode->i_blocks += 2;
                    break;
                }

            }
        //else}


   //if }return 0;
//main blanket
}
