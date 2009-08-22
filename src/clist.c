#include <stdio.h>
#include <stdlib.h>

#include "clist.h"
#include "ospfd.h"
#include "shared.h"


struct list_e *list_create(void)
{
	return NULL;
}

struct list_e *list_insert_before(struct list_e *before, void *data)
{
	struct list_e *n;

	n = (struct list_e *) xzalloc(sizeof(struct list_e));
	n->data = data;

	if (!before) {
		n->prev = n;
		n->next = n;
		return n;
	}

	n->prev = before->prev;
	n->next = before;

	before->prev->next = n;
	before->prev = n;

	return n;
}

struct list_e *list_insert_after(struct list_e *after, void *data)
{
	struct list_e *n;

	n = (struct list_e *) xzalloc(sizeof(struct list_e));
	n->data = data;

	if (!after) {
		n->prev = n;
		n->next = n;
		return n;
	}

	n->prev = after;
	n->next = after->next;
	if (after->prev) {
		after->next->prev = n;
	}
	after->next = n;

	return n;
}

struct list_e *list_delete_before(struct list_e *before)
{
	struct list_e* tmp;

	tmp = before->prev;
	before->prev = tmp->prev;
	if (tmp->prev) {
		tmp->prev->next = before;
	}
	free(tmp);

	return before->prev;
}

struct list_e *list_delete_after(struct list_e *after)
{
	struct list_e* tmp;

	tmp = after->next;

	after->next = tmp->next;
	tmp->next->prev = after;
	free(tmp);

	return after->next;

}

int list_delete(struct list_e *n, void (*cb)(void *data))
{
	void *taddr = n;
	struct list_e *tmp;
	struct list_e* after = n;

	if (n == NULL)
		return 0;

	do {
		tmp = after->next;
		cb(after->data); /* user supplied free routine */
		free(after);
		after = tmp;
	} while (after && (taddr != after));

	return 0;
}

int list_for_each(struct list_e *n, void (*cb)(const void *data))
{
	void *taddr = n;
	struct list_e *tmp;
	struct list_e *after = n;

	while (after) {
		tmp = after->next;
		cb(after->data); /* call callback handler */
		free(after);
		after = tmp;
		if (after == taddr)
			break;
	}

	return 0;
}

int list_for_each_with_arg(struct list_e *n, void (*cb)(void *d1, void *d2), void *data2)
{
	void *taddr = n;
	struct list_e *tmp;
	struct list_e *after = n;

	while (after) {
		tmp = after->next;
		cb(after->data, data2); /* call callback handler */
		free(after);
		after = tmp;
		if (after == taddr)
			break;
	}

	return 0;
}


struct list_e *list_search(struct list_e *h,
		int (*cmp)(void *d1, void *d2), void *data)
{
	struct list_e* after;

	if (h == NULL)
		return NULL;

	after = h;

	do {
		if (cmp(after->data, data)) {
			return after;
		}
		after = after->next;
	} while (after && (after != h));

	return NULL;
}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
