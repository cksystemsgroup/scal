CPPFLAGS = -Wall -Werror -ggdb
# CPPFLAGS = -Wall -Werror -03
LDFLAGS = 

all: analyzer

%.o: %.cpp *.h Makefile
	g++ ${CPPFLAGS} -c $< -o $@

analyzer: analyzer.o operation.o
	g++ ${CPPFLAGS} ${LDFLAGS} $^ -o $@

.PHONY: clean

clean:
	@rm -f *.o analyzer
