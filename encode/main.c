#include <stdio.h>
#include "rbtree.h"
#include "huffman.h"
int main(int argc, const char *argv[])
{
    struct hm_root hmr = HM_ROOT;
    create_hm_tree(argv[1], &hmr);
    generate_code_table(&hmr);
    encode(argv[1], argv[2]);
    return 0;
}
