// Use-after-free detection test

int main() {
    int* p = new int(42);
    delete p;
    int* q = p;  // q now points to freed memory
    return 0;
}
