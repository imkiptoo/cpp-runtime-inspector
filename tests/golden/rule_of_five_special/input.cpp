// Rule-of-5 lifecycle tracking: Special cases
//
// Test cases:
// 1. Explicit constructors
// 2. Converting constructors (single arg, not copy/move)
// 3. Initializer list construction
// 4. Aggregate initialization
// 5. Brace initialization vs parentheses
// 6. Multiple constructor overloads
//
// Note: = default special members are not instrumented since they have
// no body. This test focuses on user-defined special members.

// Explicit constructor - prevents implicit conversions
struct Explicit {
    int value;

    Explicit() {
        this->value = 0;
    }

    // Explicit single-arg constructor - NOT a converting constructor
    explicit Explicit(int v) {
        this->value = v;
    }

    Explicit(const Explicit& o) {
        this->value = o.value + 10;
    }

    Explicit(Explicit&& o) {
        this->value = o.value + 20;
        o.value = -1;
    }

    Explicit& operator=(const Explicit& o) {
        this->value = o.value + 100;
        return *this;
    }

    Explicit& operator=(Explicit&& o) {
        this->value = o.value + 200;
        o.value = -2;
        return *this;
    }

    ~Explicit() {}
};

// Converting constructor (implicit single-arg)
struct Converting {
    int value;

    Converting() {
        this->value = 0;
    }

    // Implicit converting constructor from int
    Converting(int v) {
        this->value = v;
    }

    // Implicit converting constructor from double
    Converting(double v) {
        this->value = static_cast<int>(v);
    }

    Converting(const Converting& o) {
        this->value = o.value + 100;
    }

    Converting(Converting&& o) {
        this->value = o.value + 200;
        o.value = -1;
    }

    Converting& operator=(const Converting& o) {
        this->value = o.value + 1000;
        return *this;
    }

    Converting& operator=(Converting&& o) {
        this->value = o.value + 2000;
        o.value = -2;
        return *this;
    }

    ~Converting() {}
};

// With initializer list support
struct WithInitList {
    int a;
    int b;
    int c;

    WithInitList() : a(0), b(0), c(0) {}

    // Takes brace-init-list implicitly (not std::initializer_list)
    WithInitList(int x, int y, int z) : a(x), b(y), c(z) {}

    WithInitList(const WithInitList& o) : a(o.a), b(o.b), c(o.c) {}

    WithInitList(WithInitList&& o) : a(o.a), b(o.b), c(o.c) {
        o.a = -1;
        o.b = -1;
        o.c = -1;
    }

    ~WithInitList() {}
};

// Multiple constructor overloads
struct MultiCtor {
    int x;
    int y;

    MultiCtor() {
        this->x = 0;
        this->y = 0;
    }

    MultiCtor(int val) {
        this->x = val;
        this->y = val;
    }

    MultiCtor(int a, int b) {
        this->x = a;
        this->y = b;
    }

    MultiCtor(const MultiCtor& o) {
        this->x = o.x + 10;
        this->y = o.y + 10;
    }

    MultiCtor(MultiCtor&& o) {
        this->x = o.x + 20;
        this->y = o.y + 20;
        o.x = -1;
        o.y = -1;
    }

    ~MultiCtor() {}
};

// Function taking Converting by value (tests implicit conversion)
int takeConverting(Converting c) {
    return c.value;
}

// Function returning Explicit
Explicit makeExplicit(int v) {
    return Explicit(v);
}

int main() {
    // Test 1: Explicit constructor - must use direct init
    Explicit e1;
    Explicit e2(100);  // Direct initialization OK
    // Explicit e3 = 100;  // Would be error - implicit conversion not allowed
    Explicit e4 = e2;  // Copy is OK

    // Test 2: Explicit move
    Explicit e5 = static_cast<Explicit&&>(e1);

    // Test 3: Explicit assignment
    Explicit e6;
    e6 = e2;  // Copy assign
    Explicit e7;
    e7 = static_cast<Explicit&&>(e6);  // Move assign

    // Test 4: Converting constructor - implicit conversion
    Converting c1;
    Converting c2 = 50;      // Implicit from int
    Converting c3 = 3.14;    // Implicit from double
    Converting c4 = c2;      // Copy constructor

    // Test 5: Converting move
    Converting c5 = static_cast<Converting&&>(c1);

    // Test 6: Converting assignment
    Converting c6;
    c6 = c2;
    Converting c7;
    c7 = static_cast<Converting&&>(c6);

    // Test 7: Implicit conversion in function call
    int result = takeConverting(75);  // int -> Converting
    (void)result;

    // Test 8: Brace initialization
    WithInitList wil1{10, 20, 30};
    WithInitList wil2 = wil1;

    // Test 9: Parentheses vs braces
    WithInitList wil3(40, 50, 60);
    WithInitList wil4{wil3};  // Copy via braces

    // Test 10: Move with braces
    WithInitList wil5{static_cast<WithInitList&&>(wil3)};

    // Test 11: Return with explicit type
    Explicit e8 = makeExplicit(999);

    // Test 12: Multiple constructor overloads
    MultiCtor mc1;           // Default
    MultiCtor mc2(42);       // Single arg
    MultiCtor mc3(10, 20);   // Two args
    MultiCtor mc4 = mc2;     // Copy
    MultiCtor mc5 = static_cast<MultiCtor&&>(mc3);  // Move

    return 0;
}
