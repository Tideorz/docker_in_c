objects = user cdocker
all: $(objects)
CC = gcc
CFLAGS=-Wall -g

user: user.o
	$(CC) -o $@ $^

cdocker: cdocker.o
	$(CC) -o $@ $^

.c.o:
	$(CC) -c $<
clean:
	rm -rf *.o cdocker user
