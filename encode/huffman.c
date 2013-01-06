#include "rbtree.h"
#include "huffman.h"
#include <fcntl.h>
#include <linux/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
static unsigned char char_table[260][32], save[32];
static int sz[260];
static unsigned long long int record[260] = {};

static void huffman_insert(struct hm_root *hm_root, struct hm_node *data)
{
    register struct rb_node **link = &hm_root->rank.rb_node, *parent = NULL;
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
static struct hm_node *base;
static void traverse(struct hm_node *node, int level)
{
    if (node->left == NULL && node->right == NULL) {
        int offset = node - base, i;
        sz[offset] = level;
        save[level++] = '\0';
        for (i = 0; i < level; i++)
            char_table[offset][i] = save[i];
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
extern void create_hm_tree(const char *input)
{
    /* variable initialize */
    unsigned char buf[BUFFERSIZE << 1];
    int nbytes = sizeof(buf),  bytes_read, index = 0, fd, i;
    struct hm_root *hm_root = &HM_ROOT;
    struct hm_node *node_table, *node;
    node_table = (struct hm_node *) malloc(NODESIZE * sizeof (struct hm_node));
    memset(record, 0, sizeof(record));

    /* open the file */
    if ((fd = open(input, O_RDONLY)) == -1) {
        fprintf(stderr, "Cannot open %s. Try again later.\n", input);
        exit(1);
    }

    /* count the frequency */
    while ((bytes_read = read(fd, buf, nbytes)) > 0) 
#pragma omp parallel for 
        for (i = 0; i < bytes_read; i++)
#pragma omp atomic
            ++record[(int) buf[i]];
    
    /* tree initialize */
    for (i = 0; i < 256; i++) {
        node = node_table + index++;
        if (record[i] == 0)
            continue;
        hm_init_node(node);
        node->frequency = record[i];
        huffman_insert(hm_root, node); 
    }
    /* build the tree */
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

    /* build char table */
    node = huffman_top(hm_root);
    base = node_table;
    traverse(node, 0);

    free(node_table);
    close(fd);
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
    int fip, i, fop;
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
        /* read the file, encode it, then write to pipe */
        unsigned char buf[BUFFERSIZE >> 2];
        unsigned char tmp[BUFFERSIZE << 2];
        int nbytes = sizeof(buf), bytes_read, idx = 0, j;
        close(fd[0]);
        while ((bytes_read = read(fip, buf, nbytes)) > 0) {
            idx = 0;
            for (i = 0; i < bytes_read; i++)
                for (j = 0; j < sz[(int)buf[i]]; j++)
                    tmp[idx++] = char_table[(int)buf[i]][j];
            write(fd[1], tmp, idx);
        }
    } else {
        /* set char to bit */
        close(fd[1]);
        unsigned char out[BUFFERSIZE], bit, tmp[BUFFERSIZE >> 2];
        int idx = 0, k = 0, bytes_read;
        while ((bytes_read = read(fd[0], out, BUFFERSIZE)) > 0) {

            idx = 0;
            /* take ride the left chars */
            if (k != 0) {
                for (;k < 8; k++)
                    bit |= (out[idx++] == '1' ? (1 << (7 - k)) : 0);
                write(fop, (unsigned char *) &bit, 1);
            }
            bytes_read -= 8;




            while (idx < bytes_read) {
                bit = 0;
                for (k = 0; k < 8; k++)
                    bit |= (out[idx + k] == '1' ? (1 << (7 - k)) : 0);
                idx += k;
                tmp[((idx - 1) >> 3)] = bit;
            }

//            int id;
//#pragma omp parallel for private(bit, k) num_threads(8)
//            for (id = idx; id < bytes_read; id += 8) {
//                bit = 0;
//                for (k = 0; k < 8; k++)
//                    bit |= (out[id + k] == '1' ? (1 << (7 - k)) : 0);
//                tmp[((id - 1) >> 3)] = bit;
//            }
//            printf("%d\n", ((7 + bytes_read - ((bytes_read - 1) & 7)) >> 3)); 

            write(fop, (unsigned char *) tmp, ((7 + bytes_read - ((bytes_read - 1) & 7)) >> 3)); 
            bit = 0;
            idx = 7 + bytes_read - ((bytes_read - 1) & 7);
            bytes_read += 8;
            for (k = 0; idx < bytes_read; k++)
                bit |= (out[idx++] == '1' ? (1 << (7 - k)) : 0);
        }
        if (k)
            write(fop, (unsigned char *) &bit, 1);
    }
    close(fip);
    close(fop);
}
