FLAGS = -Wall -g -std=c99
all : ext2_ls

ext2_ls: ext2_ls.o ext2_helper.o
	gcc ${FLAGS} -o ext2_ls ext2_ls.o ext2_helper.o


ext2_helper.o : ext2_helper.c ext2_helper.h ext2.h
	gcc ${FLAGS} -c ext2_helper.c

ext2_ls.o : ext2_ls.c ext2.h ext2_helper.h
	gcc ${FLAGS} -c ext2_ls.c 


clean :
	rm *.o ext2_ls