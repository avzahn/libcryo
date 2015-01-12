CC=clang
LFLAGS=-lm -ljansson -lcurl
LISTENERSRCS=listener.c cryo.c interpolator.c listener_test.c
SPATH=

all: listener interpolator

listener: 
	$(CC) $(LISTENERSRCS) -o listener $(LFLAGS)
	
interpolator:
	$(CC) interpolator_test.c -o interpolator
	

	
clean:
	rm -rf listener interpolator
