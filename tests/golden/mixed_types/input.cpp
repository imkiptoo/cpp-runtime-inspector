// Mixed types test
// Tests handling of various type combinations

struct Point {
    int x;
    int y;
};

enum Color { RED, GREEN, BLUE };

int main() {
    // Primitives
    int i = 42;
    double d = 3.14;
    bool b = true;
    char c = 'A';

    // Struct
    Point p = {10, 20};

    // Enum
    Color color = GREEN;

    // Array
    int arr[3] = {1, 2, 3};

    // Pointer
    int* ptr = &i;

    // Reference
    int& ref = i;

    // Modify some values
    i = 100;
    p.x = 30;
    arr[1] = 42;

    return 0;
}
