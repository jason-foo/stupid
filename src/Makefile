all: server

objs = main.o utils.o skbuff.o dev.o ip_input.o

$(objs):%.o:%.c
	gcc -Wall -c -o $@ $<

server:	$(objs)
#	gcc -Wall -g -o ip ip.c
	gcc -Wall -g -o $@ $(objs) -lpthread
clean:
	rm server $(objs)