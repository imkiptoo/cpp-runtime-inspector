// Comprehensive Tier 2 test: all composite types

// Unscoped enum
enum Color { Red = 1, Green = 2, Blue = 4 };

// Scoped enum
enum class Status { Pending, Running, Completed };

// Simple struct
struct Point {
    int x;
    int y;
};

// Struct with array
struct Triangle {
    Point vertices[3];
};

// Base class
struct Shape {
    int id;
    Color color;
};

// Derived class (single inheritance)
struct Circle : Shape {
    Point center;
    int radius;
};

// Union
union Value {
    int i;
    float f;
    char c;
};

// Class with virtual function (polymorphic)
class Animal {
public:
    int legs;
    virtual void speak() {}
};

class Dog : public Animal {
public:
    int tailLength;
    void speak() override {}
};

int main() {
    // Enum tests
    Color c = Green;
    Status s = Status::Running;

    // Struct tests
    Point p = {10, 20};

    // Struct with array
    Triangle t;
    t.vertices[0] = {0, 0};
    t.vertices[1] = {100, 0};
    t.vertices[2] = {50, 100};

    // Inheritance test
    Circle circle;
    circle.id = 1;
    circle.color = Blue;
    circle.center = {50, 50};
    circle.radius = 25;

    // Union test
    Value v;
    v.i = 42;

    // Fixed array test
    int nums[5] = {1, 2, 3, 4, 5};

    // Polymorphic test
    Dog dog;
    dog.legs = 4;
    dog.tailLength = 30;

    return 0;
}
