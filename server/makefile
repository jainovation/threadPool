CC = g++
CFLAGS = -W -Wall
SRC = main.cpp Server.cpp
OBJ = main.o Server.o
PROG = server.out

all : ${PROG}
${PROG} : ${OBJ}
	${CC} -o ${PROG} ${OBJ}
	rm *.o

${Obj} : ${SRC}
	${CC} ${CFLAGS} -c -o main.o main.cpp
	${CC} ${CFLAGS} -c -o Server.o Server.cpp

clean :
	rm *.o server