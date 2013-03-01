all: server

objs = main.o utils.o skbuff.o dev.o ip_input.o udp.o tcp.o ip_output.o arp.o \
ip_route.o netdevice.o

$(objs):%.o:%.c
	gcc -Wall -c -o $@ $<

server:	$(objs)
	gcc -Wall -g -o $@ $(objs) -lpthread
clean:
	rm server $(objs)

# auto generated rules
arp.o : arp.h skbuff.h dev.h utils.h
dev.o : dev.h ip.h skbuff.h arp.h utils.h
ip_input.o : ip.h udp.h tcp.h utils.h
ip_output.o : ip.h skbuff.h dev.h utils.h ip_route.h
ip_route.o : ip_route.h utils.h netdevice.h
main.o : utils.h skbuff.h netdevice.h sock.h skbuff.h dev.h udp.h arp.h ip_route.h
netdevice.o : netdevice.h
skbuff.o : skbuff.h sock.h utils.h
tcp.o : tcp.h skbuff.h
udp.o : skbuff.h udp.h utils.h ip.h
utils.o : utils.h
