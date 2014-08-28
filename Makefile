.PHONY: all
all:
	mkdir -p output
	g++ rps.cpp -o output/rps.out -lncurses

.PHONY: clean
clean:
	rm -rf output
