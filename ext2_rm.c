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

    /* Open */
    int fd = open(argv[1], O_RDWR);
    char *path;

    path = malloc(strlen(argv[2]));
    strcpy(path, argv[2]);

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
    char *parent_path;
    char *file_name;
    get_file_parent_path(path, &parent_path);
    get_file_name(path, &file_name);
    int inode_num = get_inode_num(parent_path, inodes, disk);

    if (inode_num == -1) {
        perror("The file does not exist.\n");
        exit(ENOENT);
    }

    struct ext2_inode *inode = (struct ext2_inode *)(disk + 1024 * gd->bg_inode_table + sizeof(struct ext2_inode) * (inode_num - 1));
    struct ext2_dir_entry_2 *entry;
    struct ext2_dir_entry_2 *next_entry;
    int count, timeout, offset, found;
    char *name;
    count = 0;
    timeout = 0;
    offset = 0;
    found = 0;

    while (count < 1024) {

        entry = (struct ext2_dir_entry_2 *)(disk + 1024 * inode->i_block[0] + count);
        count += entry->rec_len;
        next_entry = (struct ext2_dir_entry_2 *)(disk + 1024 * inode->i_block[0] + count);

        name = malloc(sizeof(char) * next_entry->name_len + 1);
        strncpy(name, next_entry->name, next_entry->name_len);
        name[next_entry->name_len] = '\0';

        // If found
        if (strcmp(file_name, name) == 0) {

            found = 1;

            // If not a regular file
            if (next_entry->file_type & EXT2_S_IFREG) {
                perror("This is not a regular file.\n");
                exit(ENOENT);
            }

            // If is a regular file
            entry->rec_len += next_entry->rec_len;
            next_entry->rec_len = 0;
            next_entry->name_len = 0;
            strcpy(next_entry->name, "");
            next_entry->inode = 0;

        }

        free(name);

        timeout ++;
        if (timeout > 10) {
            break;
        }
    }

    /* If the file does not exist. */
    if (!found) {
        perror("The file does not exist.\n");
        exit(ENOENT);
    }

}
