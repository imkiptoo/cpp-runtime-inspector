// Function overloading test
// Same name resolved by argument type/count.

int add(int a, int b) {
    return a + b;
}

double add(double a, double b) {
    return a + b;
}

int add(int a, int b, int c) {
    return a + b + c;
}

int main() {
    int s1 = add(1, 2);
    double s2 = add(1.5, 2.5);
    int s3 = add(1, 2, 3);
    return 0;
}
