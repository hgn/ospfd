#ifndef RBTREE_H
#define	RBTREE_H


enum rbtree_color { RED, BLACK };

typedef int (*rbtree_compare_func)(void* left_key, void* right_key);

struct rbtree_node {
    struct rbtree_node* left;
    struct rbtree_node* right;
    struct rbtree_node* parent;
    enum rbtree_color color;
    void* key;
    void* data;
};

struct rbtree {
    struct rbtree_node * root;
    rbtree_compare_func compare;
	size_t size;
};


void rbtree_init(struct rbtree *, int (*cmp)(void *, void *));
void rbtree_init_int(struct rbtree *);
void rbtree_init_double(struct rbtree *);
void *rbtree_lookup(struct rbtree *, void *);
void rbtree_insert(struct rbtree *, struct rbtree_node *);
struct rbtree_node * rbtree_delete(struct rbtree *, void *);
size_t rbtree_size(struct rbtree *);
struct rbtree_node *rbtree_node_alloc(void);
void rbtree_node_free(struct rbtree_node *);

#endif /* RBTREE_H */

/* vim:set ts=4 sw=4 sts=4 tw=78 ff=unix noet: */
