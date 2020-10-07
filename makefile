SOURCE=$(filter-out main.c,$(wildcard *.c))

server: $(SOURCE)
	gcc -o $@ $^ -lrt -Wall


clean:
	rm server