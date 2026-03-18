// Heap array allocation test

int main() {
    int* arr = new int[3];
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    delete[] arr;
    return 0;
}
