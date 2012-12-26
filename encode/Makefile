CC=gcc
CFLAGS=-finline-functions -Wall -Wextra -O2

.SUFFIXS: .c
encode: main.o huffman.o rbtree.o
		${CC} main.o huffman.o rbtree.o ${CFLAGS} -o $@
%.o: %.c rbtree.h
	${CC} $< ${CFLAGS} -c
%.o: %.c huffman.h
	${CC} $< ${CFLAGS} -c
.PHONY: clean
clean:
	@rm -rf *.o output encode
