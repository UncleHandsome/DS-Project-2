CC=gcc
CFLAGS= -Wall -Wextra -O2
OBJS= huffman.o rbtree.o main.o
HDRS= huffman.h rbtree.h
TARGETS=encode

all: $(TARGETS)

$(TARGETS): $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

.PHONY: clean
clean:
	@rm -rf $(OBJS) $(TARGETS) *~
