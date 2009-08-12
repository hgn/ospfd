#ifndef OSPFD_H
#define	OSPFD_H

#include <inttypes.h>
#include <netdb.h>
#include <sys/socket.h>

#define min(x,y) ({                     \
        typeof(x) _x = (x);             \
        typeof(y) _y = (y);             \
        (void) (&_x == &_y);    \
        _x < _y ? _x : _y; })

#define max(x,y) ({                     \
        typeof(x) _x = (x);             \
        typeof(y) _y = (y);             \
        (void) (&_x == &_y);    \
        _x > _y ? _x : _y; })

#if !defined likely && !defined unlikely
# define likely(x)   __builtin_expect(!!(x), 1)
# define unlikely(x) __builtin_expect(!!(x), 0)
#endif

/* function return codes */
#define SUCCESS 0
#define FAILURE -1

/* program error codes */
#define	EXIT_NETWORK 2



#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

struct opts {
	char *me;
	sa_family_t family;
};

struct network {
	int fd;
};


struct ospfd {
	struct opts opts;
	struct network network;
};


#endif /* OSPFD */

/* vim:set ts=4 sw=4 sts=4 tw=78 ff=unix noet: */
