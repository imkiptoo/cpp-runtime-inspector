// STL string test
// Verify std::string contents are properly encoded

#include <string>

int main() {
    std::string s = "hello";
    s += " world";

    int len = s.length();

    s.clear();
    return 0;
}
