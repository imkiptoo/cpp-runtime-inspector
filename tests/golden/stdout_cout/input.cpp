// Test stdout capture with std::cout

#include <iostream>

int main() {
    int x = 5;
    std::cout << "x = " << x << std::endl;

    int y = 10;
    std::cout << "y = " << y << std::endl;

    int sum = x + y;
    std::cout << "sum = " << sum << std::endl;

    return 0;
}
