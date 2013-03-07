CFLAGS = -Wall
LIBS = -lpthread
inc_dir = ../include

all:
	gcc $(CFLAGS) -I$(inc_dir) -c -o stupid.o stupid.c $(LIBS)
	ar -rv libstupid.a stupid.o
clean:
	rm *.o lib*