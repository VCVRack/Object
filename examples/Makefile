
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


all: Animal

run: Animal
	time ./$^

Animal: Animal.c.o ../Object.cpp.o
	$(CXX) $(LDFLAGS) -o $@ $^

%.cpp.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

%.c.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -rfv *.o Animal
