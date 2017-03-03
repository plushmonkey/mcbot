CXXFLAGS=-std=c++11 -Wall -fPIC -O2 -I/usr/include/jsoncpp -Ilib/mclib
LIBS=-L. -ljsoncpp -lmc
CXX=clang++
BIN=bin

SRC=$(wildcard mcbot/*.cpp mcbot/*/*.cpp)

bot: $(SRC:.cpp=.o) libmc.so
	$(CXX) -o $(BIN)/$@ $(CXXFLAGS) $^ -Wl,-rpath,. $(LIBS)
	-mv -f libmc.so bin/libmc.so

libmc.so: directory
	$(MAKE) -C lib/mclib
	cp lib/mclib/libmc.so libmc.so

directory:
	-mkdir $(BIN)

clean:
	-rm -f mcbot/*.o
	-rm -f mcbot/*/*.o
