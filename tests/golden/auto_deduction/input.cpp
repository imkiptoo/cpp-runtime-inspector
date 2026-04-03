// auto type deduction test
// Verify auto for primitives, references, and value semantics.

int main() {
    auto i = 42;          // int
    auto d = 3.14;        // double
    auto c = 'X';         // char

    int x = 10;
    auto& r = x;          // int&
    r = 20;

    const auto k = 100;   // const int
    return 0;
}
