OBJ=buf.o        \
		shared.o     \
		network.o    \
		ipv4_input.o \
		rbtree.o     \
		timer.o      \
		event.o      \
		hello.o      \
		cli_opt.o    \
		parser.tab.o \
		lex.yy.o     \
		rc.o         \
		ospfd.o

TARGET=ospfd

.SUFFIXES:
.SUFFIXES: .c .o

CC = gcc
CFLAGS = -Wall -W -pipe -g -Wwrite-strings -Wsign-compare -Wshadow -Wformat=2
LIBS = -lm -lrt

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
