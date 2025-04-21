#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Animal.h"
#include "Ref.h"


/** POSIX's implementation doesn't gracefully handle NULL */
char* strdup2(const char *s) {
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
	STORE_ACCESSORS(Animal, legs);
};


DEFINE_CLASS(Animal, (), (), {
	Animal* data = (Animal*) calloc(1, sizeof(Animal));
	data->legs = 0;
	PUSH_CLASS(self, Animal, data);
	// TODO We're not exactly overriding here, and the arguments are redundant
	OVERRIDE_METHOD(self, Animal, Animal, speak);
	OVERRIDE_ACCESSORS(self, Animal, Animal, legs);
}, {
	free(data);
})


DEFINE_METHOD_CONST(Animal, speak, void, VOID, (), (), {
	printf("I'm an animal with %d legs.\n", GET(self, Animal, legs));
})


DEFINE_ACCESSORS_AUTOMATIC(Animal, legs, int, -1)


DEFINE_FUNCTION_CONST(Animal, pet, void, VOID, (), {
	printf("You pet the animal.\n");
})


// Dog

struct Dog {
	char* name;

	STORE_ACCESSORS(Dog, name);
};


DEFINE_CLASS(Dog, (), (), {
	SPECIALIZE(self, Animal);
	Dog* data = (Dog*) calloc(1, sizeof(Dog));
	data->name = NULL;
	PUSH_CLASS(self, Dog, data);
	OVERRIDE_METHOD(self, Dog, Animal, speak);
	// OVERRIDE_ACCESSORS(self, Dog, Animal, legs);
	// TODO We're not exactly overriding here, and the arguments are redundant
	OVERRIDE_ACCESSORS(self, Dog, Dog, name);
	SET(self, Animal, legs, 4);
}, {
	free(data->name);
	free(data);
})


DEFINE_METHOD_CONST_OVERRIDE(Dog, speak, void, VOID, (), {
	printf("Woof, I'm a dog named %s with %d legs.\n", GET(self, Dog, name), GET(self, Animal, legs));
})


// This example override is pointless overhead, but it demonstrates how to call the superclass' method from an overridden method.
DEFINE_ACCESSORS_OVERRIDE(Dog, legs, int, -1, {
	return GET_DIRECT(self, Animal, legs);
}, {
	SET_DIRECT(self, Animal, legs, legs);
})


DEFINE_ACCESSORS(Dog, name, const char*, "", {
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

	// Ref_obtain(animal);
	// Ref_release(animal);

	// All Objects must be freed.
	Object_free(animal);

	return 0;
}