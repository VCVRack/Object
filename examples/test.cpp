#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
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
		assert(cppDog.use_count() == 1);

		cppDog->speak();
		cppDog->name = "Ralph";
		cppDog->speak();
		cppDog->legs = 3;
		cppDog->speak();

		cppDogWeak = cppDog;
		// Weak handles don't increase the Object's reference count
		assert(cppDogWeak.use_count() == 1);

		Ref<cpp::Dog> cppDog2 = cppDog;
		assert(cppDog.use_count() == 2);
	}

	assert(cppDogWeak.use_count() == 0);



	// C++ Animal proxy example
	printf("\nC++ Animal proxy example\n");

	Object* dog = Dog_create("Toto");
	assert(GET(dog, Object, refs) == 1);

	{
		// Only one proxy object is allowed per context (language or plugin), so use castOrCreate to guarantee this.
		cpp::Dog* cppDog = CppWrapper_castOrCreate<cpp::Dog>(dog);
		// Proxies don't increase the Object's reference count
		assert(GET(dog, Object, refs) == 1);
		cppDog->speak();

		// Refs increase reference count
		Ref<cpp::Dog> cppDogRef = cppDog;
		assert(GET(dog, Object, refs) == 2);
		cppDogRef->speak();

		// Proxy is valid until Object is deleted.
	}

	assert(GET(dog, Object, refs) == 1);
	Object_release(dog);

	return 0;
}