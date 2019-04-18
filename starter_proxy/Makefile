IDIR=include
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=build
SDIR=src

LIBS=-lm

_DEPS = proxy.h httpparser.h customsocket.h log.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = proxy.o httpparser.o customsocket.o log.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


BINS = proxy

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: proxy

proxy: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

customsocket: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

httpparser: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

log: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)


.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ $(BINS) $(IDIR)/*~ 