#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"
#include <string.h>
#include <strings.h>

unsigned char *disk;

void aaaa(int *a) {
    *a += 1;
}

int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: readimg <image file name>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + 2048);
    struct ext2_dir_entry_2 *entry;

    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);
    printf("Block group:\n");
    printf("\tblock bitmap: %d\n", gd->bg_block_bitmap);
    printf("\tinode bitmap: %d\n", gd->bg_inode_bitmap);
    printf("\tinode table: %d\n", gd->bg_inode_table);
    printf("\tfree blocks: %d\n", gd->bg_free_blocks_count);
    printf("\tfree inodes: %d\n", gd->bg_free_inodes_count);
    printf("\tused_dirs: %d\n", gd->bg_used_dirs_count);


    
    int i, j, tmp;
    printf("Block bitmap:");
    for(i = 0; i < 16; i++) {
        tmp = disk[gd->bg_block_bitmap * 1024 + i];
        for(j = 0; j < 8; j++) {
            if (tmp & 1)
                printf("1");
            else
                printf("0");
            tmp >>= 1;
        }
        printf(" ");
    }
    printf("\n");

    printf("Inode bitmap:");
    for(i = 0; i < 4; i++) {
        tmp = disk[gd->bg_inode_bitmap * 1024 + i];
        for(j = 0; j < 8; j++) {
            if (tmp & 1)
                printf("1");
            else
                printf("0");
            tmp >>= 1;
        }
        printf(" ");
    }
    printf("\n\nInodes:\n");

    int count;
    char type;
    for(i = 1; i < sb->s_inodes_count; i++) {
        if(i < EXT2_GOOD_OLD_FIRST_INO && i != EXT2_ROOT_INO - 1) {
            continue;
        } 
        struct ext2_inode *inode = (struct ext2_inode *)(disk + 1024 * gd->bg_inode_table + sizeof(struct ext2_inode) * i);

        if ((inode->i_mode & EXT2_S_IFLNK) == EXT2_S_IFLNK) {
            type = 'l';
        } else if (inode->i_mode & EXT2_S_IFDIR) {
            type = 'd';
        } else if ((inode->i_mode & EXT2_S_IFREG) == EXT2_S_IFREG ) {
            type = 'f';
        }
        if(inode->i_size != 0) {
            printf("[%d] type: %c size: %d links: %d blocks: %d\n", i + 1, type, inode->i_size, inode->i_links_count, inode->i_blocks);
            printf("[%d] Blocks: ", i + 1);
            for(j = 0; j < 12; j++) {
                if(inode->i_block[j] != 0)
                    printf(" %d", inode->i_block[j]);
            }
            if(inode->i_block[12] != 0) {
                entry = (struct ext2_dir_entry_2*)(disk + 1024 * inode->i_block[12]);
                count = 0;
                while(count < 1024 && disk[1024 * inode->i_block[12] + count] != 0) {
                    printf(" %d", disk[1024 * inode->i_block[12] + count]);
                    count += 4;
                }
            }
        
            printf("\n");
        }
    }

    
    char *name;
    
    for(i = 0; i < sb->s_inodes_count; i++) {
        if(i < EXT2_GOOD_OLD_FIRST_INO && i != EXT2_ROOT_INO - 1) {
            continue;
        }
        struct ext2_inode *inode = (struct ext2_inode *)(disk + 1024 * gd->bg_inode_table + 128 * i);
        if(inode->i_mode & EXT2_S_IFREG) {
            continue;
        }

        if(inode->i_size != 0) {
            for(j = 0; j < 12; j++) {
                if(inode->i_block[j] != 0) {
                    entry = (struct ext2_dir_entry_2*)(disk + 1024 * inode->i_block[j]);
                    
                    printf("\tDIR BLOCK NUM: %d (for inode %d)\n", inode->i_block[j], i + 1);
            
                    count = 0;
                    while(count < 1024) {
                        entry = (struct ext2_dir_entry_2*)(disk + 1024 * inode->i_block[j] + count);
                        
                        if (entry->file_type == EXT2_FT_REG_FILE) {
                            type = 'f';
                        } else if (entry->file_type == EXT2_FT_DIR) {
                            type = 'd';
                        } else if (entry->file_type == EXT2_FT_SYMLINK) {
                            type = 'l';
                        } else if (entry->file_type == EXT2_FT_UNKNOWN) {
                            type = 'u';
                        }
                        
                        count += entry->rec_len;
                        name = malloc(sizeof(char) * (entry->name_len + 1));
                        strncpy(name, entry->name, entry->name_len);
                        name[entry->name_len] = '\0';
                        printf("Inode: %d rec_len: %d name_len: %d type= %c name=%s\n", entry->inode, entry->rec_len, entry->name_len, type, name);
                        free(name);
                    }
                }
            }
            
        }
    }
    return 0;
}
