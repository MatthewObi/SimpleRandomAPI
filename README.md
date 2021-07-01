# SimpleRandomAPI
A simple header-only API for random number generation. Uses C++17.

## About this API
This API is intended to be used as a convienience for programmers who wish to generate high-quality random numbers or randomize elements while being as easy to use as APIs for random number generation from other languages.

This API uses [pcg32](http://www.pcg-random.org/) for random number generation, which performs much faster and is much more deterministic on multiple platforms than C++'s builtin Mersenne Twister generator. It specifically uses a C++ implementation, which you can find [here](https://github.com/imneme/pcg-cpp).

## Setup
All you need to do is include ``Random.hpp`` from the single_include directory in your project. That's it! Alternatively, you can download and include both ``Random.hpp`` in the root directory and the ``pcg`` directory in your project. It is ultimately up to you. Don't forget to include the licenses in your project as well.

Using the API is easy. Just include ``Random.hpp`` at the top of any file you wish to use the API. No need for any special runtime setup. All of the setup is done automatically when you generate your first number.

```cpp
#include "Random.hpp"
...

int main() 
{
    int randomNum = Random::GetInt(0, 10);
    auto randomFloat = Random::GetFloat(-38.0f, 38.0f);
    
    int diceRoll = Random::GetInt(1, 6);
    int twoDiceRoll = Random::GetInt(1, 6) + Random::GetInt(1, 6);
    if(Random::Chance(0.33)) {
        std::cout << "33% percent chance this will appear.";
    }
    
    std::string base64str = Random::GetString(Random::Charset::Base64, 16);
    
    auto diceRollArray = Random::GetIntArray<10>(1, 6);
    for(auto roll : diceRollArray) {
        std::cout << roll << ", ";
    }
    std::cout << "\n";
}
```
## How it works
The ``Random`` class is a singleton that is statically allocated. It includes a ``pcg32`` instance which is the generator used by the class. When you call any of the static functions, if it is your first time calling a static function from ``Random``, it will automatically setup the generator with a seed sequence from ``std::random_device``. You can manually set this seed at any time with ``Random::Seed()``. Because of how singletons work in C++, you should be able to use this API across multiple threads.

## Features
### Generating numbers
```cpp 
template<typename Int_t>
Int_t Random::GetInt(Int_t begin, Int_t end);
...
```

This generates a random integer of a chosen size and signedness (64-bit, 32-bit, 16-bit, 8-bit; signed or unsigned) between begin and end (inclusive), using a uniform distribution. You can call the function without template arguments and it will deduce the type using the type of the arguments you pass in.

```cpp
auto myInt0 = Random::GetInt<int16_t>(1, 6); //myInt0 is int16_t
auto myInt1 = Random::GetInt(1ll, 6ll); //myInt1 is of type long long
auto myInt2 = Random::GetInt(1u, 6u); //myInt is of type unsigned int
```

```cpp 
template<typename Float_t>
Float_t Random::GetFloat(Float_t min, Float_t max);
```

This generates a random floating-point number of double or single precision depending on what type you use for the arguments, between min and max (exclusive), using a uniform distribution.

### Binary Probability
```cpp 
bool Random::Chance(double pct);
```

Generates a random number between ``0.0`` and ``1.0`` (exclusive), and if that number is less than ``pct``, then the function will return ``true``. Otherwise, it will return false. Used to create a discrete binary probability, like in the following example:

```cpp
void onEnemyDeath()
{
    if(Random::Chance(0.1)) {
        DropRareLoot(); //10% chance
    } else {
        DropCommonLoot(); //90% chance
    }
}
```

A ``pct`` of ``1.0`` or higher will always return ``true``. Conversly, a ``pct`` of ``0.0`` or lower will always return ``false``.

```cpp 
bool Random::Chance(int n, int d);
```

Used the same as the above, but formulates the probability as an integer fraction instead of a floating-point decimal. Generates an integer between ``1`` and ``d`` (inclusive) and returns ``true`` if the number generated is less than or equal to ``n``.

```cpp
void onFireHit()
{
    if(Random::Chance(1, 12)) {
        Burn(); // 1/12 chance or ~8.333% chance
    }
}
```

If ``n`` is ``0`` or less, the function will always return ``false``. Conversly, if ``n`` is equal to or greater than ``d``, the function will always return ``true``.

### Arrays

There is small performance overhead with every call to ``Random::Get*()``. This is because of the construction of ``uniform_*_distribution``s which are designed to be used multiple times, but are only used to generate that one random number for each call. This isn't a huge deal if you are generating only a small number of random numbers. However, if you try generating a large number of them, the performance penalty will be noticable. To counteract this, the API comes with functions to generate ``std::array``s of numbers so that ``uniform_*_distributions`` can be reused.

```cpp
template<size_t N, typename Int_t> std::array<Int_t, N>&& Random::GetIntArray(Int_t begin, Int_t end);
template<size_t N, typename Float_t> std::array<Float_t, N>&& Random::GetFloatArray(Float_t min, Float_t max);
```

Notice that the arrays are passed by rvalue-reference. That means you can assign the result to a value variable and use it directly on the stack. Also, because it is a ``std::array``, you can use the standard iterator syntax on it. There is no default array size, so you must supply it using angle brackets:

```cpp
auto myIntArray = Random::GetIntArray<10>(0, 9); //10 ints between 0 and 9.
```

The only drawback to using these functions is that you must use the same ``begin``/``end`` or ``min``/``max`` range for each number generated in the array.

### Strings

The wrapper also comes with functions for generating strings using a charset. ``Random::GetString`` uses a ``std::string_view`` for its charset, so you can supply the function with a regular string or a raw C string.

```cpp
std::string Random::GetString(std::string_view charset, const size_t length);
```

```cpp
std::string myString = Random::GetString("abc", 4);
std::cout << "My string:" << myString << "\n";
```
```sh
My string: bacb
```

You can also used a predefined charset. The wrapper comes bundled with a few under ``Random::Charset``.

```cpp
std::string myString = Random::GetString(Random::Charset::Alpha, 6);
std::cout << "My string:" << myString << "\n";
```
```sh
My string: znTaxW
```

> Disclaimer: I have no idea if it is a good idea to use this to generate sensitive strings, so I hold no liability if you use this and screw up.

### Shuffle

The API also has a few functions for shuffling containers.

```cpp
template<typename Iter_t>
void Random::Shuffle(Iter_t begin, Iter_t end);
template<typename Container_t>
void Random::Shuffle(Container_t& container);
template<typename Container_t>
Container_t&& Random::ShuffleCopy(const Container_t& container);
```

``Random::Shuffle`` shuffles a given container in place. One version of the function uses iterators that the user passes in, while the other uses the container itself. For that version, the container must adhere to C++ container standards by having standard iterators ``begin()`` and ``end()``. If you do not wish to modify an existing container, you can use ``Random::ShuffleCopy`` which will create a copy of the container that is then shuffled.

## End
Not sure what to put here, but thanks for taking the time to check this out. Let me know if you decide to use this in a project of yours. I'd love to hear about it. Constructive criticism is always appriciated. This is the first time I've released an API.