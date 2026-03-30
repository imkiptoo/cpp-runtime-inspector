// Reference test
// Verify references are properly tracked

void increment(int& x) {
    x++;
}

void swap(int& a, int& b) {
    int temp = a;
    a = b;
    b = temp;
}

int main() {
    int x = 10;
    int y = 20;

    int& ref = x;
    ref = 15;

    increment(x);
    swap(x, y);

    return 0;
}
