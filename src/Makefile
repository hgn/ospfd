OBJ=buf.o        \
		shared.o     \
		network.o    \
		rx.o         \
		rbtree.o     \
		hello_tx.o   \
		hello_rx.o   \
		cli_opt.o    \
		parser.tab.o \
		lex.yy.o     \
		rc.o         \
		clist.o      \
		interface.o  \
		neighbor.o   \
		ev.o         \
		ospfd.o

TARGET=ospfd

.SUFFIXES:
.SUFFIXES: .c .o

CC = gcc
LIBS = -lm -lrt
CFLAGS := -Wall -W -ggdb3 -pipe -Wextra -pipe -Wwrite-strings \
				 -Wsign-compare -Wshadow -Wformat=2 -Wundef -Wstrict-prototypes \
				 -fno-strict-aliasing -fno-common -Wformat-security \
				 -Wformat-y2k -Winit-self -Wpacked -Wredundant-decls \
				 -Wstrict-aliasing=3 -Wswitch-default -Wswitch-enum \
				 -Wno-system-headers -Wundef -Wvolatile-register-var \
				 -Wcast-align -Wbad-function-cast -Wwrite-strings \
				 -Wold-style-definition  -Wdeclaration-after-statement

CFLAGS += -DHAVE_EPOLL

all: $(TARGET)

lex.yy.c: lexer.l
	flex lexer.l

parser.tab.c: parser.y
	bison -d parser.y

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LIBS) -o $(TARGET) $(OBJ)

clean:
	-rm -f $(OBJ) $(TARGET) lex.yy.c parser.tab.h parser.tab.c *~ core

cscope:
	cscope -R -b
