#include "rbtree.h"
#include <linux/stddef.h>

#define NODESIZE 1000
#define BUFFERSIZE 4096

struct hm_node {
    struct rb_node node;
    struct hm_node *left, *right;
    int frequency;
} __attribute__((aligned(sizeof(long long))));

struct hm_root {
    struct rb_root rank;
    struct rb_node *left_most;
}__attribute__((aligned(sizeof(long long))));

static inline int hm_compare(struct hm_node *a, struct hm_node *b)
{
    return a->frequency >= b->frequency;
}

#define HM_ROOT (struct hm_root) {RB_ROOT, NULL,}
#define HM_EMPTY_ROOT(root) (RB_EMPTY_ROOT(root.rank))
#define HM_EMPTY_NODE(node) (RB_EMPTY_NODE(node->node))

static inline void hm_init_node(struct hm_node *hm)
{
    rb_init_node(&hm->node);
    hm->frequency = 0;
    hm->left = hm->right = NULL;
}
extern void create_hm_tree(const char *);
extern void encode(const char *, const char *);
