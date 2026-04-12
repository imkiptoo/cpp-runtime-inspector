// Structured bindings test
// Decompose a pair / array via auto [a, b] = ...

#include <utility>

struct Point {
    int x;
    int y;
};

int main() {
    auto p = std::make_pair(1, 2);
    auto [a, b] = p;

    Point pt{10, 20};
    auto [px, py] = pt;

    int arr[3] = {7, 8, 9};
    auto [r, s, t] = arr;

    return 0;
}
