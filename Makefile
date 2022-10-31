CC		  := gcc
CFLAGS := -fPIC -Wall -pedantic -std=gnu99 -lpthread

objects = psserver psclient
all: $(objects)

$(CC) $(CFLAGS) $(objects):%: %.c $(-o) $@ $<