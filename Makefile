CFLAGS = -std=gnu11
LIBS = -pthread -lm
SOURCES = main.c interface.c my_memory.c
OUT = proj2

default:
	gcc $(CFLAGS) $(SOURCES) $(LIBS) -o $(OUT)
debug:
	gcc -g $(CFLAGS) $(SOURCES) $(LIBS) -o $(OUT)
clean:
	rm $(OUT)
