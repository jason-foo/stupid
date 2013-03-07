all: stupid

objs = main.o utils.o skbuff.o dev.o ip_input.o udp.o tcp.o ip_output.o arp.o \
ip_route.o netdevice.o sock.o

$(objs):%.o:%.c
	gcc -Wall -c -o $@ $<

stupid:	$(objs)
	gcc -Wall -g -o $@ $(objs) -lpthread
clean:
	rm stupid $(objs)

# auto generated rules
arp.o : arp.h skbuff.h dev.h utils.h
dev.o : dev.h ip.h sock.h skbuff.h arp.h utils.h
ip_input.o : ip.h udp.h tcp.h utils.h
ip_output.o : ip.h sock.h skbuff.h dev.h utils.h ip_route.h
ip_route.o : ip_route.h utils.h netdevice.h
main.o : utils.h sock.h skbuff.h netdevice.h dev.h udp.h arp.h ip_route.h
netdevice.o : netdevice.h
skbuff.o : sock.h skbuff.h utils.h arp.h
sock.o : ../include/common.h sock.h utils.h udp.h skbuff.h
tcp.o : tcp.h skbuff.h
udp.o : sock.h skbuff.h udp.h utils.h ip.h
utils.o : utils.h
