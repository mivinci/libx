CROSS_COMPILE =
PREFIX = build

CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)ar

all: libx.a

%.o: %.c
	$(CC) -Wall -c -o $@ $<

libx.a: ev.o net.o tun.o
	$(AR) rcs $@ $^


.PHONY: install
install:
	@[ -d $(PREFIX)/lib ] || mkdir $(PREFIX)/lib
	@[ -d $(PREFIX)/include/x ] || mkdir -p $(PREFIX)/include/x
	
	@echo "copying libx.a to $(PREFIX)/lib/"
	@cp libx.a $(PREFIX)/lib/
	
	@echo "copying *.h to $(PREFIX)/include/x/"
	@cp -r *.h $(PREFIX)/include/x/

clean:
	rm *.o
	rm *.a
	rm -r build
