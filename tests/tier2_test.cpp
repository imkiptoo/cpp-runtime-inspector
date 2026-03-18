// Tier 2 test: structs, enums, arrays

enum Color {
    Red = 0,
    Green = 1,
    Blue = 2
};

enum class Direction {
    North = 0,
    South = 1,
    East = 2,
    West = 3
};

struct Point {
    int x;
    int y;
};

struct Line {
    Point start;
    Point end;
};

int main() {
    // Test enum
    Color c = Green;
    c = Blue;

    // Test scoped enum
    Direction d = Direction::East;

    // Test struct
    Point p;
    p.x = 10;
    p.y = 20;

    // Test nested struct
    Line ln;
    ln.start.x = 0;
    ln.start.y = 0;
    ln.end.x = 100;
    ln.end.y = 100;

    // Test array
    int arr[3] = {1, 2, 3};
    arr[0] = 10;

    return 0;
}
