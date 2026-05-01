// Memory layout test
// Tests sizeof and field offset display

struct Point {
    int x;
    int y;
};

struct Rect {
    Point topLeft;
    Point bottomRight;
    double area;
};

// Struct with padding
struct Padded {
    char a;
    int b;
    char c;
};

int main() {
    Point p{10, 20};
    Rect r{{0, 0}, {100, 50}, 5000.0};
    Padded pad{'A', 42, 'Z'};

    int sizes[3] = {0, 0, 0};
    sizes[0] = sizeof(Point);
    sizes[1] = sizeof(Rect);
    sizes[2] = sizeof(Padded);

    return 0;
}
