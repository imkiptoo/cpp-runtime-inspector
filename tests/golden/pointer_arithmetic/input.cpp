// Pointer arithmetic test
// Verify pointer offset resolution in heap arrays

int main() {
    int* arr = new int[5];
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 40;
    arr[4] = 50;

    int* p = arr + 2;  // Points to arr[2]
    int val = *p;

    p++;  // Now points to arr[3]
    val = *p;

    delete[] arr;
    return 0;
}
