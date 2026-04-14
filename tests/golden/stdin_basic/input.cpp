// Basic stdin capture test
// Reads a number from stdin and doubles it

#include <iostream>

int main() {
    int x = 0;
    std::cin >> x;
    int doubled = x * 2;
    std::cout << "Result: " << doubled << std::endl;
    return 0;
}
