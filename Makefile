objects = user cdocker test_cgroup_cpu test_cgroup_memory
all: $(objects)
CC = gcc
CFLAGS=-Wall -g

user: user.o
	$(CC) -o $@ $^

cdocker: cdocker.o
	$(CC) -o $@ $^

test_cgroup_cpu: test_cgroup_cpu.o
	$(CC) -o $@ $^

test_cgroup_memory: test_cgroup_memory.o
	$(CC) -o $@ $^

.c.o:
	$(CC) -c $<

clean:
	rm -rf *.o cdocker user
