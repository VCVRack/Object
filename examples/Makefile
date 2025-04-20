
FLAGS += -g
FLAGS += -O3
FLAGS += -Wall -Wextra
FLAGS += -mavx
FLAGS += -fPIC
FLAGS += -I..

CXXFLAGS += $(FLAGS)
CXXFLAGS += -std=c++11

CFLAGS += $(FLAGS)
CFLAGS += -std=c99

LDFLAGS += $(FLAGS)


all: Animal

run: Animal
	time ./$^

Animal: ../Object.cpp.o Animal.c.o
	$(CXX) $(LDFLAGS) -o $@ $^

%.cpp.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

%.c.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -rfv *.o Animal
