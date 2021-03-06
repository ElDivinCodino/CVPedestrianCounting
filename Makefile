CC = g++
CFLAGS = -g -Wall -std=c++11
SRCS = src/Blob.cpp src/main.cpp src/AdaptiveBackgroundLearning.cpp
PROG = main

OPENCV = $(shell pkg-config opencv --cflags --libs)
LIBS = $(OPENCV)
LFLAGS = -Wl,-rpath,/usr/local/lib/

$(PROG) : $(SRCS)
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LIBS) $(LFLAGS)

.PHONY : clean
clean :
		-rm -f *.o $(PROG)
 
