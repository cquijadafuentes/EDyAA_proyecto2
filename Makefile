GPP = g++

CFLAGS = -Wall -g -O2 -std=c++11 -DNDEBUG -O3 -I/home/vbm_f/include -L/home/vbm_f/lib

LIBS =-lsdsl -ldivsufsort -ldivsufsort64

BINS=ejercicio_2 ejercicio_3 ejercicio_4 ejercicio_5 ejercicio_6

all: clean ${BINS}

ejercicio_2:
	${GPP} ${CFLAGS} -o ejercicio_2 ejercicio_2.cpp ${LIBS}

ejercicio_3:
	${GPP} ${CFLAGS} -o ejercicio_3 ejercicio_3.cpp ${LIBS}

ejercicio_4:
	${GPP} ${CFLAGS} -o ejercicio_4 ejercicio_4.cpp ${LIBS}

ejercicio_5:
	${GPP} ${CFLAGS} -o ejercicio_5 ejercicio_5.cpp ${LIBS}

ejercicio_6:
	${GPP} ${CFLAGS} -o ejercicio_6 ejercicio_6.cpp ${LIBS} 

clean:
	rm -f ${BINS}