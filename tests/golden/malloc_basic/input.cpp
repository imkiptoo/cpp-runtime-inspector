// Basic malloc/free test
// Requires the malloc shim to track C-style allocations

#include <cstdlib>

int main() {
    int* p = static_cast<int*>(malloc(sizeof(int)));
    *p = 42;
    free(p);
    return 0;
}
