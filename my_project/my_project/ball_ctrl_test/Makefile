ctrl_client.out : client.o
	gcc -o ctrl_client.out client.o -lpthread
client.o : client.c
	gcc -c client.c

clean : 
	rm *.o *.out
