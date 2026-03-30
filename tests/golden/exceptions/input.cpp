// Exception test
// Verify throw and catch are properly tracked

#include <stdexcept>

int divide(int a, int b) {
    if (b == 0) {
        throw std::runtime_error("division by zero");
    }
    return a / b;
}

int main() {
    int result = 0;

    try {
        result = divide(10, 2);  // 5
        result = divide(10, 0);  // throws
    } catch (const std::runtime_error& e) {
        result = -1;
    }

    return result;
}
