int main(int argc, char **argv) {

    /* This program takes three command line arguments.
    1st is the name of an ext2 formatted virtual disk.
    2nd is the path to a file on your native operating system,
    3rd is an absolute path on your ext2 formatted disk. */
    if (argc != 4) {
        fprintf(stderr, "Usage: ext2_cp <virtual disk name> <native path> <abs path>\n");
        exit(1);
    }

    /* mmap disk */
    int fd = open(argv[1], O_RDWR);
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(disk == MAP_FAILED) {
	    perror("mmap");
	    exit(1);
	}

    /* Init ext2_super_block & ext2_group_desc */
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2048);
}
