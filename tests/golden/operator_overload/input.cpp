// Operator overloading test
//
// Verify user-defined operators are entered, their bodies are traced, and
// the trace shows the resulting state through every flavour of operator:
// binary (member + free), compound assignment, unary pre/post inc-dec,
// and equality comparison.

struct Vec2 {
    int x;
    int y;

    // Member binary +
    Vec2 operator+(const Vec2& o) const {
        Vec2 r{};
        r.x = this->x + o.x;
        r.y = this->y + o.y;
        return r;
    }

    // Compound assignment
    Vec2& operator+=(const Vec2& o) {
        this->x += o.x;
        this->y += o.y;
        return *this;
    }

    // Pre-increment
    Vec2& operator++() {
        ++this->x;
        ++this->y;
        return *this;
    }

    // Post-increment
    Vec2 operator++(int) {
        Vec2 copy = *this;
        ++(*this);
        return copy;
    }

    // Member equality
    bool operator==(const Vec2& o) const {
        return this->x == o.x && this->y == o.y;
    }
};

// Free-function unary -
Vec2 operator-(const Vec2& v) {
    Vec2 r{};
    r.x = -v.x;
    r.y = -v.y;
    return r;
}

int main() {
    Vec2 a{};
    a.x = 1;
    a.y = 2;

    Vec2 b{};
    b.x = 10;
    b.y = 20;

    Vec2 c = a + b;        // member operator+
    a += b;                // compound assignment
    ++a;                   // pre-increment
    Vec2 prev = a++;       // post-increment
    Vec2 neg = -c;         // free-function unary -
    bool eq = (a == b);    // equality

    return 0;
}
