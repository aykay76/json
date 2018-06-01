all: json

json: main.c
	gcc -g main.c -o json

%.o: %.c
	gcc -c -o $@ $^

clean:
	rm json