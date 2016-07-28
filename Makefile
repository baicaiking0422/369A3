FLAGS = -Wall -g -std=c99
all : ext2_ls ext2_mkdir ext2_cp

ext2_ls: ext2_ls.o ext2_helper.o
	gcc ${FLAGS} -o ext2_ls ext2_ls.o ext2_helper.o

ext2_mkdir: ext2_mkdir.o ext2_helper.o
	gcc ${FLAGS} -o ext2_mkdir ext2_mkdir.o ext2_helper.o

ext2_cp: ext2_cp.o ext2_helper.o
	gcc ${FLAGS} -o ext2_cp ext2_cp.o ext2_helper.o

ext2_helper.o : ext2_helper.c ext2_helper.h ext2.h
	gcc ${FLAGS} -c ext2_helper.c

ext2_ls.o : ext2_ls.c ext2.h ext2_helper.h
	gcc ${FLAGS} -c ext2_ls.c

ext2_mkdir.o : ext2_mkdir.c ext2.h ext2_helper.h
	gcc ${FLAGS} -c ext2_mkdir.c

ext2_cp.o : ext2_cp.c ext2.h ext2_helper.h
	gcc ${FLAGS} -c ext2_cp.c

clean :
	rm *.o ext2_ls
