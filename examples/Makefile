
FLAGS += -g
FLAGS += -O3
FLAGS += -Wall -Wextra
FLAGS += -mavx
FLAGS += -fPIC
FLAGS += -I..

CXXFLAGS += $(FLAGS)
CXXFLAGS += -std=c++17

CFLAGS += $(FLAGS)
CFLAGS += -std=c99

LDFLAGS += $(FLAGS)


all: test

run: test
	time ./$^

test: Animal.c.o ../src/Object.cpp.o ../src/WeakObject.c.o ../src/ObjectProxies.cpp.o test.cpp.o
	$(CXX) $(LDFLAGS) -o $@ $^

%.cpp.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

%.c.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -rfv *.o ../src/*.o test
