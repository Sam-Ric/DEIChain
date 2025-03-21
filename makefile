FLAGS	= -Wall -g
CC	= gcc
PROG	= DEIChain
OBJS	= controller.o miner.o validator.o statistics.o utils.o

all:	${PROG}

clean:
	rm ${OBJS}

${PROG}:	${OBJS}
	${CC} ${FLAGS} ${OBJS} -o $@

.c.o:
	${CC}	${FLAGS} $< -c

################################

utils.o:	utils.h utils.c

miner.o:	utils.h miner.h miner.c

validator.o:	utils.h validator.h validator.c

statistics.o:	utils.h statistics.h statistics.c

controller.o:	utils.h validator.h statistics.h miner.h controller.c

DEIChain:	controller.o statistics.o validator.o miner.o utils.o
