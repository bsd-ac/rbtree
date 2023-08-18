#include <stddef.h>

struct avl_node {
        struct avl_node *child[3];
        short unsigned int rdiff[2];
        int key;
};

struct avl_tree {
        struct avl_node *root;
};

void avl_init(struct avl_tree *tree) {
        tree->root = NULL;
}

void avl_rotate(struct avl_node *node, struct avl_node *parent, int dir, int parent_dir) {
        struct avl_node *child = node->child[!dir];
        struct avl_node *grandchild = child->child[dir];
        int rdiff = node->rdiff[!dir] - child->rdiff[dir];
        node->child[!dir] = grandchild;
        node->rdiff[!dir] = rdiff - child->rdiff[dir];
        child->child[dir] = node;
        child->rdiff[dir] = rdiff;
        if (parent) {
                parent->child[parent->child[1] == node] = child;
        }
}