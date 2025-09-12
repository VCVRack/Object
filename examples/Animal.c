#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Animal.h"


/** Duplicates a null-terminated string like the POSIX function but gracefully handles NULL.
*/
char* strdup2(const char* s) {
	if (!s)
		return NULL;
	size_t len = strlen(s);
	char* s2 = (char*) malloc(len + 1);
	memcpy(s2, s, len + 1);
	return s2;
}


// Animal

struct Animal {
	int legs;

	STORE_METHOD(Animal, speak);
	STORE_ACCESSOR(Animal, legs);
};


DEFINE_CLASS(Animal, (), (), {
	Animal* data = (Animal*) calloc(1, sizeof(Animal));
	data->legs = 0;
	PUSH_CLASS(self, Animal, data);
	SET_METHOD(self, Animal, Animal, speak);
	SET_ACCESSOR(self, Animal, Animal, legs);
}, {
	// printf("bye Animal\n");
	free(data);
})


DEFINE_METHOD_CONST_VIRTUAL(Animal, speak, void, VOID, (), (), {
	printf("I'm an animal with %d legs.\n", GET(self, Animal, legs));
})


DEFINE_ACCESSOR_VIRTUAL_AUTOMATIC(Animal, legs, int, -1)


DEFINE_METHOD_CONST(Animal, pet, void, VOID, (), {
	printf("You pet the animal.\n");
})


// Dog

struct Dog {
	char* name;

	STORE_ACCESSOR(Dog, name);
};


DEFINE_CLASS(Dog, (), (), {
	SPECIALIZE(self, Animal);
	Dog* data = (Dog*) calloc(1, sizeof(Dog));
	data->name = NULL;
	PUSH_CLASS(self, Dog, data);
	SET_METHOD(self, Dog, Animal, speak);
	SET_ACCESSOR(self, Dog, Animal, legs);
	SET_ACCESSOR(self, Dog, Dog, name);
	SET(self, Animal, legs, 4);
}, {
	// printf("bye Dog\n");
	free(data->name);
	free(data);
})


DEFINE_METHOD_CONST_OVERRIDE(Dog, speak, void, VOID, (), {
	printf("Woof, I'm a dog named %s with %d legs.\n", GET(self, Dog, name), GET(self, Animal, legs));
})


// This example override is pointless overhead, but it demonstrates how to call the superclass' method from an overridden method.
DEFINE_ACCESSOR_OVERRIDE(Dog, legs, int, -1, {
	return GET_DIRECT(self, Animal, legs);
}, {
	if (legs > 4)
		legs = 4;
	SET_DIRECT(self, Animal, legs, legs);
})


DEFINE_ACCESSOR_VIRTUAL(Dog, name, const char*, "", {
	return data->name;
}, {
	free(data->name);
	data->name = strdup2(name);
})


// Usage example

int main() {
	Object* animal = Animal_create();

	// Non-virtual method, cannot be overridden
	Animal_pet(animal); // "You pet the animal."

	// Virtual method
	Animal_speak(animal); // "I'm an animal with 0 legs."

	// We can specialize existing Objects even after they are created.
	// If the object is already a Dog, this fails gracefully.
	Dog_specialize(animal);

	// Call virtual setters
	Dog_name_set(animal, "Fido");
	Animal_legs_set(animal, 3);

	// Call virtual method, which calls Dog's overridden implementation.
	Animal_speak(animal); // "Woof, I'm a dog named Fido with 3 legs."

	// All Objects must be released.
	Object_release(animal);

	return 0;
}