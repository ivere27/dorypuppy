CC=g++
CFLAGS=-c -Wall -std=c++11 \
		-I./libuv/include/
LDFLAGS=libuv.so.1.0.0 \
		-lpthread
SOURCES=example.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=example

ANDROID_X86_CC=./android-toolchain/bin/i686-linux-android-g++

all: child $(SOURCES) $(EXECUTABLE) run

child:
		$(CC) -std=c++11 child.cpp -o child

$(EXECUTABLE): $(OBJECTS)
		$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.cpp.o:
		$(CC) $(CFLAGS) $< -o $@

clean:
		rm -rf *.o child $(EXECUTABLE)

ANDROID_X86:
		$(ANDROID_X86_CC) -std=c++11 child.cpp -o child
		$(ANDROID_X86_CC) -std=c++11 example.cpp -o example \
			-I../libuv/include/ \
			./SampleApplication/app/libs/x86/libuv.so
		adb -e push child /data/local/tmp/
		adb -e push example /data/local/tmp/

run:
		./example