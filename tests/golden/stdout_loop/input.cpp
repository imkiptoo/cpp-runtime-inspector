// Test stdout capture with loop - progressive output

#include <iostream>

int main() {
    for (int i = 0; i < 3; i++) {
        std::cout << i << " ";
    }
    std::cout << std::endl;
    return 0;
}
