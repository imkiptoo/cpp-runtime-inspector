// constexpr test
// Compile-time evaluated function reused at runtime.

constexpr int square(int x) {
    return x * x;
}

constexpr int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

int main() {
    constexpr int s = square(4);    // evaluated at compile time
    constexpr int f = factorial(5); // 120

    int x = 6;
    int s2 = square(x);             // same function at runtime
    return 0;
}
