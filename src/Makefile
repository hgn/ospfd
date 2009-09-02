OBJ=buf.o        \
		shared.o     \
		network.o    \
		rx.o \
		rbtree.o     \
		timer.o      \
		event.o      \
		hello_tx.o   \
		cli_opt.o    \
		parser.tab.o \
		lex.yy.o     \
		rc.o         \
		clist.o      \
		interface.o  \
		ospfd.o

TARGET=ospfd

.SUFFIXES:
.SUFFIXES: .c .o

CC = gcc
LIBS = -lm -lrt
CFLAGS = -Wall -W -pipe -g -Wwrite-strings -Wsign-compare \
				 -Wshadow -Wformat=2 -Wundef -Wstrict-prototypes \
				 -fno-strict-aliasing -fno-common -fomit-frame-pointer

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
