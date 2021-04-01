CFLAGS = -std=gnu99 -Wall -W -Wextra

all :  main

main : main.o

main.o: main.c

clean :
	rm -f main.o
distclean :
	rm -f main.o