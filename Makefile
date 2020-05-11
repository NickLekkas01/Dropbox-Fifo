exec: main.o functions.o
	gcc functions.o main.o -o exec

functions.o: functions.c
	gcc functions.c -c
main.o: main.c
	gcc main.c -c
clean:
	rm exec main.o functions.o
