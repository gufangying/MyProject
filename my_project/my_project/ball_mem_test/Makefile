test.out : main.o
	arm-poky-linux-gnueabi-gcc -o test.out main.o
main.o : main.c main.h
	arm-poky-linux-gnueabi-gcc -c main.c
clean : 
	rm *.o *.out
