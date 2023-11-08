CC = gcc
DIR = ..

%.out: %.c $(DIR)/libx.a
	$(CC) -g -o $@ $< -I$(DIR)/include -L$(DIR) -lx
