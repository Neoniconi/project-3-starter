IDIR=include
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=build
SDIR=src

LIBS=-lm

_DEPS = proxy.h httpparser.h customsocket.h log.h dns_packet.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = proxy.o httpparser.o customsocket.o log.o dns_packet.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_nOBJ = nameserver.o log.o dns_packet.o
nOBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


BINS = proxy nameserver

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: proxy nameserver

proxy: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

customsocket: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

httpparser: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

dns_packet: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

log: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

nameserver: $(nOBJ)
	$(CC) -c -o build/nameserver.o src/nameserver.c $(CFLAGS)
	$(CC) -o nameserver build/nameserver.o build/log.o build/dns_packet.o $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ $(BINS) $(IDIR)/*~ 