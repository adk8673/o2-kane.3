CC=gcc
OSS_OBJECTS=oss.o function_library.o
LINKEDLBS=-lrt
CFLAGS=-w

default: oss 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

oss: $(OSS_OBJECTS)
	$(CC) $(CFLAGS) $(LINKEDLBS) $(OSS_OBJECTS) -o oss

clean:
	rm -f oss
	rm -f *.o
	rm -f *.log
