// enum class vs plain enum test
// Verify scoped enums require qualified access and don't decay to int.

enum Color { Red, Green, Blue };
enum class Direction { North, South, East, West };

int main() {
    Color c = Green;                         // unqualified ok for plain enum
    int ci = c;                              // implicit conversion to int

    Direction d = Direction::East;           // qualification required
    int di = static_cast<int>(d);            // explicit cast required

    return 0;
}
