OBJ=buf.o        \
		shared.o     \
		network.o    \
		ipv4_input.o \
		ospfd.o

TARGET=ospfd

.SUFFIXES:
.SUFFIXES: .c .o

CC = gcc
CFLAGS = -Wall -W -pipe -g -Wwrite-strings -Wsign-compare -Wshadow -Wformat=2
LIBS = -lm

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

all: $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

clean:
	-rm -f $(OBJ) $(TARGET) *~ core
