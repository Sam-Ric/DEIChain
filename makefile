FLAGS	= -Wall -g
CC	= gcc
PROG	= DEIChain
OBJS	= controller.o miner.o validator.o statistics.o

all:	${PROG}

clean:
	rm ${OBJS}

${PROG}:	${OBJS}
	${CC} ${FLAGS} ${OBJS} -o $@

.c.o:
	${CC}	${FLAGS} $< -c

################################

miner.o:	header.h miner.c

validator.o:	validator.c

statistics.o: statistics.c

controller.o: header.h controller.c

DEIChain:	controller.o statistics.o validator.o miner.o

