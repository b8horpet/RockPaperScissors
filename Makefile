.PHONY: all
all:
	mkdir -p output
	g++ rps.cpp -g -o output/rps.out -lcurses -lboost_system -lboost_program_options -lpthread && echo "all done"

.PHONY: clean
clean:
	rm -rf output
