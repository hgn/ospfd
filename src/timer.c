#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <time.h>
#include <sys/epoll.h>

#include "shared.h"
#include "event.h"
#include "timer.h"


int timer_add_ns_rel(struct ospfd *ospfd, time_t sec, long nsec,
		void (*cb)(int fd, void *data), void *data)
{
	int ret, fd;
	struct timespec now;
	struct itimerspec new_value;
	uint32_t flags;

	/* create timer fd object */
	ret = clock_gettime(CLOCK_REALTIME, &now);
	if (ret < 0) {
		err_sys("failed to determine current time via clock_gettime(2)");
		return FAILURE;
	}

	new_value.it_value.tv_sec = now.tv_sec + sec;
	new_value.it_value.tv_nsec = now.tv_nsec + nsec;

	new_value.it_interval.tv_sec = 0;
	new_value.it_interval.tv_nsec = 0;

	fd = timerfd_create(CLOCK_REALTIME, 0);
	if (fd == -1) {
		err_sys("failed to determine current time via clock_gettime(2)");
		return FAILURE;
	}

	ret = timerfd_settime(fd, TFD_TIMER_ABSTIME, &new_value, NULL);
	if (ret < 0) {
		err_sys("failed to determine current time via clock_gettime(2)");
		return FAILURE;
	}

	/* and add to event mechanism to monitor for it */
	flags = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;

	ret = ev_add(ospfd, fd, cb, data, flags);
	if (ret < 0) {
		err_msg("failed to add timer fd to event based monitoring system");
		return FAILURE;
	}

	return SUCCESS;
}


int timer_add_s_rel(struct ospfd *ospfd, unsigned int seconds,
		void (*cb)(int fd, void *data), void *data)
{
	return timer_add_ns_rel(ospfd, seconds, 0, cb, data);
}




/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
