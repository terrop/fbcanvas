# Makefile - 20.5.2008 - 20.5.2008 Ari & Tero Roponen

CFLAGS:=$(shell pkg-config --cflags poppler-glib) -Os
LIBS:=$(shell pkg-config --libs poppler-glib) -lncurses -lmagic

oma: main.o fbcanvas.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
clean:
	rm -f oma main.o fbcanvas.o
