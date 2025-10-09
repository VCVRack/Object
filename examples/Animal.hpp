#pragma once

#include <stdio.h>
#include <Object/CppWrapper.h>
#include "Animal.h"


namespace cpp {


struct Animal : ObjectWrapper {
	Animal() : Animal(Animal_create(), true) {}

	Animal(Object* self, bool original = false) : ObjectWrapper(self, original) {
		if (original) {
			BIND_METHOD_CONST(Animal, Animal, speak, (), {
				that->speak();
			});
		}
	}

	~Animal() {
		printf("bye cpp::Animal\n");
	}

	virtual void speak() const {
		if (original)
			CALL_DIRECT(self, Animal, speak);
		else
			CALL(self, Animal, speak);
	}

	void pet() {
		CALL(self, Animal, pet);
	}

	ACCESSOR_PROXY(Animal, Animal, legs, int);
};


struct Dog : Animal {
	Dog(const char* name = NULL) : Dog(Dog_create(name), true) {}

	Dog(Object* self, bool original = false) : Animal(self, original) {}

	~Dog() {
		printf("bye cpp::Dog\n");
	}

	void speak() const override {
		if (original)
			CALL_DIRECT(self, Dog, speak);
		else
			CALL(self, Animal, speak);
	}

	ACCESSOR_PROXY(Dog, Dog, name, const char*);

	GETTER_PROXY_CUSTOM(Dog, that, Dog*, {
		return CppWrapper_castOrCreate<Dog>(self);
	});
};


/** Example of extending a C++ wrapper class without defining an Object class. */
struct Poodle : Dog {
	~Poodle() {
		printf("bye Poodle\n");
	}

	void speak() const override {
		printf("Yip yip yip yip yip yip %s!\n", GET(self, Dog, name));
	}
};


} // namespace cpp
