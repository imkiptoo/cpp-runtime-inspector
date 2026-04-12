// std::map test
// Verify map size and key-ordered iteration. Values are not introspected
// because the runtime currently only knows the key type.

#include <map>

int main() {
    std::map<int, int> m;
    m[3] = 30;
    m[1] = 10;
    m[2] = 20;

    int n = static_cast<int>(m.size());
    int v = m[2];

    return 0;
}
