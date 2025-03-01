
FLAGS += -g
FLAGS += -O3
FLAGS += -Wall -Wextra
FLAGS += -mavx
FLAGS += -fPIC

CXXFLAGS += $(FLAGS)
CXXFLAGS += -std=c++11

CFLAGS += $(FLAGS)
CFLAGS += -std=c99

LDFLAGS += $(FLAGS)


all: example

run: example
	./example

example: example.c.o Object.cpp.o
	$(CXX) $(LDFLAGS) -o $@ $^

%.cpp.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

%.c.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -rfv *.o example
