// std::function test
// Verify engaged-vs-empty status flips as the target is assigned and reset.

#include <functional>

int doubled(int x) { return x * 2; }

int main() {
    std::function<int(int)> f;          // empty
    int has1 = f ? 1 : 0;

    f = doubled;                         // engaged with free function
    int has2 = f ? 1 : 0;
    int v = f(5);

    f = nullptr;                         // empty again
    int has3 = f ? 1 : 0;

    f = [](int x) { return x + 1; };     // engaged with lambda
    int has4 = f ? 1 : 0;
    int w = f(10);

    return 0;
}
