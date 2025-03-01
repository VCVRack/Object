#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "example.h"


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
	STORE_PROPERTY(Animal, legs);
} Animal;


DEFINE_CLASS(Animal, (), (), {
	Animal* data = calloc(1, sizeof(Animal));
	data->legs = 0;
	PUSH_TYPE(self, Animal, data);
	// TODO We're not exactly overriding here, and the arguments are redundant
	OVERRIDE_METHOD(self, Animal, Animal, speak);
	OVERRIDE_PROPERTY(self, Animal, Animal, legs);
}, {
	free(data);
})


DEFINE_METHOD_CONST(Animal, speak, void, (), (), {
	if (!data)
		return;
	printf("I'm an animal with %d legs.\n", GET_PROPERTY(self, Animal, legs));
})


DEFINE_PROPERTY_AUTOMATIC(Animal, legs, int, -1)


// Dog

typedef struct Dog {
	char* name;

	STORE_PROPERTY(Dog, name);
} Dog;


DEFINE_CLASS(Dog, (), (), {
	INIT_CLASS(self, Animal);
	Dog* data = calloc(1, sizeof(Dog));
	data->name = NULL;
	// TODO Add `self` to signatures?
	PUSH_TYPE(self, Dog, data);
	OVERRIDE_METHOD(self, Dog, Animal, speak);
	OVERRIDE_PROPERTY(self, Dog, Animal, legs);
	// TODO We're not exactly overriding here, and the arguments are redundant
	OVERRIDE_PROPERTY(self, Dog, Dog, name);
	SET_PROPERTY(self, Animal, legs, 4);
}, {
	if (!data)
		return;
	free(data->name);
	free(data);
})


DEFINE_METHOD_CONST_OVERRIDE(Dog, speak, void, (), {
	printf("Woof, I'm a dog named %s with %d legs.\n", GET_PROPERTY(self, Dog, name), GET_PROPERTY(self, Animal, legs));
})


DEFINE_PROPERTY_OVERRIDE(Dog, legs, int, {
	return GET_PROPERTY_DIRECT(self, Animal, legs);
}, {
	SET_PROPERTY_DIRECT(self, Animal, legs, legs);
})


DEFINE_PROPERTY(Dog, name, const char*, {
	if (!data)
		return NULL;
	return data->name;
}, {
	if (!data)
		return;
	free(data->name);
	data->name = strdup(name);
})


// Usage example

int main() {
	Object* animal = Animal_create();
	Animal_speak(animal);

	// We can specialize existing Objects with *_init().
	// If the object is already a Dog, this fails gracefully.
	Dog_init(animal);

	Dog_name_set(animal, "Fido");
	Animal_legs_set(animal, 3);

	// Call virtual method, which calls Dog's overridden implementation.
	Animal_speak(animal);

	// We must free each Object.
	Object_free(animal);

	return 0;
}