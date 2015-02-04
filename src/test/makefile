CC=clang
LFLAGS=-lm -ljansson -lcurl
LISTENERSRCS=../listener.c ../cryo.c ../interpolator.c listener_test.c
INCFLAGS= -I.. -L..

all: listener interpolator

listener: 
	$(CC) $(INCFLAGS) $(LISTENERSRCS) -o listener $(LFLAGS)
	
interpolator:
	$(CC) $(INCFLAGS) ../interpolator.c interpolator_test.c -o interpolator
	
clean:
	rm -rf listener interpolator
