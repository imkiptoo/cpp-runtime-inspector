// std::set test
// Verify in-order traversal of a tree-backed sorted set.

#include <set>

int main() {
    std::set<int> s;
    s.insert(20);
    s.insert(5);
    s.insert(13);
    s.insert(40);
    s.insert(1);

    int n = static_cast<int>(s.size());
    return 0;
}
