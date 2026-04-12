// Virtual dispatch test
//
// Verify that calling a virtual method through a base pointer enters the
// most-derived implementation, and that the encoded value of the receiver
// reports its dynamic (most-derived) class name alongside the static one.

struct Shape {
    int tag;

    Shape() { this->tag = 0; }
    virtual ~Shape() {}

    virtual int area() const {
        return 0;
    }
};

struct Circle : Shape {
    int radius;

    Circle(int r) {
        this->tag = 1;
        this->radius = r;
    }

    int area() const override {
        return 3 * this->radius * this->radius;
    }
};

struct Square : Shape {
    int side;

    Square(int s) {
        this->tag = 2;
        this->side = s;
    }

    int area() const override {
        return this->side * this->side;
    }
};

int main() {
    Circle c(4);
    Square s(5);

    Shape* p1 = &c;
    Shape* p2 = &s;

    int a1 = p1->area();   // -> Circle::area, returns 48
    int a2 = p2->area();   // -> Square::area, returns 25

    // Reassign p1 to point at the square; same call site, different
    // dynamic dispatch.
    p1 = &s;
    int a3 = p1->area();   // -> Square::area, returns 25

    return 0;
}
