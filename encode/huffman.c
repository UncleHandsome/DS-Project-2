#include "rbtree.h"
#include "huffman.h"
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
static unsigned char char_table[260][32], save[32];
static int sz[260];
static struct hm_node *node_table;
static unsigned long long int record[260] = {};

static void huffman_insert(struct hm_root *hm_root, struct hm_node *data)
{
    struct rb_node **link = &hm_root->rank.rb_node, *parent = NULL;
    struct hm_node *node;
    int leftmost = 1;
    while (*link) {
        parent = *link;
        node = rb_entry(parent, struct hm_node, node);
        if (hm_compare(node, data))
            link = &parent->rb_left;
        else {
            link = &parent->rb_right;
            leftmost = 0;
        }
    }
    if (leftmost)
       hm_root->left_most = &data->node; 
    rb_link_node(&data->node, parent, link);
    rb_insert_color(&data->node, &hm_root->rank);
}

static void huffman_pop(struct hm_root *hm_root)
{
    struct rb_node *next_node;
    struct hm_node *node = rb_entry(hm_root->left_most, struct hm_node, node);
    next_node = rb_next(hm_root->left_most);
    hm_root->left_most = next_node;
    rb_erase(&node->node, &hm_root->rank);
}
static inline struct hm_node *huffman_top(struct hm_root *hm_root)
{
    return rb_entry(hm_root->left_most, struct hm_node, node);
}
extern void create_hm_tree(const char *input, struct hm_root *hm_root)
{
    int fd, i;
    if ((fd = open(input, O_RDONLY)) == -1) {
        fprintf(stderr, "Cannot open %s. Try again later.\n", input);
        exit(1);
    }
    unsigned char buf[BUFFERSIZE];
    int nbytes = sizeof(buf);
    int bytes_read;

    int index = 0;
    node_table = (struct hm_node *) malloc(NODESIZE * sizeof (struct hm_node));

    memset(record, 0, sizeof(record));
    while ((bytes_read = read(fd, buf, nbytes)) > 0) 
        for (i = 0; i < bytes_read; i++)
            ++record[(int) buf[i]];
    
    for (i = 0; i < 256; i++) {
        struct hm_node *node = node_table + index++;
        if (record[i] == 0)
            continue;
        hm_init_node(node);
        node->frequency = record[i];
        huffman_insert(hm_root, node); 
    }
    while (hm_root->rank.rb_node->rb_left != NULL || hm_root->rank.rb_node->rb_right != NULL) {

        struct hm_node *first = huffman_top(hm_root);
        huffman_pop(hm_root);
        struct hm_node *second = huffman_top(hm_root);
        huffman_pop(hm_root);

        struct hm_node *new = node_table + index++;
        hm_init_node(new);
        new->frequency = first->frequency + second->frequency;
        new->left = first;
        new->right = second;
        huffman_insert(hm_root, new);
    }
    close(fd);
}
static void traverse(struct hm_node *node, int level)
{
    if (node->left == NULL && node->right == NULL) {
        int off = node - node_table, i;
        sz[off] = level;
        save[level++] = '\0';
        for (i = 0; i < level; i++)
            char_table[off][i] = save[i];
        return ;
    }
    if (node->left) {
        save[level] = '0';
        traverse(node->left, level + 1);
    }
    if (node->right) {
        save[level] = '1';
        traverse(node->right, level + 1);
    }
}
inline void generate_code_table(struct hm_root *root)
{
    /* there is only "one" node in the Red-Black Tree */
    struct hm_node *node = huffman_top(root);
    traverse(node, 0);
}
static long long int encode_count()
{
    long long int count = 0;
    int i;
    for (i = 0; i < 256; i++)
        count += record[i] * sz[i];
    return count;
}
void encode(const char *input, const char *output)
{
    int fip, i, count, fop;
    /* open file */
    if ((fip = open(input, O_RDONLY)) == -1) {
        fprintf(stderr, "Cannot open %s. Try again later.\n", input);
        exit(1);
    }
    if ((fop = open(output, O_CREAT | O_RDWR, 0x0744)) == -1) {
        fprintf(stderr, "Cannot open %s. Try again later.\n", output);
        exit(1);
    }
    /* write header */
    write(fop, "###\n", 4);
    dprintf(fop, "%lld\n", encode_count());
    write(fop, "###\n", 4);
    for (i = 0; i < 256; i++)
        if (char_table[i][0] != '\0')
            dprintf(fop, "%c:%s\n", i, char_table[i]);
    write(fop, "###\n", 4);
    /* start encode */
    int fd[2];
    pid_t pid;
    if (pipe(fd) < 0) {
        perror("pipe");
        exit(1);
    }
    if ((pid = fork()) < 0) {
        perror("fork");
        exit(1);
    }
    if (pid > 0) {
        unsigned char buf[BUFFERSIZE >> 2];
        unsigned char tmp[BUFFERSIZE << 2];
        int nbytes = sizeof(buf);
        int bytes_read, idx = 0, j;
        close(fd[0]);
        while ((bytes_read = read(fip, buf, nbytes)) > 0) {
            idx = 0;
            for (i = 0; i < bytes_read; i++)
                for (j = 0; j < sz[(int)buf[i]]; j++)
                    tmp[idx++] = char_table[(int)buf[i]][j];
            write(fd[1], tmp, idx);
        }
    } else {
        close(fd[1]);
        unsigned char out[4096], bit, tmp[1024];
        int idx = 0, k = 0;
        int bytes_read;
        while ((bytes_read = read(fd[0], out, 4096)) > 0) {
            idx = count = 0;
            /* take ride the left chars */
            if (k != 0) {
                for (;k < 8; k++)
                    bit |= (out[idx++] == '1' ? (1 << (7 - k)) : 0);
                tmp[count++] = bit;
            }
            bytes_read -= 8;
            while (idx < bytes_read) {
                bit = 0;
                /* set bit */
                for (k = 0; k < 8; k++)
                    bit |= (out[idx++] == '1' ? (1 << (7 - k)) : 0);
                tmp[count++] = bit;
            }
            write(fop, (unsigned char *) &tmp[0], count); 
            bit = 0;
            bytes_read += 8;
            for (k = 0; idx < bytes_read; k++)
                bit |= (out[idx++] == '1' ? (1 << (7 - k)) : 0);
        }
        if (idx % 8 != 0) {
            tmp[count = 0] = bit;
            write(fop, tmp, 1);
        }

    }
    close(fip);
    close(fop);
}
