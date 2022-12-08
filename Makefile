CC=gcc
AR=ar rcs
RM=rm -f
CFLAGS=-Wall
LIB=libx.a
OBJ=net.o tun.o log.o
PREFIX=/usr/local

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) $@ $?

clean:
	$(RM) *.o $(LIB)

install:
	mkdir -p $(PREFIX)/include/x
	cp *.h $(PREFIX)/include/x
	cp $(LIB) $(PREFIX)/lib
