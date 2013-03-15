LIBS = -lstupid -lpthread
obj = udp_echo_client
all: $(obj)

$(obj):%:%.c
	gcc -Wall -L../api -I../api -o $@ $< $(LIBS)
$(obj): ../api/libstupid.a

clean:
	rm $(obj)
