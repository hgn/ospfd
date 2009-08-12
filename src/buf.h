#ifndef BUF_H
#define BUF_H

#include <inttypes.h>

#ifndef min
# define min(x,y) ({			\
			typeof(x) _x = (x);		\
			typeof(y) _y = (y);		\
			(void) (&_x == &_y);	\
			_x < _y ? _x : _y; })
#endif

#ifndef max
# define max(x,y) ({			\
			typeof(x) _x = (x);		\
			typeof(y) _y = (y);		\
			(void) (&_x == &_y);	\
			_x > _y ? _x : _y; })
#endif

#define	BUF_DEFAULT_INIT_SIZE 128
#define BUF_MINIMUM_INCREASE_SIZE 128

struct buf {
	char *buffer;
	size_t len;
	size_t len_allocated;
};


struct buf* buf_alloc_hint(size_t);
struct buf* buf_alloc(void);
struct buf* buf_resize(struct buf *, const size_t);
void buf_free(struct buf *);
int buf_increase(struct buf *, size_t);
int buf_add(struct buf *, const char *, size_t);
void* buf_addr(const struct buf *);
void buf_set_addr(struct buf *, size_t);
int buf_copy(const struct buf *, struct buf *);
struct buf* buf_clone(const struct buf *);
size_t buf_len(const struct buf *);
size_t buf_len_allocated(const struct buf *);
void buf_mem_add_buf(char *, const struct buf *);

#endif /* BUF_H */
