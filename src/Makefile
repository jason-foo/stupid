all:
	gcc -Wall -g -o ip ip.c
	gcc -Wall -g -o server main.c utils.c skbuff.c -lpthread
clean:
	rm ip server