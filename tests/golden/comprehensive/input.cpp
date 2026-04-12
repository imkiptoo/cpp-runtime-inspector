// Comprehensive test
// Tests multiple features together

struct Point {
    int x;
    int y;
};

struct Rectangle {
    Point topLeft;
    Point bottomRight;

    int area() const {
        int width = bottomRight.x - topLeft.x;
        int height = bottomRight.y - topLeft.y;
        return width * height;
    }
};

int main() {
    // Stack variables
    int count = 5;
    double pi = 3.14159;
    bool active = true;
    char letter = 'A';

    // Struct on stack
    Point p = {0, 0};  // Initialize to avoid garbage values
    p.x = 10;
    p.y = 20;

    // Nested struct
    Rectangle rect = {{0, 0}, {0, 0}};  // Initialize to avoid garbage values
    rect.topLeft.x = 0;
    rect.topLeft.y = 0;
    rect.bottomRight.x = 100;
    rect.bottomRight.y = 50;

    int area = rect.area();

    // Heap allocation
    Point* hp = new Point{};
    hp->x = 30;
    hp->y = 40;

    // Array on heap
    int* arr = new int[3]{};
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;

    // Pointer arithmetic
    int* p2 = arr + 1;
    int val = *p2;

    // References
    int& refCount = count;
    refCount = 10;

    // Cleanup
    delete hp;
    delete[] arr;

    return 0;
}
