
OBJS=src/main.o \
	 src/stringutil.o \
	 src/config.o \
	 src/log.o \
	 src/proxy.o \
	 src/server.o

LIBS=lib/tomlc99/libtoml.a
SHARELIBS=

.PHONY: all clean
.SUFFIXES: .c .o

CFLAGS += -Iinclude -Ilib/tomlc99 -DLOG_USE_COLOR

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

all: webserver

lib/tomlc99/libtoml.a:
	$(MAKE) -C lib/tomlc99 libtoml.a

webserver: $(OBJS) $(LIBS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) $(SHARELIBS)

clean:
	rm -f $(OBJS) webserver

