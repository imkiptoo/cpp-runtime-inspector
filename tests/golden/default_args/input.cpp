// Default arguments test
// Verify functions called with omitted trailing args use defaults.

int build(int a, int b = 10, int c = 100) {
    return a + b + c;
}

int main() {
    int all = build(1, 2, 3);
    int two = build(1, 2);
    int one = build(1);
    return 0;
}
