// Lambda test
// Verify lambda captures are properly encoded

int main() {
    int x = 10;
    int y = 20;

    // Lambda capturing x by value, y by reference
    auto f = [x, &y]() {
        return x + y;
    };

    int result = f();

    y = 30;
    result = f();

    return 0;
}
