OBJ=buf.o        \
		shared.o     \
		network.o    \
		ipv4_input.o \
		rbtree.o     \
		timer.o      \
		event.o      \
		hello.o      \
		cli_opt.o  \
		ospfd.o

TARGET=ospfd

.SUFFIXES:
.SUFFIXES: .c .o

CC = gcc
CFLAGS = -Wall -W -pipe -g -Wwrite-strings -Wsign-compare -Wshadow -Wformat=2
LIBS = -lm -lrt

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

all: $(OBJ)
	$(CC) $(CFLAGS) $(LIBS) -o $(TARGET) $(OBJ)

clean:
	-rm -f $(OBJ) $(TARGET) *~ core
