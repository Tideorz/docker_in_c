CC = gcc

cdocker: cdocker.o
	$(CC) -o $@ $^

.c.o:
	$(CC) -c $<

clean:
	rm -rf *.o cdocker
