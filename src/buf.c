#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "shared.h"
#include "buf.h"



struct buf* buf_alloc_hint(size_t size)
{
	struct buf *buf;

	buf = xzalloc(sizeof(struct buf));

	/* GNU libc malloc() always returns 8-byte aligned
	 * memory addresses, that should be sufficient */
	buf->buffer = xzalloc(size);
	if (!buf->buffer)
		return NULL;

	buf->len = 0;
	buf->len_allocated = size;

	return buf;
}

struct buf* buf_alloc(void)
{
	return buf_alloc_hint(BUF_DEFAULT_INIT_SIZE);
}

void buf_free(struct buf *buffer)
{
	if (!buffer || !buffer->buffer)
		return;

	free(buffer->buffer);
	free(buffer);
}

size_t buf_len(const struct buf *b)
{
	return b->len;
}

size_t buf_len_allocated(const struct buf *b)
{
	return b->len_allocated;
}

struct buf* buf_resize(struct buf *buffer, size_t new_size)
{
	size_t old_size = buffer->len;

	assert(new_size >= buffer->len_allocated);

	buffer->buffer = realloc(buffer->buffer, new_size);
	if (!buffer->buffer)
		return NULL;

	memset(&buffer->buffer[old_size], 0, new_size - old_size);

	buffer->len_allocated = new_size;

	return buffer;
}

int buf_increase(struct buf *buffer, size_t increase_size)
{
	return buf_increase(buffer, buffer->len_allocated + increase_size);
}

int buf_add(struct buf *buffer, const char *src, size_t len)
{
	size_t minimum_inc_size = BUF_MINIMUM_INCREASE_SIZE;

	assert(buffer->len_allocated >= buffer->len);

	if (buffer->len_allocated - buffer->len <= len) {
		size_t new_len = buffer->len_allocated + max(len, minimum_inc_size);
		if (!buf_resize(buffer, new_len)) {
			return -1;
		}
	}
	memcpy(&buffer->buffer[buffer->len], src, len);
	buffer->len += len;

	return 0;
}

void* buf_addr(const struct buf *buffer)
{
	return (void *)buffer->buffer;
}

void buf_set_addr(struct buf *buffer, size_t new_ptr)
{
	buffer->len = new_ptr;
}

int buf_copy(const struct buf *src, struct buf *dst)
{
	if (dst->len_allocated < src->len)
		return -1;

	memcpy(dst->buffer, src->buffer, src->len);
	dst->len = src->len;

	return 0;
}

struct buf* buf_clone(const struct buf *buffer)
{
	int ret;
	struct buf *new_buf;

	new_buf = buf_alloc_hint(buffer->len_allocated);
	if (!new_buf)
		return NULL;

	ret = buf_copy(buffer, new_buf);
	if (ret < 0) {
		buf_free(new_buf);
		return NULL;
	}

	return new_buf;
}

void buf_mem_add_buf(char *dst, const struct buf *src)
{
	memcpy(dst, buf_addr(src), buf_len(src));
}

/* vim:set ts=4 sw=4 sts=4 tw=78 ff=unix noet: */
