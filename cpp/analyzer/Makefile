CPPFLAGS = -Wall -ggdb
#CPPFLAGS = -Wall -Werror -ggdb
#CPPFLAGS = -Wall -Werror -O3
LDFLAGS = 

all: analyzer

%.o: %.cpp *.h Makefile
	g++ ${CPPFLAGS} -c $< -o $@

analyzer: analyzer.o operation.o parser.o linearizer_max.o linearizer_invocation.o linearizer_sum.o linearizer_point.o
	g++ ${CPPFLAGS} ${LDFLAGS} $^ -o $@

.PHONY: clean

clean:
	@rm -f *.o analyzer
