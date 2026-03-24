# Supported C++ Language Subset

C++ Runtime Inspector supports a practical subset of C++ suitable for
introductory and intermediate programming courses.

## Fully Supported Features

### Primitives

- **Integers**: `int`, `short`, `long`, `long long` (signed and unsigned)
- **Floating point**: `float`, `double`, `long double`
- **Boolean**: `bool`
- **Character**: `char`, `signed char`, `unsigned char`
- **C-strings**: `const char*`, `char*`

### Variables

- **Local variables**: Stack-allocated in functions
- **Global variables**: File-scope, namespace-scope
- **Static variables**: Function-local static
- **References**: `T&`, `const T&`
- **Pointers**: `T*`, `const T*`, pointer arithmetic

### Control Flow

- **Conditionals**: `if`, `else`, `switch`
- **Loops**: `for`, `while`, `do-while`, range-based `for`
- **Branching**: `break`, `continue`, `return`
- **Exceptions**: `throw`, `try`/`catch` (basic support)

### Functions

- **Free functions**: Regular and inline
- **Member functions**: Methods
- **Constructors/Destructors**: Including implicit
- **Function overloading**: Multiple signatures
- **Default parameters**: Supported

### User-Defined Types

- **Structs**: POD and with methods
- **Classes**: With access specifiers
- **Single inheritance**: Base class fields tracked
- **Polymorphism**: Virtual functions (vtable hidden)
- **Enums**: Scoped (`enum class`) and unscoped
- **Unions**: All members displayed

### Arrays

- **Fixed-size arrays**: `int arr[10]`
- **Multi-dimensional**: `int matrix[3][4]`
- **Arrays of objects**: `Foo arr[5]`

### Dynamic Memory

- **new/delete**: Single objects
- **new[]/delete[]**: Arrays
- **malloc/free**: Via LD_PRELOAD shim
- **Use-after-free detection**: Dangling pointer tracking
- **Leak detection**: At program exit

### STL Containers (Tier 4)

- **std::vector<T>**: Content displayed as array
- **std::string**: Content displayed as string
- **std::array<T, N>**: Fixed-size array
- **std::pair<T1, T2>**: Displayed as struct
- **std::unique_ptr<T>**: Pointer with heap tracking
- **std::shared_ptr<T>**: Pointer with heap tracking

### Templates

- **Function templates**: Instantiations traced
- **Class templates**: Member functions traced
- **Lambda expressions**: With capture tracking

### Operators

- **Arithmetic**: `+`, `-`, `*`, `/`, `%`
- **Comparison**: `==`, `!=`, `<`, `<=`, `>`, `>=`
- **Logical**: `&&`, `||`, `!`
- **Bitwise**: `&`, `|`, `^`, `~`, `<<`, `>>`
- **Assignment**: `=`, `+=`, `-=`, `*=`, `/=`, etc.
- **Increment/Decrement**: `++`, `--` (prefix and postfix)

## Partially Supported Features

### STL Containers (Limited)

| Container       | Support Level                    |
|-----------------|----------------------------------|
| `std::map`      | Placeholder (tree traversal TBD) |
| `std::set`      | Placeholder (tree traversal TBD) |
| `std::optional` | Placeholder                      |
| `std::variant`  | Not supported                    |
| `std::tuple`    | Not supported                    |

### Inheritance

| Feature                | Support Level           |
|------------------------|-------------------------|
| Single inheritance     | Fully supported         |
| Multiple inheritance   | Warning, skipped        |
| Virtual inheritance    | Warning, skipped        |
| Diamond inheritance    | Not supported           |

### Exceptions

| Feature              | Support Level                |
|----------------------|------------------------------|
| `throw`              | Event emitted                |
| `catch`              | Event emitted on entry       |
| Stack unwinding      | Destructor calls tracked     |
| Exception rethrowing | Supported                    |
| `noexcept`           | Not validated                |

## Unsupported Features

These features will cause warnings or be skipped entirely:

### Language Features

- **Multiple inheritance** - Diagnose and refuse
- **Virtual inheritance** - Diagnose and refuse
- **Coroutines** (`co_await`, `co_yield`) - Diagnose and refuse
- **Concepts and constraints** - Compiles but not visualized
- **C++20 modules** - Stick with `#include`
- **Threading** (`std::thread`, `std::async`) - Warning emitted
- **RTTI** (`dynamic_cast`, `typeid`) - Not tracked

### Memory Features

- **Variable-length arrays (VLAs)** - Warning emitted
- **Custom allocators** - Falls back to raw bytes
- **Placement new** - Not tracked
- **Memory-mapped I/O** - Not accessible

### Optimization Features

- **Optimization levels above `-O0`** - May produce incorrect traces
- **Inline assembly** - Not instrumented
- **Compiler intrinsics** - Not tracked

## Platform Restrictions

| Platform     | Support Level                       |
|--------------|-------------------------------------|
| Linux x86_64 | Fully supported (libstdc++)         |
| Linux ARM64  | Should work (untested)              |
| macOS        | Supported (libc++ with limitations) |
| Windows      | Not supported (no MSVC)             |

## Compiler Requirements

- **Clang 17, 18, or 19** required
- **C++17 standard** minimum
- **libstdc++** preferred (GCC 11-13)
- **libc++** limited support (macOS)

## Recommendations for Users

### Best Practices

1. **Use simple types** - Prefer primitives and simple structs
2. **Avoid complex templates** - Especially nested STL containers
3. **Single inheritance only** - No MI or virtual inheritance
4. **No threading** - Single-threaded programs only
5. **Compile with `-O0`** - Required for correct tracing

### Example Program (Ideal)

```cpp
#include <vector>

struct Node {
    int value;
    Node* next;
};

int sum(const std::vector<int>& v) {
    int total = 0;
    for (int x : v) {
        total += x;
    }
    return total;
}

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    int result = sum(numbers);

    Node* head = new Node{10, nullptr};
    head->next = new Node{20, nullptr};

    delete head->next;
    delete head;

    return 0;
}
```

### Example Program (Problematic)

```cpp
#include <thread>
#include <map>

class Base1 { virtual void f() {} };
class Base2 { virtual void g() {} };
class Derived : public Base1, public Base2 {};  // MI - not supported

int main() {
    std::thread t([]{ /* ... */ });  // Threading - not supported
    t.join();

    std::map<std::string, int> m;    // Map - limited support
    m["key"] = 42;

    return 0;
}
```

## Version History

| Version | Added Support                      |
|---------|-----------------------------------|
| Tier 1  | Primitives, pointers, references  |
| Tier 2  | Structs, enums, arrays            |
| Tier 3  | Heap tracking, new/delete         |
| Tier 4  | STL containers, templates         |
| Tier 5  | Exceptions, compound operators    |
| Tier 6  | Production polish                 |
