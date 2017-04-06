CC=g++
CFLAGS=-c -Wall -std=c++11 \
		-I./libuv/include/
LDFLAGS=libuv.so.1.0.0 \
		-lpthread
SOURCES=example.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=example

all: child $(SOURCES) $(EXECUTABLE) run

child:
		$(CC) -std=c++11 child.cpp -o child

$(EXECUTABLE): $(OBJECTS)
		$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.cpp.o:
		$(CC) $(CFLAGS) $< -o $@

clean:
		rm -rf *.o child $(EXECUTABLE)

run:
		./example