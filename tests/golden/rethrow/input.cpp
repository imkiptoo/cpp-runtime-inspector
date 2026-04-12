// Exception rethrow test
// catch (...) { throw; } and propagation up the call stack.

#include <stdexcept>

void inner() {
    throw std::runtime_error("inner");
}

void middle() {
    try {
        inner();
    } catch (...) {
        // log + rethrow
        throw;
    }
}

int main() {
    int caught = 0;
    try {
        middle();
    } catch (const std::runtime_error&) {
        caught = 1;
    }
    return 0;
}
