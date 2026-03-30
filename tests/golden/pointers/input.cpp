// Pointer basics test
// Verify pointer initialization, assignment, and dereferencing

int main() {
    int x = 42;
    int y = 100;

    // Pointer initialization
    int* ptr = &x;

    // Dereferencing
    int val = *ptr;

    // Reassignment
    ptr = &y;
    val = *ptr;

    // Null pointer
    int* null_ptr = nullptr;

    // Pointer to pointer
    int** pptr = &ptr;
    val = **pptr;

    return 0;
}
