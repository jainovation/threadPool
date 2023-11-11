CC = g++
CFLAGS = -W -Wall
SRC = main.cpp Client.cpp
OBJ = main.o Client.o
PROG = client.out

all : ${PROG}
${PROG} : ${OBJ}
	${CC} -o ${PROG} ${OBJ}
	rm *.o

${Obj} : ${SRC}
	${CC} ${CFLAGS} -c -o main.o main.cpp
	${CC} ${CFLAGS} -c -o Client.o Client.cpp

clean :
	rm *.o server