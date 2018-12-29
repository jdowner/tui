CFLAGS+=$(shell pkg-config --cflags vte-2.91)
LIBS+=$(shell pkg-config --libs vte-2.91)

default: tui.c
	gcc -O2 -Wall $(CFLAGS) tui.c -o tui $(LIBS)

clean:
	rm -f tui
