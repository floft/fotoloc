OUT        = fotoloc
SRC        = ${wildcard *.cpp}
OBJ        = ${SRC:.cpp=.o}
DEPENDS    = .depends

CXXFLAGS  += $(shell pkg-config --cflags opencv) -Wall -std=c++11 \
			 -g -O2 -ffast-math -funroll-loops
LDFLAGS   += $(shell pkg-config --libs opencv) -lIL

all: ${OUT}

${OUT}: ${OBJ}
	${CXX} -o $@ ${OBJ} ${LDFLAGS}

.cpp.o:
	${CXX} -c -o $@ $< ${CXXFLAGS}

${DEPENDS}: ${SRC}
	${RM} -f ./${DEPENDS}
	${CXX} ${CXXFLAGS} -MM $^ >> ./${DEPENDS}

install:
	install -Dm755 ${OUT} ${DESTDIR}${PREFIX}/bin/${OUT}
    
uninstall:
	${RM} ${DESTDIR}${PREFIX}/bin/${OUT}

clean:
	${RM} ${OUT} ${OBJ} ${DEPENDS}

-include ${DEPENDS}
.PHONY: all debug install uninstall clean
