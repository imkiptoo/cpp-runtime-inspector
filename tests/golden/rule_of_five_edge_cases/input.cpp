// Rule-of-5 lifecycle tracking: Edge cases
//
// Test cases:
// 1. Self-assignment (copy and move)
// 2. Chained assignment (a = b = c)
// 3. Assignment in expression context
// 4. Constructor with multiple parameters (not copy/move)
// 5. Explicit copy/move calls
// 6. Const objects
// 7. Reference members affecting copy/move
// 8. Empty struct (trivial operations)

struct Widget {
    int id;
    int copy_count;
    int move_count;

    Widget() {
        this->id = 1;
        this->copy_count = 0;
        this->move_count = 0;
    }

    // Non-copy/move constructor with parameters
    Widget(int val) {
        this->id = val;
        this->copy_count = 0;
        this->move_count = 0;
    }

    // Two-parameter constructor
    Widget(int a, int b) {
        this->id = a + b;
        this->copy_count = 0;
        this->move_count = 0;
    }

    Widget(const Widget& o) {
        this->id = o.id;
        this->copy_count = o.copy_count + 1;
        this->move_count = o.move_count;
    }

    Widget(Widget&& o) {
        this->id = o.id;
        this->copy_count = o.copy_count;
        this->move_count = o.move_count + 1;
        o.id = -1;
    }

    Widget& operator=(const Widget& o) {
        // Self-assignment check
        if (this != &o) {
            this->id = o.id;
            this->copy_count = o.copy_count + 1;
        }
        return *this;
    }

    Widget& operator=(Widget&& o) {
        // Self-assignment check for move
        if (this != &o) {
            this->id = o.id;
            this->move_count = o.move_count + 1;
            o.id = -2;
        }
        return *this;
    }

    ~Widget() {
    }
};

// Empty struct - should still track lifecycle
struct Empty {
    Empty() {}
    Empty(const Empty&) {}
    Empty(Empty&&) {}
    Empty& operator=(const Empty&) { return *this; }
    Empty& operator=(Empty&&) { return *this; }
    ~Empty() {}
};

// Struct with const member (can't be assigned)
struct WithConst {
    const int id;

    WithConst() : id(42) {}
    WithConst(int v) : id(v) {}
    WithConst(const WithConst& o) : id(o.id) {}
    // No assignment operators - const member
    ~WithConst() {}
};

// Helper that returns a Widget by value (tests RVO context)
Widget makeWidget(int val) {
    Widget w(val);
    return w;
}

// Helper that takes Widget by value
int consumeWidget(Widget w) {
    return w.id;
}

int main() {
    // Test 1: Default construction
    Widget a;

    // Test 2: Parameterized construction (NOT copy/move ctor)
    Widget b(100);
    Widget c(50, 50);

    // Test 3: Self copy-assignment (should be no-op with guard)
    a = a;

    // Test 4: Regular copy assignment
    a = b;

    // Test 5: Chained assignment (a = b = c)
    Widget d;
    Widget e;
    Widget f;
    d = e = f;

    // Test 6: Self move-assignment
    Widget g;
    g = static_cast<Widget&&>(g);

    // Test 7: Move assignment
    Widget h;
    Widget i;
    h = static_cast<Widget&&>(i);

    // Test 8: Copy in expression context
    Widget j = a;
    int result = j.id + 1;
    (void)result;

    // Test 9: Empty struct lifecycle
    Empty em1;
    Empty em2 = em1;
    Empty em3;
    em3 = em2;

    // Test 10: Const member struct
    WithConst wc1;
    WithConst wc2(99);
    WithConst wc3 = wc1;

    // Test 11: Pass by value (triggers copy/move)
    Widget k(500);
    int consumed = consumeWidget(k);
    (void)consumed;

    // Test 12: Return by value
    Widget l = makeWidget(999);

    return 0;
}
