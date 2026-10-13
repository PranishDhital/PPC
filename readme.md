# PD Compiler

PD is a small programming language project. Its compiler reads `.pd` files, parses them, generates x86-64 assembly, and builds a Windows executable.

## Requirements

- CMake 3.20 or newer
- A C++20 compiler
- NASM
- GCC (MinGW-w64 on Windows)

Make sure `cmake`, `nasm`, and `gcc` are available on your `PATH`.

## Build

```sh
cmake -S . -B build
cmake --build build
```

The compiler executable is created in `build/bin` (or a configuration subfolder such as `build/bin/Debug`).

## Compile a program

```sh
ppc path/to/program.pd
```

On Windows, run `ppc.exe`. The compiler writes the finished `.exe` beside the source file. Run that executable to start your program.

To show the compiler version:

```sh
ppc --version
```

## Language example

```pd
let age;
string name;

print("Your name: ");
input(name);
print("Your age: ");
input(age);

if (age >= 18) {
    printnl("Hello, " + name + ". You are an adult.");
} else {
    printnl("Hello, " + name + ". You are under 18.");
}

exit(0);
```

`let` declares a number and defaults to `0`. `string` declares a string and defaults to empty. Both can also be initialized, for example `let score = -2.5;` or `string greeting = "Hello";`.

## Supported features

- `let` and `string` declarations
- `print`, `printnl`, `input`, and `exit`
- `if` / `else if` / `else`, `while`, and `for`
- Arithmetic: `+`, `-`, `*`, `/`; comparisons: `<`, `<=`, `>`, `>=`, `==`, `!=`; negation: `!`
- Prefix and postfix `++` and `--`
- String concatenation and string equality/inequality
- `//` and `/* ... */` comments

See [`Test/example.pd`](Test/example.pd) for a syntax showcase and [`docs/syntax.md`](docs/syntax.md) for more details.

`return` is reserved by the lexer but is not yet supported as a statement. Logical `&&` and `||` are not supported.
