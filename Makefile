CROSS_COMPILE =
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)ar
I = include
S = src

OBJS = $S/alloc.o $S/ev.o $S/net.o $S/tun.o


all: libx.a


%.o: %.c
	$(CC) -I $I -Wall -O2 -c -o $@ $<


libx.a: $(OBJS)
	$(AR) rcs $@ $^


clean:
	rm $S/*.o
	rm *.a

