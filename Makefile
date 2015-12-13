.PHONY: all
all:
	mkdir -p output
	g++ -std=c++11 MD5.cxx rps.cpp -g -o output/rps.out -lcurses -lboost_system -lboost_program_options -lpthread && echo "all done"

.PHONY: clean
clean:
	rm -rf output
