// Multi-function call test
// Verify function entry/exit and call stack tracking

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    int sub = factorial(n - 1);
    return n * sub;
}

int main() {
    int result = factorial(4);
    return 0;
}
