APPNAME=test.out
OBJ = main.o \
	./hzylib/hzySocket.o
		
INCLUDE = ./hzylib/hzySocket.h

${APPNAME} : ${OBJ}
	@echo "cc $<"
	cc -o ${APPNAME} ${OBJ} -lpthread
	
	
clean : 
	rm ${OBJ} ${APPNAME}
