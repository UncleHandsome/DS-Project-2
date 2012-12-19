#include <stdio.h>
#include "rbtree.h"
#include "huffman.h"
int main(int argc, const char *argv[])
{
    create_hm_tree(argv[1]);
    encode(argv[1], argv[2]);
    return 0;
}
