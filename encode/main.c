#include <stdio.h>
#include "rbtree.h"
#include "huffman.h"
#include <stdlib.h>
int main(int argc, const char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "There most be tow files, input and outpus\n");
        exit(1);
    }
    create_hm_tree(argv[1]);
    encode(argv[1], argv[2]);
    return 0;
}
