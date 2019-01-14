all:
	gcc  -c server.c
	gcc server.o -o server -lm -pthread
	gcc  -c client.c
	gcc client.o -o client 
	
clean:
	rm -rf *o exe
