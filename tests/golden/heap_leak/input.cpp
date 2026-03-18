// Memory leak detection test

int main() {
    int* p = new int(42);  // This will leak
    return 0;
}
