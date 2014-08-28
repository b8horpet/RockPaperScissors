.PHONY: all
all:
	mkdir output
	g++ rps.cpp -o output/rps.out -lncurses

.PHONY: clean
clean:
	rm -rf output
