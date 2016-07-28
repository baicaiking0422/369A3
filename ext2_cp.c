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

    int fd = open(argv[1], O_RDWR);
    int local_fd = open(argv[2], O_RDONLY);
    if (local_fd == -1) {
        perror(argv[2]);
        exit(ENOENT);
    }
    
    
    int path_len;
    int r_path;
    char *record_path;
    char *path;
    path_len = strlen(argv[3]);
    r_len = strlen(argv[3]);
    record_path = malloc(r_len);
    path = malloc(path_len);
    strcpy(record_path, argv[3]);
    strcpy(path, argv[3]);

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
    int *inode_bitmap = 
    
    struct ext2_inode *inode;
    struct ext2_inode *check_inode;

    // get inode number from absolute path
    //check if already dir
    // /a/c/ /a/c are different
    int inode_num;
    char *file_name;
    char *file_parent_path;
    char *new_path;
    int new_path_len;
    int new_inode = -1;
    int i;
    for (i = 0; i < 32; i++) {
        if (<#condition#>) {
            <#statements#>
        }
    }
    
    inode_num = get_inode_num(path, inodes, disk);
    //check this inode with inode_num
    check_inode = (struct ext2_inode *)(disk +1024 * gd->bg_inode_table +
                                                          sizeof(struct ext2_inode) * (inode_num - 1));
    // inode is file and already exist
    if ((inode_num != -1) && (check_inode->i_mode & EXT2_S_IFREG || check_inode->i_mode & EXT2_S_IFLNK ) {
        perror("This file name existed");
        exit(ENOENT);
    }
    // not exist
    if (inode_num == -1){
        get_file_parent_path(record_path, file_name);
        get_file_name(record_path, file_parent_path); //record file name
        new_path = malloc(strlen(file_parent_path));
        strcpy(new_path, file_parent_path); // record new path
        
        inode_num = get_inode_num(file_parent_path, inodes, disk);
        inode = (struct ext2_inode *)(disk + 1024 * gd->bg_inode_table + sizeof(struct ext2_inode) * (inode_num - 1));
        //check parent path
        if ((inode_num == -1) ||
                            ((inode_num != -1) && (inode->i_mode & EXT2_S_IFREG || inode->i_mode & EXT2_S_IFLNK ))) {
            fprintf(stderr, "Illegal target path for copy\n");
            return ENOENT;
        }else{
            //new node
            
            
    }
    

    


//main blanket
}
