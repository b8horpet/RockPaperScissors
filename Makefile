.PHONY: all
all:
	mkdir -p output
	g++ rps.cpp -g -o output/rps.out -lncurses -lboost_system -lpthread

.PHONY: clean
clean:
	rm -rf output
