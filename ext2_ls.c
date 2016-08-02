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
    
    /* check correct path*/
    if((argc != 3) && (argc != 4)) {
        fprintf(stderr, "Usage: readimg <image file name> <abs path>\n");
        exit(1);
    }
    
    /* list all */
    if (argc == 4 && (strcmp(argv[2], "-a") != 0)) {
        fprintf(stderr, "Wrong command\n");
        exit(1);
    }
    
    /* Read and record path*/
    int fd = open(argv[1], O_RDWR);
    int path_len;
    char *path;
    char *record_path;
    int r_path;

    if (argc == 4 && (strcmp(argv[2], "-a") == 0)) {
        path_len = strlen(argv[3]);
        r_path = strlen(argv[3]);
        record_path = malloc(r_path+1);
        path = malloc(path_len+1);
        strncpy(record_path, argv[3],r_path);
        record_path[r_path] = '\0';
        strncpy(path, argv[3],path_len);
        path[path_len] = '\0';
    }

    if (argc == 3) {
        path_len = strlen(argv[2]);
        r_path = strlen(argv[2]);
        record_path = malloc(r_path+1);
        path = malloc(path_len+1);
        strncpy(record_path, argv[2],r_path);
        record_path[r_path] = '\0';
        strncpy(path, argv[2],path_len);
        path[path_len] = '\0';
    }

    if (path[0] != '/') {
        fprintf(stderr, "This is not an absolute path!\n");
        exit(1);
    }


    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    struct ext2_group_desc * gd = (struct ext2_group_desc *)(disk + 2048);
    void *inodes = disk + 1024 * gd->bg_inode_table;
    
    /* Test full pat*/
    int inode_num = get_inode_num(path, inodes, disk);
    if (inode_num == -1) {
        fprintf(stderr, "The directory or file does not exist.\n");
        return ENOENT;
    }

    struct ext2_inode *inode = (struct ext2_inode *)(disk + 1024 * gd->bg_inode_table + sizeof(struct ext2_inode) * (inode_num - 1));

    if (inode->i_mode & EXT2_S_IFREG || inode->i_mode & EXT2_S_IFLNK){
        char *file_name;
        get_file_name(record_path, &file_name);
        printf("%s\n", file_name);
        return 0;
    }

    if (inode -> i_size != 0) {
        int inode_block_num;
        int count;
        char *name;
        int check;
        struct ext2_dir_entry_2 *entry;

        for (inode_block_num = 0; inode_block_num < 12; inode_block_num ++) {
            count = 0;
            check = 0;
            if (inode->i_block[inode_block_num] != 0) {

                while (count < 1024) {
                    entry = (struct ext2_dir_entry_2*)(disk + 1024 * inode -> i_block[inode_block_num] + count);
                    count += entry->rec_len;
                    check ++;
                    name = malloc(sizeof(char) * (entry->name_len+1));
                    strncpy(name, entry->name, entry->name_len);
                    name[entry->name_len] = '\0';
                    if (argc == 4) {
                        printf("%s", name);
                        if (entry->rec_len != 1024) {
                            printf("\n");
                        }
                    }

                    else {
                        if (check >= 3) {
                            printf("%s", name);
                            if (entry -> rec_len != 1024) {
                                printf("\n");
                            }
                        }
                    }
                }
            }
        }
        
        // indirection
        if (inode->i_block[12] != 0){
            entry = (struct ext2_dir_entry_2*)(disk + 1024 * inode->i_block[12]);
            count = 4;
            while (count < 1024 && (disk[1024 * inode->i_block[12] + count != 0])) {
                entry = (struct ext2_dir_entry_2*)(disk + 1024 * inode->i_block[12] + count);
                count += 4;
                name = malloc(sizeof(char) * (entry->name_len + 1));
                strncpy(name, entry->name, entry->name_len);
                name[entry->name_len] = '\0';
                printf("%s\n",name);
            }
        }
    }
    return 0;
}
