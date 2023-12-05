CROSS_COMPILE =
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)ar
I = include
S = src

OBJS = $S/alloc.o $S/ev.o $S/net.o $S/tun.o $S/bio.o


all: libx.a


%.o: %.c
	$(CC) -I $I -Wall -O2 -c -o $@ $<


libx.a: $(OBJS)
	$(AR) rcs $@ $^


example: example/*.c
	@for file in $^; do \
		$(CC) -g -o $${file}.out $${file} -I$I -L. -lx; \
	done


clean:
	rm $S/*.o
	rm *.a

