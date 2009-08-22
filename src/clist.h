#ifndef CLIST_H
#define CLIST_H

struct list_e {
	void *data;
	struct list_e *next;
	struct list_e *prev;
};

struct list_e *list_create(void);
struct list_e *list_insert_before(struct list_e *before, void *data);
struct list_e *list_insert_after(struct list_e *after, void *data);
struct list_e *list_delete_before(struct list_e *before);
struct list_e *list_delete_after(struct list_e *after);
int list_delete(struct list_e *n, void (*cb)(void *data));
int list_for_each(struct list_e *n, void (*cb)(const void *data));
int list_for_each_with_arg(struct list_e *n, void (*cb)(void *d1, void *d2), void *data);
struct list_e *list_search(struct list_e *h, int (*cmp)(void *d1, void *d2), void *data);

#endif /* CLIST_H */
