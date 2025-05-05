FLAGS	= -Wall -g -lpthread -L/usr/lib/aarch64-linux-gnu -lcrypto
CC	= gcc
PROG1	= DEIChain
PROG2 = TxGen
OBJS1	= controller.o miner.o validator.o statistics.o utils.o pow.o
OBJS2 = tx_gen.o utils.o

all:	${PROG1} ${PROG2}

clean:
	rm -f ${OBJS1} ${OBJS2}

${PROG1}: ${OBJS1}
	${CC} ${OBJS1} -o $@ -lpthread -L/usr/lib/aarch64-linux-gnu -lcrypto

${PROG2}: ${OBJS2}
	${CC} ${FLAGS} ${OBJS2} -o $@

.c.o:
	${CC}	${FLAGS} $< -c

################################

utils.o:	utils.h utils.c

pow.o:	pow.h pow.c

miner.o:	utils.h miner.h pow.h miner.c

validator.o:	utils.h validator.h validator.c

statistics.o:	utils.h statistics.h statistics.c

controller.o:	utils.h validator.h statistics.h miner.h controller.c

tx_gen.o:	utils.h tx_gen.c

DEIChain:	controller.o statistics.o validator.o miner.o utils.o

TxGen:	tx_gen.o utils.o
