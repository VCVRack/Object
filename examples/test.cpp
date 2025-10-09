#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include "Animal.hpp"


int main() {
	// C Animal example
	printf("\nC Animal example\n");

	// Construct an Animal
	Object* animal = Animal_create();

	// Non-virtual method, cannot be overridden
	Animal_pet(animal); // "You pet the animal."

	// Virtual method
	Animal_speak(animal); // "I'm an animal with 0 legs."

	// We can specialize existing Objects even after they are created.
	// If the object is already a Dog, this fails gracefully.
	Dog_specialize(animal, "Dogbert");

	// Call virtual setters
	Dog_name_set(animal, "Fido");
	Animal_legs_set(animal, 3);

	// Call virtual method, which calls Dog's overridden implementation.
	Animal_speak(animal); // "Woof, I'm a dog named Fido with 3 legs."

	// All Objects must be released.
	Object_release(animal);



	// C++ Animal example
	printf("\nC++ Animal example\n");

	WeakRef<cpp::Dog> cppDogWeak;

	{
		Ref<cpp::Dog> cppDog(std::in_place, "Gromit");
		printf("%lu refs\n", cppDog.use_count());

		cppDog->speak();
		cppDog->name = "Ralph";
		cppDog->speak();
		cppDog->legs = 3;
		cppDog->speak();

		cppDogWeak = cppDog;

		printf("%lu weak refs\n", cppDogWeak.use_count());

		Ref<cpp::Dog> cppDog2 = cppDog;
		printf("%lu refs\n", cppDog.use_count());
	}

	printf("%lu weak refs\n", cppDogWeak.use_count());



	// C++ Animal proxy example
	printf("\nC++ Animal proxy example\n");

	Object* dog = Dog_create("Toto");
	printf("%lu refs\n", Object_refs_get(dog));

	{
		Ref<cpp::Dog> cppDog(std::in_place, dog);
		printf("%lu refs\n", Object_refs_get(dog));

		cppDog->speak();
	}

	printf("%lu refs\n", Object_refs_get(dog));

	Object_release(dog);

	return 0;
}