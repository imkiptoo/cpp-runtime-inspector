// C-string basics test
// Verify const char* handling and string literals

#include <cstring>

int main() {
    // String literal
    const char* str1 = "hello";

    // Another string
    const char* str2 = "world";

    // String length
    int len1 = strlen(str1);
    int len2 = strlen(str2);

    // Character access
    char first = str1[0];
    char last = str1[4];

    // Reassignment
    str1 = str2;

    // Empty string
    const char* empty = "";
    int empty_len = strlen(empty);

    // Null pointer
    const char* null_str = nullptr;

    return 0;
}
