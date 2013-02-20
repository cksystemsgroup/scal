CPPFLAGS = -Wall -ggdb
#CPPFLAGS = -Wall -Werror -ggdb
#CPPFLAGS = -Wall -Werror -O3
LDFLAGS = 

all: linearizer analyzer test nondet

test: test.o operation.o parser.o linearizer_max.o linearizer_invocation.o linearizer_sum.o linearizer_point.o linearizer_response.o
	g++ ${CPPFLAGS} ${LDFLAGS} $^ -o $@

%.o: %.cpp *.h
	g++ ${CPPFLAGS} -c $< -o $@

analyzer: analyzer.o operation.o parser.o age.o
	g++ ${CPPFLAGS} ${LDFLAGS} $^ -o $@

nondet: nondet.o operation.o parser.o
	g++ ${CPPFLAGS} ${LDFLAGS} $^ -o $@

linearizer: linearizer.o operation.o parser.o linearizer_max.o linearizer_invocation.o linearizer_sum.o linearizer_point.o linearizer_response.o
	g++ ${CPPFLAGS} ${LDFLAGS} $^ -o $@

.PHONY: clean

clean:
	@rm -f *.o analyzer
