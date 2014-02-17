CC=gcc
DEPS=threads.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $<

threads: threads.h main.c
	$(CC) -o threads -O0 main.c threads.h threads.c

clean: 
	rm -r ./threads
