
#
all: clean server client clear

dataSetCheck: dataSetCheck.o
	gcc -o dataSetCheck dataSetCheck.o

dataSetCheck.o: dataSetCheck.c
	gcc -c dataSetCheck.c

server: sender.o
	gcc -o server sender.o

sender.o: sender.c
	gcc -c sender.c

client: receiver.o
	gcc -o client receiver.o

receiver.o: receiver.c
	gcc -c receiver.c

clean:
	rm -f *.o server client recv.txt dataSetCheck
clear:
	clear