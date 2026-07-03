CC=gcc
CXX=g++
CFLAGS=-O2 -Wall -Wextra -std=c11 -I./src
CXXFLAGS=-O2 -Wall -Wextra -std=c++17
LDLIBS=-lwiringPi -lm
OPENCV_LIBS=$(shell pkg-config --cflags --libs opencv4 2>/dev/null)

all: ppg server client ear

ppg: src/ppg.c src/config.h
	$(CC) $(CFLAGS) -o ppg src/ppg.c $(LDLIBS)

server: src/server.c src/config.h
	$(CC) $(CFLAGS) -o server src/server.c $(LDLIBS)

client: src/client.c src/config.h
	$(CC) $(CFLAGS) -o client src/client.c $(LDLIBS)

ear: src/ear.cpp
	$(CXX) $(CXXFLAGS) -o ear src/ear.cpp $(OPENCV_LIBS)

clean:
	rm -f ppg server client ear
.PHONY: all clean
