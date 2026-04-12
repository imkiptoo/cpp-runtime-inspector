// Lambda captures test
//
// Verify the trace shows every interesting lambda capture flavour:
//   - capture by value (primitive)
//   - capture by reference (primitive)
//   - capture by value (user struct)
//   - capture by reference (user struct)
//   - default by-copy `[=]`
//   - default by-reference `[&]`
//
// Each captured variable must appear in the closure's encoded fields
// under its source-level name, not as the empty-string placeholder
// that Clang emits for synthesized lambda fields.

struct Point {
    int x;
    int y;
};

int main() {
    int a = 5;
    int b = 10;
    Point p{};
    p.x = 100;
    p.y = 200;

    // [a, &b] : a by value, b by reference.
    auto f1 = [a, &b]() {
        return a + b;
    };

    // [p] : struct captured by value (full copy in the closure).
    auto f2 = [p]() {
        return p.x + p.y;
    };

    // [&p] : struct captured by reference.
    auto f3 = [&p]() {
        return p.x - p.y;
    };

    // [=] : default by-copy. Captures everything used by value.
    auto f4 = [=]() {
        return a + p.x;
    };

    // [&] : default by-reference. Same backing memory as outer scope.
    auto f5 = [&]() {
        return b + p.y;
    };

    int r1 = f1();   // 5 + 10 = 15
    b = 99;          // observe through f1's reference capture: r1b = 5 + 99
    int r1b = f1();
    int r2 = f2();   // 100 + 200 = 300
    p.x = 7;         // f2 unaffected (by-value snapshot), f3 sees the new x
    int r2b = f2();
    int r3 = f3();   // 7 - 200 = -193
    int r4 = f4();   // 5 + 100 = 105 (f4 captured p.x = 100 at construction)
    int r5 = f5();   // 99 + 200

    return 0;
}
