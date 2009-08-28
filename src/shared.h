#ifndef SHARED_H
#define	SHARED_H

#include <inttypes.h>
#include <netdb.h>
#include <sys/socket.h>

#include "ospfd.h"

/* error handling */
#define err_msg(format, args...) \
        do { \
                x_err_ret(__FILE__, __LINE__,  format , ## args); \
        } while (0)

#define err_sys(format, args...) \
        do { \
                x_err_sys(__FILE__, __LINE__,  format , ## args); \
        } while (0)

#define err_sys_die(exitcode, format, args...) \
        do { \
                x_err_sys(__FILE__, __LINE__, format , ## args); \
                exit(exitcode); \
        } while (0)

#define err_msg_die(exitcode, format, args...) \
        do { \
                x_err_ret(__FILE__, __LINE__,  format , ## args); \
                exit(exitcode); \
        } while (0)

void x_err_ret(const char *file, int line_no, const char *, ...);
void x_err_sys(const char *file, int line_no, const char *, ...);
void *xmalloc(size_t len);
void *xzalloc(size_t len);
char * xstrdup(const char *);
void xsetsockopt(int, int, int, const void *, socklen_t, const char *);
int xsnprintf(char *, size_t , const char *, ...);
unsigned long long xstrtoull(const char *);
int xatoi(const char *);
void *get_in_addr(struct sockaddr *);
void msg(struct ospfd *, const int , const char *, ...);

#endif /* SHARED_H */

/* vim:set ts=4 sw=4 sts=4 tw=78 ff=unix noet: */
