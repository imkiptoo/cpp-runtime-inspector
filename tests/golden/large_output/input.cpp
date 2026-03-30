// Large output test
// Tests handling of programs that generate many events

#include <vector>

int main() {
    // Create a moderately sized array
    std::vector<int> v;

    // Generate multiple events
    for (int i = 0; i < 10; i++) {
        v.push_back(i);
    }

    // Access elements
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += v[i];
    }

    v.clear();
    return 0;
}
