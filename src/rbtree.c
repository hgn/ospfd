#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>

#include "ospfd.h"
#include "shared.h"
#include "rbtree.h"


static struct rbtree_node* sibling(struct rbtree_node* n);
static struct rbtree_node* uncle(struct rbtree_node* n);
static enum rbtree_color node_color(struct rbtree_node* n);

static struct rbtree_node *lookup_node(struct rbtree *, void *);
static void rotate_left(struct rbtree *, struct rbtree_node *);
static void rotate_right(struct rbtree *, struct rbtree_node *);

static struct rbtree_node *maximum_node(struct rbtree_node *);
static void replace_node(struct rbtree *, struct rbtree_node *, struct rbtree_node*);
static void insert_case1(struct rbtree *, struct rbtree_node *);
static void insert_case2(struct rbtree *, struct rbtree_node *);
static void insert_case3(struct rbtree *, struct rbtree_node *);
static void insert_case4(struct rbtree *, struct rbtree_node *);
static void insert_case5(struct rbtree *, struct rbtree_node *);
static void delete_case1(struct rbtree *, struct rbtree_node *);
static void delete_case2(struct rbtree *, struct rbtree_node *);
static void delete_case3(struct rbtree *, struct rbtree_node *);
static void delete_case4(struct rbtree *, struct rbtree_node *);
static void delete_case5(struct rbtree *, struct rbtree_node *);
static void delete_case6(struct rbtree *, struct rbtree_node *);

size_t rbtree_size(struct rbtree *tree)
{
	return tree->size;
}

struct rbtree_node *rbtree_node_alloc(void)
{
	return malloc(sizeof(struct rbtree_node));
}

void rbtree_node_free(struct rbtree_node *n)
{
	free(n);
}

static struct rbtree_node* grandparent(struct rbtree_node* n) {
    assert (n != NULL);
    assert (n->parent != NULL); /* Not the root struct rbtree_node* */
    assert (n->parent->parent != NULL); /* Not child of root */
    return n->parent->parent;
}

struct rbtree_node* sibling(struct rbtree_node* n) {
    assert (n != NULL);
    assert (n->parent != NULL); /* Root struct rbtree_node* has no sibling */
    if (n == n->parent->left)
        return n->parent->right;
    else
        return n->parent->left;
}

struct rbtree_node* uncle(struct rbtree_node* n) {
    assert (n != NULL);
    assert (n->parent != NULL); /* Root struct rbtree_node* has no uncle */
    assert (n->parent->parent != NULL); /* Children of root have no uncle */
    return sibling(n->parent);
}



enum rbtree_color node_color(struct rbtree_node* n) {
    return n == NULL ? BLACK : n->color;
}


void rbtree_init(struct rbtree* tree, rbtree_compare_func compare) {
    tree->root    = NULL;
    tree->compare = compare;
	tree->size    = 0;
}

static int compare_int(void *l, void *r) {
     uintptr_t left  = (uintptr_t) l;
     uintptr_t right = (uintptr_t) r;
     if (left < right)
         return -1;
     else if (left > right)
         return 1;
     else {
         assert (left == right);
         return 0;
     }
 }

void rbtree_init_int(struct rbtree* tree)
{
	return rbtree_init(tree, compare_int);
}


static int compare_double(void *leftp, void *rightp) {
     double *left  = (double *)leftp;
     double *right = (double *)rightp;
     if (*left < *right)
         return -1;
     else if (*left > *right)
         return 1;
     else {
         assert (*left == *right);
         return 0;
     }
 }

void rbtree_init_double(struct rbtree* tree)
{
	return rbtree_init(tree, compare_double);
}


struct rbtree_node* lookup_node(struct rbtree* t, void* key) {
    struct rbtree_node* n = t->root;
    while (n != NULL) {
        int comp_result = t->compare(key, n->key);
        if (comp_result == 0) {
            return n;
        } else if (comp_result < 0) {
            n = n->left;
        } else {
            assert(comp_result > 0);
            n = n->right;
        }
    }
    return n;
}


void* rbtree_lookup(struct rbtree* t, void* key) {
    struct rbtree_node* n = lookup_node(t, key);
    return n == NULL ? NULL : n->data;
}


void rotate_left(struct rbtree* t, struct rbtree_node* n) {
    struct rbtree_node* r = n->right;
    replace_node(t, n, r);
    n->right = r->left;
    if (r->left != NULL) {
        r->left->parent = n;
    }
    r->left = n;
    n->parent = r;
}


void rotate_right(struct rbtree* t, struct rbtree_node* n) {
    struct rbtree_node* L = n->left;
    replace_node(t, n, L);
    n->left = L->right;
    if (L->right != NULL) {
        L->right->parent = n;
    }
    L->right = n;
    n->parent = L;
}


void replace_node(struct rbtree* t, struct rbtree_node* oldn, struct rbtree_node* newn) {
    if (oldn->parent == NULL) {
        t->root = newn;
    } else {
        if (oldn == oldn->parent->left)
            oldn->parent->left = newn;
        else
            oldn->parent->right = newn;
    }
    if (newn != NULL) {
        newn->parent = oldn->parent;
    }
}


void rbtree_insert(struct rbtree* t, struct rbtree_node *inserted_node) {

    inserted_node->color  = RED;
    inserted_node->left   = NULL;
    inserted_node->right  = NULL;
    inserted_node->parent = NULL;

    if (t->root == NULL) {
        t->root = inserted_node;
    } else {
        struct rbtree_node* n = t->root;
        while (1) {
            int comp_result = t->compare(inserted_node->key, n->key);
            if (comp_result == 0) {
				/* duplicate */
                n->data = inserted_node->data;
                return;
            } else if (comp_result < 0) {
                if (n->left == NULL) {
                    n->left = inserted_node;
                    break;
                } else {
                    n = n->left;
                }
            } else {
                assert (comp_result > 0);
                if (n->right == NULL) {
                    n->right = inserted_node;
                    break;
                } else {
                    n = n->right;
                }
            }
        }
        inserted_node->parent = n;
    }
	t->size++;
    insert_case1(t, inserted_node);
}


void insert_case1(struct rbtree* t, struct rbtree_node* n) {
    if (n->parent == NULL)
        n->color = BLACK;
    else
        insert_case2(t, n);
}


void insert_case2(struct rbtree* t, struct rbtree_node* n) {
    if (node_color(n->parent) == BLACK)
        return;
    else
        insert_case3(t, n);
}


void insert_case3(struct rbtree* t, struct rbtree_node* n) {
    if (node_color(uncle(n)) == RED) {
        n->parent->color = BLACK;
        uncle(n)->color = BLACK;
        grandparent(n)->color = RED;
        insert_case1(t, grandparent(n));
    } else {
        insert_case4(t, n);
    }
}


void insert_case4(struct rbtree* t, struct rbtree_node* n) {
    if (n == n->parent->right && n->parent == grandparent(n)->left) {
        rotate_left(t, n->parent);
        n = n->left;
    } else if (n == n->parent->left && n->parent == grandparent(n)->right) {
        rotate_right(t, n->parent);
        n = n->right;
    }
    insert_case5(t, n);
}


void insert_case5(struct rbtree* t, struct rbtree_node* n) {
    n->parent->color = BLACK;
    grandparent(n)->color = RED;
    if (n == n->parent->left && n->parent == grandparent(n)->left) {
        rotate_right(t, grandparent(n));
    } else {
        assert (n == n->parent->right && n->parent == grandparent(n)->right);
        rotate_left(t, grandparent(n));
    }
}


struct rbtree_node *rbtree_delete(struct rbtree* t, void* key) {
    struct rbtree_node* child;
    struct rbtree_node* n = lookup_node(t, key);
    if (n == NULL) return NULL; /* Key not found, do nothing */
    if (n->left != NULL && n->right != NULL) {
        /* Copy key/data from predecessor and then delete it instead */
        struct rbtree_node* pred = maximum_node(n->left);
        n->key = pred->key;
        n->data = pred->data;
        n = pred;
    }

    assert(n->left == NULL || n->right == NULL);
    child = n->right == NULL ? n->left : n->right;
    if (node_color(n) == BLACK) {
        n->color = node_color(child);
        delete_case1(t, n);
    }
    replace_node(t, n, child);

    return n;
}


static struct rbtree_node* maximum_node(struct rbtree_node* n) {
    assert (n != NULL);
    while (n->right != NULL) {
        n = n->right;
    }
    return n;
}


void delete_case1(struct rbtree* t, struct rbtree_node* n) {
    if (n->parent == NULL)
        return;
    else
        delete_case2(t, n);
}


void delete_case2(struct rbtree* t, struct rbtree_node* n) {
    if (node_color(sibling(n)) == RED) {
        n->parent->color = RED;
        sibling(n)->color = BLACK;
        if (n == n->parent->left)
            rotate_left(t, n->parent);
        else
            rotate_right(t, n->parent);
    }
    delete_case3(t, n);
}


void delete_case3(struct rbtree* t, struct rbtree_node* n) {
    if (node_color(n->parent) == BLACK &&
        node_color(sibling(n)) == BLACK &&
        node_color(sibling(n)->left) == BLACK &&
        node_color(sibling(n)->right) == BLACK)
    {
        sibling(n)->color = RED;
        delete_case1(t, n->parent);
    }
    else
        delete_case4(t, n);
}


void delete_case4(struct rbtree* t, struct rbtree_node* n) {
    if (node_color(n->parent) == RED &&
        node_color(sibling(n)) == BLACK &&
        node_color(sibling(n)->left) == BLACK &&
        node_color(sibling(n)->right) == BLACK)
    {
        sibling(n)->color = RED;
        n->parent->color = BLACK;
    }
    else
        delete_case5(t, n);
}


void delete_case5(struct rbtree* t, struct rbtree_node* n) {
    if (n == n->parent->left &&
        node_color(sibling(n)) == BLACK &&
        node_color(sibling(n)->left) == RED &&
        node_color(sibling(n)->right) == BLACK)
    {
        sibling(n)->color = RED;
        sibling(n)->left->color = BLACK;
        rotate_right(t, sibling(n));
    }
    else if (n == n->parent->right &&
             node_color(sibling(n)) == BLACK &&
             node_color(sibling(n)->right) == RED &&
             node_color(sibling(n)->left) == BLACK)
    {
        sibling(n)->color = RED;
        sibling(n)->right->color = BLACK;
        rotate_left(t, sibling(n));
    }
    delete_case6(t, n);
}


void delete_case6(struct rbtree* t, struct rbtree_node* n) {
    sibling(n)->color = node_color(n->parent);
    n->parent->color = BLACK;
    if (n == n->parent->left) {
        assert (node_color(sibling(n)->right) == RED);
        sibling(n)->right->color = BLACK;
        rotate_left(t, n->parent);
    }
    else
    {
        assert (node_color(sibling(n)->left) == RED);
        sibling(n)->left->color = BLACK;
        rotate_right(t, n->parent);
    }
}



/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
