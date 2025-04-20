#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "example.h"


/** POSIX's implementation doesn't gracefully handle NULL */
char* strdup(const char *s) {
	if (!s)
		return NULL;
	size_t len = strlen(s);
	char* s2 = malloc(len + 1);
	memcpy(s2, s, len + 1);
	return s2;
}


// Animal

typedef struct Animal {
	int legs;

	STORE_METHOD(Animal, speak);
	STORE_ACCESSORS(Animal, legs);
} Animal;


DEFINE_CLASS(Animal, (), (), {
	Animal* data = calloc(1, sizeof(Animal));
	data->legs = 0;
	PUSH_CLASS(self, Animal, data);
	// TODO We're not exactly overriding here, and the arguments are redundant
	OVERRIDE_METHOD(self, Animal, Animal, speak);
	OVERRIDE_ACCESSORS(self, Animal, Animal, legs);
}, {
	free(data);
})


DEFINE_METHOD_CONST(Animal, speak, void, VOID, (), (), {
	printf("I'm an animal with %d legs.\n", GET_PROPERTY(self, Animal, legs));
})


DEFINE_ACCESSORS_AUTOMATIC(Animal, legs, int, -1)


// Dog

typedef struct Dog {
	char* name;

	STORE_ACCESSORS(Dog, name);
} Dog;


DEFINE_CLASS(Dog, (), (), {
	SPECIALIZE(self, Animal);
	Dog* data = calloc(1, sizeof(Dog));
	data->name = NULL;
	PUSH_CLASS(self, Dog, data);
	OVERRIDE_METHOD(self, Dog, Animal, speak);
	OVERRIDE_ACCESSORS(self, Dog, Animal, legs);
	// TODO We're not exactly overriding here, and the arguments are redundant
	OVERRIDE_ACCESSORS(self, Dog, Dog, name);
	SET_PROPERTY(self, Animal, legs, 4);
}, {
	free(data->name);
	free(data);
})


DEFINE_METHOD_CONST_OVERRIDE(Dog, speak, void, VOID, (), {
	printf("Woof, I'm a dog named %s with %d legs.\n", GET_PROPERTY(self, Dog, name), GET_PROPERTY(self, Animal, legs));
})


// This override is pointless, but it shows how to call the superclass' method from an overridden method.
DEFINE_ACCESSORS_OVERRIDE(Dog, legs, int, -1, {
	return GET_PROPERTY_DIRECT(self, Animal, legs);
}, {
	SET_PROPERTY_DIRECT(self, Animal, legs, legs);
})


DEFINE_ACCESSORS(Dog, name, const char*, "", {
	if (!data->name)
		return "";
	return data->name;
}, {
	free(data->name);
	data->name = strdup(name);
})


// Usage example

int main() {
	Object* animal = Animal_create();
	Animal_speak(animal);

	// We can specialize existing Objects with *_init().
	// If the object is already a Dog, this fails gracefully.
	Dog_specialize(animal);

	Dog_name_set(animal, "Fido");
	Animal_legs_set(animal, 3);

	// Call virtual method, which calls Dog's overridden implementation.
	Animal_speak(animal);

	// All Objects must be freed.
	Object_free(animal);

	return 0;
}