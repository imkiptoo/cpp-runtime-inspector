// Function pointer test
// Take the address of a function and call through the pointer.

int add(int a, int b) { return a + b; }
int mul(int a, int b) { return a * b; }

int apply(int (*op)(int, int), int x, int y) {
    return op(x, y);
}

int main() {
    int (*fp)(int, int) = add;
    int s = fp(3, 4);
    fp = mul;
    int p = fp(3, 4);
    int a = apply(add, 10, 20);
    return 0;
}
