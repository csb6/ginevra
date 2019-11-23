# Ginevra++

This is a collection of two implementations of a very basic C-like preprocessor that operates on any text file with a `.cpp` or `.h` extension. It only implements the `#define` preprocessor directive, but it tokenizes the content of the file, replaces all tokens that match symbols
defined in the `#define` directives, and prints out the result to the console.

Basic error handling is included: it checks that the given file exists and has the right file extension, and the program will print errors if a token ends unexpectedly.

Both programs are based on the ginevra preprocessor implemented in Arthur Pyster's book
*Compiler Design and Construction*, but both take advantage of C++'s standard library to
simplify the implementation immensely. Also, since the book's implementation of the symbol
table used a linked list instead of a hash map, so this implementation may in fact be
faster than the C version found in the book for programs with large numbers of `#define`s!

This project was more challenging than I expected, but it helped me gain experience
in C++'s fstreams, as well as when object-oriented programming does/doesn't work well

### ginevra++.cpp

This is the more object-oriented version of the program, although it is quite a bit longer
than `better.cpp` and much more complicated. However, there is slightly more error checking,
and if this were to be expanded into a full compiler, it would probably be more maintainable
and extensible.

### better.cpp

After writing `ginevra++.cpp` and feeling dissatisfied with its complexity, I rewrote
the program in a procedural form, removing the separate constants representing tokens
and greatly simplifying the conditional logic of the parser while still preserving nearly
all of the features. The code is not the prettiest, but I think that it is much more easy to
reason about than the OOP version in `ginevra++.cpp`.

## Usage

Building `ginevra++.cpp` is built with C++17, although it should also be able to compile.
under C++14. Run `./build-ginevra++.sh`, then run `./ginevra++ [some .cpp or .h file]`

Building `better.cpp` requires at least C++17. Run `./build.sh`, then run
`./better [some .cpp or .h file]`