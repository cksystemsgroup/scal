#CPPFLAGS = -Wall -Werror -ggdb
CPPFLAGS = -Wall -Werror -O3
LDFLAGS = 

all: analyzer

%.o: %.cpp *.h Makefile
	g++ ${CPPFLAGS} -c $< -o $@

analyzer: analyzer.o operation.o parser.o linearizer.o
	g++ ${CPPFLAGS} ${LDFLAGS} $^ -o $@

.PHONY: clean

clean:
	@rm -f *.o analyzer
