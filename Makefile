
OBJS=src/main.o src/minhttp.o src/config.o src/stringutil.o src/log.o
LIBS=lib/tomlc99/libtoml.a
SHARELIBS=

.PHONY: all clean
.SUFFIXES: .c .o

CFLAGS += -Iinclude -Ilib/tomlc99

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

all: webserver

lib/tomlc99/libtoml.a:
	$(MAKE) -C lib/tomlc99 libtoml.a

webserver: $(OBJS) $(LIBS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) $(SHARELIBS)

clean:
	rm -f $(OBJS) webserver

