// STL vector test
// Verify std::vector contents are properly encoded

#include <vector>

int main() {
    std::vector<int> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);

    int sum = 0;
    for (int x : v) {
        sum += x;
    }

    v.clear();
    return 0;
}
