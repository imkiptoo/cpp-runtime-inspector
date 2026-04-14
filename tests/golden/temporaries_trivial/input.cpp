// Ghost frames for trivial/minimal destructor temporaries
//
// These tests focus on classes where the destructor has minimal or no
// user-visible side effects, making ghost frames particularly valuable
// for understanding object lifetimes.

// Trivial destructor (implicitly defined)
struct Trivial {
    int x;
    int y;

    Trivial(int a, int b) {
        this->x = a;
        this->y = b;
    }

    Trivial(const Trivial& o) {
        this->x = o.x;
        this->y = o.y;
    }

    // No user-defined destructor - compiler generates trivial one

    int sum() const { return this->x + this->y; }
};

// Minimal destructor (empty body)
struct Minimal {
    int val;

    Minimal(int v) {
        this->val = v;
    }

    Minimal(const Minimal& o) {
        this->val = o.val;
    }

    ~Minimal() {
        // Empty destructor body
    }

    int get() const { return this->val; }
};

// POD-like struct
struct PodLike {
    int a;
    int b;
    int c;

    PodLike(int x, int y, int z) {
        this->a = x;
        this->b = y;
        this->c = z;
    }

    PodLike(const PodLike& o) {
        this->a = o.a;
        this->b = o.b;
        this->c = o.c;
    }

    ~PodLike() {}

    int total() const { return this->a + this->b + this->c; }
};

// Nested trivial types
struct Outer {
    Trivial inner;

    Outer(int a, int b) : inner(a, b) {
    }

    Outer(const Outer& o) : inner(o.inner) {
    }

    ~Outer() {}

    int getSum() const { return this->inner.sum(); }
};

// Function returning trivial type
Trivial makeTrivial(int a, int b) {
    return Trivial(a, b);
}

Minimal makeMinimal(int v) {
    return Minimal(v);
}

PodLike makePod(int a, int b, int c) {
    return PodLike(a, b, c);
}

// Consume functions
int consumeTrivial(Trivial t) {
    return t.sum();
}

int consumeMinimal(Minimal m) {
    return m.get();
}

int consumePod(PodLike p) {
    return p.total();
}

int main() {
    // Test 1: Trivial temporary (implicit destructor)
    Trivial(1, 2).sum();

    // Test 2: Minimal temporary (empty destructor body)
    Minimal(10).get();

    // Test 3: POD-like temporary
    PodLike(1, 2, 3).total();

    // Test 4: Trivial from function return
    makeTrivial(5, 6).sum();

    // Test 5: Minimal from function return
    makeMinimal(20).get();

    // Test 6: POD from function return
    makePod(4, 5, 6).total();

    // Test 7: Multiple trivials in expression
    Trivial(7, 8).sum() + Trivial(9, 10).sum();

    // Test 8: Nested trivial types
    Outer(11, 12).getSum();

    // Test 9: Trivial passed to function
    consumeTrivial(Trivial(13, 14));

    // Test 10: Minimal passed to function
    consumeMinimal(Minimal(30));

    // Test 11: POD passed to function
    consumePod(PodLike(7, 8, 9));

    // Test 12: Mixed trivial types
    Trivial(1, 1).sum() + Minimal(2).get() + PodLike(1, 1, 1).total();

    // Test 13: Chained calls with trivial temps
    Trivial(makeTrivial(100, 200).sum(), 300).sum();

    // Test 14: Multiple function returns
    makeTrivial(10, 20).sum() + makeTrivial(30, 40).sum();

    return 0;
}
