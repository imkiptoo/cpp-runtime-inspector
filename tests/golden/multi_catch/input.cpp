// Multiple catch handlers test
// Verify the matching handler is selected by exception type.

#include <stdexcept>

int classify(int code) {
    if (code == 1) throw std::runtime_error("oops");
    if (code == 2) throw std::logic_error("bug");
    if (code == 3) throw 42;
    return 0;
}

int main() {
    int hits = 0;

    for (int code = 1; code <= 3; ++code) {
        try {
            classify(code);
        } catch (const std::runtime_error&) {
            hits += 1;
        } catch (const std::logic_error&) {
            hits += 10;
        } catch (...) {
            hits += 100;
        }
    }

    return 0;
}
