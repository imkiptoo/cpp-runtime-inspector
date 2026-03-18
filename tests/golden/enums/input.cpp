enum Color { Red = 0, Green = 1, Blue = 2 };

enum class Direction { North = 0, South = 1, East = 2, West = 3 };

int main() {
    Color c = Green;
    Direction d = Direction::East;
    c = Blue;
    return 0;
}
