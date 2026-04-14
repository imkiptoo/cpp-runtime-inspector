// Test stdout capture with mixed output styles

#include <iostream>

int main() {
    // Multiple outputs on same line
    std::cout << "Hello";
    std::cout << " ";
    std::cout << "World";
    std::cout << "!" << std::endl;

    // Numeric output
    int a = 42;
    std::cout << "Value: " << a << "\n";

    return 0;
}
