// Return value test
// Verify function return values are captured

int add(int a, int b) {
    return a + b;
}

double multiply(double x, double y) {
    return x * y;
}

struct Point {
    int x;
    int y;
};

Point makePoint(int x, int y) {
    Point p = {x, y};
    return p;
}

int main() {
    int sum = add(3, 5);
    double product = multiply(2.5, 4.0);
    Point p = makePoint(10, 20);

    return 0;
}
