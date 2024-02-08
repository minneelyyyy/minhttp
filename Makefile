
OBJS=src/main.o src/minhttp.o

.PHONY: all clean
.SUFFIXES: .c .o

CFLAGS += -I.

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

all: webserver

webserver: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

clean:
	rm -f $(OBJS) webserver

