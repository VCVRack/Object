# [![](https://vcvrack.com/port.svg)](https://vcvrack.com/) VCV's Object system

*A flexible cross-language [ABI](https://en.wikipedia.org/wiki/Application_binary_interface)-stable [OOP](https://en.wikipedia.org/wiki/Object-oriented_programming) C library*


## Features and usage

There is a single polymorphic object type `Object*`.
It can be multiple classes.
```c
Object* dog = Dog_create();
Animal_is(dog); // true
Dog_is(dog); // true
```

You can call an object's virtual methods simply by calling a C function.
```c
Dog_bury(dog, bone);
Animal_speak(dog); // Woof!

// Call direct method without virtual method lookup
// Similar to dog->Animal::speak() in C++
Animal_speak_mdirect(dog); // Hello

// Or use C/C++ macros if you prefer DSL-like code
CALL(dog, Dog, bury, bone);
CALL(dog, Animal, speak); // Woof!
CALL_DIRECT(dog, Animal, speak); // Hello
```

Each class's data is encapsulated in a private/opaque struct unless exposed by getters/setters, which can be virtual or non-virtual.
```c
// legs property is stored in Animal class data
Animal_legs_get(dog); // 4, since Dog calls Animal_legs_set(dog, 4) upon specialization
Animal_legs_set(dog, 5); // Dog can override Animal_legs_set() to validate the value and perform custom behavior

// name property is stored in Dog class data
Dog_name_set(dog, "Fido");

// Or use macros if you prefer
GET(dog, Animal, legs); // 4
SET(dog, Animal, legs, 5);
SET(dog, Dog, name, "Fido");
```

You can specialize an object's type *after* instantiation, even if classes are unrelated.
```c
Poodle_specialize(dog); // dog is now an Animal, Dog, and Poodle
Poodle_is(dog); // true
// Objects cannot be un-specialized, since doing so could leave objects with invalid/impossible state.

// Or use macros if you prefer
SPECIALIZE(dog, Poodle);
IS(dog, Poodle); // true
```

Calling methods of the wrong class gracefully returns default/error values.
```c
Bird_is(dog); // false
Bird_fly(dog); // Nothing happens, no-op
Bird_wingspan_get(dog); // Returns a default/error value defined by Bird_wingspan_get() implementation, such as -1
```

Objects are reference-counted, and each shared owner must release their reference to avoid a memory leak.
When an object's last reference is released, each class's `finalize()` function is called in reverse order of specialization, where virtual methods are allowed to be called.
Then each class's `free()` is called in reverse order, where virtual methods are *not* allowed to be called.
```c
Object_release(dog);
```


## Declaring and defining classes

You can create your own object classes by including `Object/Object.h` and using its macros.

The `Animal` and `Dog` classes above can be declared by this `.h` header.
```c
// You can add constructor/initialization arguments if needed
CLASS(Animal, ());
METHOD_CONST_VIRTUAL(Animal, speak, void, ());
ACCESSOR_VIRTUAL(Animal, legs, int);

/** A domesticated wolf. Specializes Animal. */
CLASS(Dog, ());
METHOD_VIRTUAL(Dog, bury, void, (Object* thing));
ACCESSOR_VIRTUAL(Dog, name, const char*);
METHOD_CONST_OVERRIDE(Dog, speak, void, ());
```

See [examples/Animal.c](examples/Animal.c) for a possible implementation using `DEFINE_*` macros.


## ABI-stability

Without breaking your library's ABI (application binary interface), you can:
- Add, remove, and modify class data.
- Add virtual and non-virtual methods, getters, and setters.
- Convert non-virtual methods to virtual methods.
- Change class hierarchy, such as inserting a `Mammal` class between `Dog` and `Animal`, or making `Animal` specialize `Organism`, or making `Dog` not based on `Animal`.
	- Note that `Animal_speak(dog)` will do nothing (no-op) if `Dog` is no longer based on `Animal`.

Without breaking the ABI, you cannot:
- Remove classes.
- Remove virtual and non-virtual methods, getters, and setters.
- Convert virtual methods to non-virtual methods.
- Change method signatures, such as adding/removing an argument.


## Cross-language compatibility

Each virtual method such as `METHOD_VIRTUAL(Foo, bar, int, (int n))` declares the following C functions.
```c
int Foo_bar(Object* self, int n);
int Foo_bar_mdirect(Object* self, int n);
typedef int (*Foo_bar_m)(Object* self, int n);
```

Any language/environment with a C FFI interface can call these functions to interact with your library, including C++, Go, Rust, Zig, Java, C#, Python, PHP, Node, Ruby, Julia, Lua, etc.

The Object struct and each class's data structs are private/opaque and must not be accessed except by these functions.


## In this repo

- [Object.h](Object/Object.h) contains macros to declare and define your classes and methods, as well as runtime function declarations.
- [Object.cpp](src/Object.cpp) is a possible C++ implementation of the (tiny) runtime. Feel free to port this to C, Go, etc if you have a favorite map/dictionary implementation.
- [WeakObject.h](Object/WeakObject.h) defines a handle that holds a weak reference to an Object.
- [examples/](examples/) contains example programs that demonstrate usage and features.


## License

The contents of this repository are released into the public domain.

If you use this library in your project, we'd love to hear about it!
