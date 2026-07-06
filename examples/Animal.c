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
};


DEFINE_CLASS(Animal, (), (), {
	Animal* slot = (Animal*) calloc(1, sizeof(Animal));
	CLASS_PUSH(self, Animal, slot);
	METHOD_PUSH(self, Animal, Animal, speak);
	ACCESSOR_PUSH(self, Animal, Animal, legs);
}, {
	printf("bye Animal\n");
	free(slot);
})


DEFINE_METHOD_CONST_VIRTUAL(Animal, speak, void, (), VOID, (), {
	printf("I'm an animal with %d legs.\n", GET(self, Animal, legs));
})


DEFINE_ACCESSOR_VIRTUAL_AUTOMATIC(Animal, legs, int, -1)


DEFINE_METHOD_CONST(Animal, pet, void, (), VOID, {
	printf("You pet the animal.\n");
})


// Dog

struct Dog {
	char* name;
};


DEFINE_CLASS(Dog, (const char* name), (name), {
	SPECIALIZE(self, Animal);

	Dog* slot = (Dog*) calloc(1, sizeof(Dog));
	CLASS_PUSH(self, Dog, slot);

	METHOD_PUSH(self, Animal, Dog, speak);
	ACCESSOR_PUSH(self, Animal, Dog, legs);
	ACCESSOR_PUSH(self, Dog, Dog, name);

	SET(self, Animal, legs, 4);
	SET(self, Dog, name, name);
}, {
	printf("bye Dog\n");
	free(slot->name);
	free(slot);
})


DEFINE_METHOD_CONST_OVERRIDE(Dog, speak, void, (), VOID, {
	printf("Woof, I'm a dog named %s with %d legs.\n", GET(self, Dog, name), GET(self, Animal, legs));
})


// This example is a bit pointless, but it demonstrates how to call the superclass' method from an overridden method.
DEFINE_ACCESSOR_OVERRIDE(Dog, legs, int, -1, {
	return SUPER_GET(self, Animal, Dog, legs);
}, {
	if (legs > 4)
		legs = 4;
	SUPER_SET(self, Animal, Dog, legs, legs);
})


DEFINE_ACCESSOR_VIRTUAL(Dog, name, const char*, "", {
	return slot->name;
}, {
	free(slot->name);
	slot->name = strdup2(name);
})
