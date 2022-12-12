all: clean server client 

server: sender.o
	gcc -o server sender.o

sender.o: sender.c
	gcc -c sender.c

client: receiver.o
	gcc -o client receiver.o

receiver.o: receiver.c
	gcc -c receiver.c

clean:
	rm -f *.o server client recv.txt
clear:
	clear