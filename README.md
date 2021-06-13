# SimpleRandomWrapper
A simple wrapper for the random library in C++. Uses C++17.

## About this wrapper
This wrapper is intended to be used as a convienience for programmers who wish to generate high-quality random numbers or randomize elements with a clean, easy to use API.

## Setup
All you need to do is include ``Random.cpp`` and ``Random.hpp`` in your project. That's it!

Using the wrapper is easy. Just include ``Random.hpp`` at the top of any file you wish to use the wrapper. No need for any special runtime setup. All of the setup is done automatically when you generate your first number.

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
The ``Random`` class is a singleton that is statically allocated. It includes a ``mt1337_64`` instance which is the generator used by the class. When you call any of the static functions, if it is your first time calling a static function from ``Random``, it will automatically setup the generator with a seed from ``std::random_device``. You can manually set this seed at any time with ``Random::Seed()``. Because of how singletons work in C++, you should be able to use this wrapper across multiple threads.

## Features
### Generating numbers
```cpp 
int64_t Random::GetInt64(int64_t begin, int64_t end);
int32_t Random::GetInt32(int32_t begin, int32_t end);
int16_t Random::GetInt16(int16_t begin, int16_t end);
int8_t Random::GetInt8(int8_t begin, int8_t end);
...
```

These all generate a random integer of a different size (64-bit, 32-bit, 16-bit, 8-bit) between begin and end (inclusive), using a uniform distribution. There are both signed and unsigned variants.

```cpp 
double Random::GetDouble(double min, double max);
float Random::GetFloat(float min, float max);
```

These generate a random floating-point number of double or single precision respectively, between min and max (exclusive), using a uniform distribution.

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

There is small performance overhead with every call to ``Random::Get*()``. This is because of the construction of ``uniform_*_distribution``s which are designed to be used multiple times, but are only used to generate that one random number for each call. This isn't a huge deal if you are generating only a small number of random numbers. However, if you try generating a large number of them, the performance penalty will be noticable. To counteract this, the wrapper comes with functions to generate ``std::array``s of numbers so that ``uniform_*_distributions`` can be reused.

```cpp
template<size_t N> std::array<int, N>&& Random::GetIntArray(int begin, int end);
template<size_t N> std::array<double, N>&& Random::GetDoubleArray(double min, double max);
template<size_t N> std::array<float, N>&& Random::GetFloatArray(float min, float max);
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

## End
Not sure what to put here, but thanks for taking the time to check this out. 
