#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUFFERSIZE 4096
#define NODESIZE 700
typedef struct TrieNode {
    struct TrieNode *child[2];
    int end;
} node_t __attribute__((aligned(sizeof(long))));
node_t *trie_table, *prev;
int trie_index, pos, limit;
void trie_init()
{
    trie_index = 0;
    trie_table = (node_t *)malloc(sizeof(node_t) * NODESIZE);
    int i;
    for (i = 0; i < NODESIZE; i++)
        (*(trie_table + i)).end = -1;
}
void add_string(const unsigned char *word, node_t *root, unsigned char end)
{
    register unsigned char c;
    register int off;
    while ((c = *word++) && c != '\n') {
        off = c - '0';
        if (root->child[off] == NULL)
            root->child[off] = trie_table + trie_index++;
        root = root->child[off];
    }
    root->end = end;
}
int lookup(const int *word, node_t *root)
{
    register int off;
    while (pos < limit) {
        if (root->end != -1)  
            return root->end;
        off = word[pos++];
        root = root->child[off];
    }
    prev = root;
    return -1;
}
/* read header and build the tree */
long long int bit_count;
void build_tree(node_t *root, FILE *fp)
{    
    unsigned char buf[100], c;
    fgets(buf, 100, fp);
    fscanf(fp, "%lld", &bit_count);
    fgets(buf, 100, fp);
    fgets(buf, 100, fp);
    while(1) {
        fgets(buf, 100, fp);
        if(buf[0] == '#' && buf[1] == '#')
            break;
        int id = 2;
        if (buf[0] == '\n') {
            c = '\n';
            fgets(buf, 100, fp);
            id = 1;
        }
        else 
            c = buf[0];
        add_string(buf + id, root, c);
    }
}
void decode(const char *filename, const char *output)
{
    FILE *fp, *fo;
    trie_init();
    node_t *root = trie_table + trie_index++;
    int idx, bytes_read, bit, out[BUFFERSIZE], c;
    unsigned char buf[BUFFERSIZE >> 3];
    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "Cannot open %s. Try again later.\n", filename);
        exit(1);
    }
    if ((fo = fopen(output, "w+")) == NULL) {
        fprintf(stderr, "Cannot open %s. Try again later.\n", output);
        exit(1);
    }
    build_tree(root, fp);
    prev = root;
    while ((bytes_read = fread(buf, 1, BUFFERSIZE >> 3, fp)) > 0) {
        for (idx = 0; idx < bytes_read; idx++)
            for (bit = 0; bit < 8; bit++) 
                out[(idx << 3) | bit] = ((buf[idx] >> (7 - bit)) & 1);
        pos = 0;
        limit = (bytes_read << 3) + 0;
        bit_count -= limit;
        if (prev != root) {
            fputc(lookup(out, prev), fo);
            prev = root;
        }
        while ((c = lookup(out, root)) != -1) 
            fputc(c, fo);
    }
    limit += bytes_read + 1;
    if (prev != root) {
        fputc(lookup(out, prev), fo);
        prev = root;
    }
    while ((c = lookup(out, root)) != -1) 
            fputc(c, fo);

    free(trie_table);
    fclose(fp);
    fclose(fo);
}
int main(int argc, char *argv[])
{
    decode(argv[1], argv[2]);
    return 0;
}
