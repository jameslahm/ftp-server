SOURCE=$(wildcard *.c)

server: $(SOURCE)
	gcc -o $@ $^ -lrt -Wall


clean:
	rm server