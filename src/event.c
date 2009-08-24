/* An epoll based event library with timer support */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "ospfd.h"
#include "event.h"
#include "shared.h"

struct ev_data {
	int fd;
	void (*cb)(int, void *);
	void *data;
	uint32_t flags;
};

int ev_init(struct ospfd *ospfd)
{
	ospfd->ev.fd = epoll_create(EVENT_BACKING_STORE_HINT);
	if (ospfd->ev.fd < 0) {
		err_sys("failed to create epoll file descriptor");
		return FAILURE;
	}

	ospfd->ev.size = 0;

	return SUCCESS;
}

int ev_del(struct ospfd *ospfd, int fd)
{
	int ret;
	struct epoll_event ev; /* catch kernel bug before 2.6.9 */

	memset(&ev, 0, sizeof(struct epoll_event));

	ret = epoll_ctl(ospfd->ev.fd, EPOLL_CTL_DEL, fd, &ev);
	if (ret < 0) {
		err_sys("failed to remove filedescriptor from epolls event pool");
		return FAILURE;
	}

	ospfd->ev.size--;

	return SUCCESS;
}

int ev_add(struct ospfd *ospfd, int fd,
		void (*cb)(int fd, void *data), void *data, uint32_t flags, int32_t event_flags)
{
	int ret = SUCCESS;
	struct ev_data *evd;
	struct epoll_event ev;

	evd = xzalloc(sizeof(struct ev_data));

	evd->fd    = fd;
	evd->cb    = cb;
	evd->data  = data;
	evd->flags = event_flags;

	/* fill event structure */
	ev.events   = flags;
	ev.data.ptr = evd;

	ret = epoll_ctl(ospfd->ev.fd, EPOLL_CTL_ADD, fd, &ev);
	if (ret < 0) {
		err_sys("failed to add filedescriptor to epolls event pool");
		return FAILURE;
	}

	ospfd->ev.size++;

	return ret;
}

int ev_loop(struct ospfd *ospfd)
{
	int ret = SUCCESS, nfds, i;
	struct epoll_event events[EVENT_ARRAY_SIZE];

	while (23) {
		nfds = epoll_wait(ospfd->ev.fd, events, EVENT_ARRAY_SIZE, -1);
		if (nfds < 0) {
			err_sys("call to epoll_wait failed");
			return FAILURE;
		}

		/* multiplex and call the registerd callback handler */
		for (i = 0; i < nfds; i++) {
			struct ev_data *ev_data = (struct ev_data *)events[i].data.ptr;
			if (ev_data->fd == 0) { // DEBUG
				fprintf(stderr,"ERROR: see FD == 0 at ev_loop()\n");
			}
			ev_data->cb(ev_data->fd, ev_data->data);
			/* TODO: if this event is only added one time we delete
			 * the wrapper structure here - of not than we delete
			 * it not here. This is not clean! Normally you must
			 * make the wrapper known to the environment and let
			 * the callee decide of this structure must be deleted */
			if (ev_data->flags == EVENT_ONE_SHOT)
				free(ev_data);
		}
	}

	return ret;
}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
