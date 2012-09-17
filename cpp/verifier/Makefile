CPPFLAGS= -Wall -Werror -O3
#CPPFLAGS=-Wall -Werror -ggdb
#CPPFLAGS= -ggdb
LDFLAGS=#-lprofiler

all: generate_traces analyze_semantics test

%.o: %.cpp *.h Makefile
	g++ ${CPPFLAGS} -c $< -o $@

generate_traces: generate_traces.o trace.o operation.o semantic.o fifoExecuter.o
	g++ ${CPPFLAGS} ${LDFLAGS} $^ -o $@

analyze_semantics: analyze_semantics.o operation.o fifoExecuter.o fifoExecuterLowerBound.o fifoExecuterUpperBound.o
	g++ ${CPPFLAGS} ${LDFLAGS} $^ -o $@

test: test.o operation.o trace.o fifoExecuter.o fifoExecuterLowerBound.o fifoExecuterUpperBound.o
	g++ ${CPPFLAGS} ${LDFLAGS} $^ -o $@

.PHONY: clean

clean:
	@rm -f *.o generate_traces
