# Riven Compiler `rivc`

<p align="center">
  <img src="https://img.shields.io/badge/language-C%2B%2B17-blue.svg" />
  <img src="https://img.shields.io/badge/version-1.0.0-green.svg" />
  <img src="https://img.shields.io/badge/license-MIT-lightgrey.svg" />
  <img src="https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-orange.svg" />
</p>

> **Riven** is a statically-typed, systems-level programming language with clean, expressive syntax. `rivc` is its complete compiler, transpiling Riven (`.rn`) source to optimized C++ and then to native binaries.

---

## Table of Contents
- [Features](#features)
- [Quick Start](#quick-start)
- [Language Syntax](#language-syntax)
- [Compiler Usage](#compiler-usage)
- [Building from Source](#building-from-source)
- [Examples](#examples)
- [Project Structure](#project-structure)
- [Architecture](#architecture)
- [Roadmap](#roadmap)

---

## Features

| Feature | Status |
|---|---|
| Lexer with `~~` line comments and `<< >>` block comments | ✅ |
| `riven core { }` mandatory entry point | ✅ |
| `Imprint(...)` for output | ✅ |
| `scanq(var)` for user input | ✅ |
| `firm` constants | ✅ |
| `if` / `altif` / `else` conditionals | ✅ |
| `+>` increment / `-<` decrement operators | ✅ |
| `return fin` / `return fin value` | ✅ |
| `consistof "file.rh"` imports | ✅ |
| `while`, `for`, `do-while` loops | ✅ |
| `struct` with fields and methods | ✅ |
| `class` with `pub:` / `priv:` / `prot:` access | ✅ |
| Single inheritance via `class B : A` | ✅ |
| `enum` + `match` pattern matching | ✅ |
| `try` / `catch` / `throw` error handling | ✅ |
| `unsafe { }` blocks | ✅ |
| `async { }` blocks | ✅ |
| `new` / `delete` memory management | ✅ |
| `namespace` / `trait` / `impl` | ✅ |
| Lambda expressions | ✅ |
| Ternary operator | ✅ |
| Full type system: `int`, `float`, `char`, `bool`, `string`, pointers, arrays | ✅ |
| Semantic analysis with scope chains | ✅ |
| Native binary output via C++ backend | ✅ |

---

## Quick Start

```bash
# 1. Clone
git clone https://github.com/yourusername/riven-compiler.git
cd riven-compiler

# 2. Build
./build.sh

# 3. Run Hello World
./build/rivc examples/hello.rn -o hello && ./hello
```

Output:
```
Hello, World!
```

---

## Language Syntax

### Hello World
```riven
riven core {
    Imprint("Hello, World!");
    return fin;
}
```

### Comments
```riven
~~ This is a line comment

<< This is a
   multi-line block comment >>
```

### File Extension
- Source files: `.rn`
- Header files: `.rh` or `.rvh`

### Imports
```riven
consistof "utils.rh"
consistof utils.rh
```

### Constants
```riven
firm int MAX = 100;
firm string APP = "Riven";
```

### Variables
```riven
int x = 42;
float pi = 3.14;
bool active = true;
string name = "Riven";
mut int counter = 0;
let int y = 10;
```

### Output & Input
```riven
Imprint("Hello, ", name, "!");    ~~ prints with newline
scanq(x);                          ~~ reads user input into x
```

### Increment / Decrement
```riven
x+>;     ~~ x++ (Riven increment)
x-<;     ~~ x-- (Riven decrement)
```

### Conditionals
```riven
if (score >= 90) {
    Imprint("A");
} altif (score >= 80) {
    Imprint("B");
} altif (score >= 70) {
    Imprint("C");
} else {
    Imprint("F");
}
```

### Loops
```riven
~~ While
while (i < 10) { i+>; }

~~ For
for (int i = 0; i < 10; i+>) {
    Imprint(i);
}

~~ Do-while
do {
    Imprint(x);
    x+>;
} while (x < 5);
```

### Functions
```riven
int factorial(int n) {
    if (n <= 1) return fin 1;
    return fin n * factorial(n - 1);
}
```

### Return
```riven
return fin;          ~~ return (void / 0 in main)
return fin value;    ~~ return a value
```

### Structs
```riven
struct Point {
    int x;
    int y;

    void print() {
        Imprint("(", x, ", ", y, ")");
    }
}
```

### Classes
```riven
class Animal {
pub:
    string name;
    int age;

pub:
    void speak() {
        Imprint(name, " speaks!");
    }
}

class Dog : Animal {
pub:
    string breed;
pub:
    void speak() {
        Imprint(name, " says Woof!");
    }
}
```

### Enums & Match
```riven
enum Direction { NORTH, SOUTH, EAST, WEST }

match (dir) {
    case Direction::NORTH: { Imprint("North"); }
    case Direction::SOUTH: { Imprint("South"); }
    default:               { Imprint("Other"); }
}
```

### Error Handling
```riven
try {
    if (x < 0) throw "Negative value!";
    Imprint("OK");
} catch (string e) {
    Imprint("Error: ", e);
}
```

### Unsafe & Async
```riven
unsafe {
    int* ptr = new int(42);
    Imprint(*ptr);
    delete ptr;
}

async {
    Imprint("Running asynchronously");
}
```

### Namespaces
```riven
namespace Math {
    int square(int x) { return fin x * x; }
}
```

### Traits & Impl
```riven
trait Printable {
    void print();
}

impl Point : Printable {
    void print() {
        Imprint("Point");
    }
}
```

### Lambda
```riven
lambda(int x) -> int { return fin x * x; }
```

---

## Compiler Usage

```
Usage: rivc [options] <input.rn>

Options:
  -o <file>        Output file (default: <stem> or out.cpp)
  --emit-cpp       Emit C++ source instead of compiling to binary
  --emit-tokens    Dump the lexer token stream and exit
  --no-semantic    Skip semantic analysis pass
  --cc <compiler>  C++ compiler backend (default: g++)
  --cflags <flags> Extra C++ compiler flags
  -v, --verbose    Verbose multi-stage output
  -h, --help       Show help
  --version        Show version
```

### Examples

```bash
# Compile to native binary
rivc hello.rn

# Compile with custom output name
rivc hello.rn -o myapp

# Inspect generated C++
rivc hello.rn --emit-cpp

# Dump token stream
rivc hello.rn --emit-tokens

# Use clang++ as backend
rivc hello.rn --cc clang++

# Verbose compilation pipeline
rivc hello.rn -v
```

---

## Building from Source

### Prerequisites
- C++17-compatible compiler (GCC 9+ or Clang 10+)
- CMake 3.16+

### Linux / macOS
```bash
./build.sh           # Release build
./build.sh --debug   # Debug build
./build.sh --clean   # Clean + rebuild
```

### Manual CMake
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Windows (MSVC)
```cmd
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

---

## Examples

| File | Description |
|---|---|
| `examples/hello.rn` | Hello World |
| `examples/features.rn` | Comprehensive feature demo |
| `tests/test_basic.rn` | Variables, loops, increment/decrement |
| `tests/test_functions.rn` | Functions, recursion, altif chains |
| `tests/test_structs.rn` | Structs, classes, inheritance |
| `tests/test_enums.rn` | Enums and match statements |
| `tests/test_errors.rn` | Try/catch/throw error handling |
| `tests/test_unsafe.rn` | Unsafe blocks and pointer operations |

---

## Project Structure

```
riven-compiler/
├── CMakeLists.txt          # CMake build configuration
├── build.sh                # Quick build script
├── README.md               # This file
├── LICENSE                 # MIT License
├── include/
│   ├── Token.h             # Token types + keyword map
│   ├── Lexer.h             # Lexer interface
│   ├── AST.h               # Full AST node hierarchy (40+ node types)
│   ├── Parser.h            # Pratt parser interface
│   ├── SemanticAnalyzer.h  # Semantic analysis + scope chains
│   ├── CodeGen.h           # C++ code generator interface
│   ├── riven_std.rh        # Riven standard library (Riven declarations)
│   └── riven_std.h         # Riven standard library (C++ implementation)
├── src/
│   ├── main.cpp            # Compiler driver (CLI)
│   ├── Lexer.cpp           # Full lexer implementation
│   ├── Parser.cpp          # Full recursive-descent + Pratt parser
│   ├── SemanticAnalyzer.cpp # Semantic analysis implementation
│   └── CodeGen.cpp         # Code generation (Riven → C++)
├── examples/
│   ├── hello.rn            # Hello World
│   └── features.rn         # Full feature showcase
└── tests/
    ├── test_basic.rn
    ├── test_functions.rn
    ├── test_structs.rn
    ├── test_enums.rn
    ├── test_errors.rn
    └── test_unsafe.rn
```

---

## Architecture

```
Source (.rn)
    │
    ▼
┌─────────┐     tokens      ┌─────────┐     AST       ┌──────────────────┐
│  Lexer  │ ─────────────▶  │ Parser  │ ────────────▶  │ SemanticAnalyzer │
└─────────┘                 └─────────┘                └──────────────────┘
                                                                │
                                                           annotated AST
                                                                │
                                                                ▼
                                                       ┌─────────────┐
                                                       │  CodeGen    │
                                                       └─────────────┘
                                                                │
                                                          C++ source
                                                                │
                                                                ▼
                                                       ┌─────────────┐
                                                       │  g++/clang  │
                                                       └─────────────┘
                                                                │
                                                         Native Binary
```

### Compiler Passes

| Pass | Module | Description |
|---|---|---|
| 1 | `Lexer` | Tokenizes source → token stream |
| 2 | `Parser` | Pratt recursive-descent → AST |
| 3 | `SemanticAnalyzer` | Scope resolution, type checking, warnings |
| 4 | `CodeGenerator` | AST → C++17 source (7 ordered passes) |
| 5 | `g++/clang++` | C++ → native binary |

---

## Roadmap

- [ ] Direct LLVM IR backend (skip C++ intermediary)
- [ ] Generics / templates (`template<T>`)
- [ ] Module system
- [ ] Package manager (`riven pkg`)
- [ ] LSP (Language Server Protocol) support
- [ ] Standard library expansion
- [ ] WASM target
- [ ] Debugger integration

---

## License

MIT License — see [LICENSE](LICENSE) for details.
