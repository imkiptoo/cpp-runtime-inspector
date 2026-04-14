// Rule-of-5 lifecycle tracking: Temporaries and destruction order
//
// Test cases:
// 1. Temporary objects created and destroyed in expressions
// 2. Function return value handling
// 3. Destruction order (reverse of construction)
// 4. Destruction during exception (implicit - not throw)
// 5. Scope-based destruction timing
// 6. Conditional object creation

struct Tracker {
    int id;
    static int next_id;
    static int destruction_order;

    Tracker() {
        this->id = 1;
    }

    Tracker(int v) {
        this->id = v;
    }

    Tracker(const Tracker& o) {
        this->id = o.id + 100;
    }

    Tracker(Tracker&& o) {
        this->id = o.id + 200;
        o.id = -1;
    }

    Tracker& operator=(const Tracker& o) {
        this->id = o.id + 1000;
        return *this;
    }

    Tracker& operator=(Tracker&& o) {
        this->id = o.id + 2000;
        o.id = -2;
        return *this;
    }

    ~Tracker() {
        // Mark destruction happened
        this->id = this->id * -1;
    }

    int get() const { return this->id; }
};

// Function that returns by value
Tracker makeTracker(int val) {
    Tracker t(val);
    return t;
}

// Function that takes by value (forces copy/move)
int consumeTracker(Tracker t) {
    return t.id;
}

// Function that takes by reference (no copy)
int inspectTracker(const Tracker& t) {
    return t.id;
}

// Function returning prvalue
Tracker createTemp() {
    return Tracker(500);
}

// Overloaded operator for chaining
struct Chainable {
    int val;

    Chainable() { this->val = 0; }
    Chainable(int v) { this->val = v; }
    Chainable(const Chainable& o) { this->val = o.val + 1; }
    Chainable(Chainable&& o) { this->val = o.val; o.val = -1; }
    ~Chainable() {}

    Chainable operator+(const Chainable& o) const {
        return Chainable(this->val + o.val);
    }
};

// Scope test helper
void scopeTest() {
    Tracker outer(1);
    {
        Tracker inner(2);
        // inner destroyed first when scope ends
    }
    // outer destroyed when function returns
}

// Conditional creation
Tracker conditionalCreate(bool flag) {
    if (flag) {
        return Tracker(100);
    } else {
        return Tracker(200);
    }
}

int main() {
    // Test 1: Basic temporary in expression
    int result1 = Tracker(10).get();
    (void)result1;

    // Test 2: Temporary from function return
    Tracker t1 = makeTracker(20);

    // Test 3: Pass temporary to function (copy)
    int result2 = consumeTracker(Tracker(30));
    (void)result2;

    // Test 4: Pass lvalue to function (copy)
    Tracker t2(40);
    int result3 = consumeTracker(t2);
    (void)result3;

    // Test 5: Pass moved value
    Tracker t3(50);
    int result4 = consumeTracker(static_cast<Tracker&&>(t3));
    (void)result4;

    // Test 6: Chained operations creating temporaries
    Chainable c1(1);
    Chainable c2(2);
    Chainable c3(3);
    Chainable c4 = c1 + c2 + c3;  // Multiple temporaries

    // Test 7: Destruction order (LIFO)
    Tracker first(1);
    Tracker second(2);
    Tracker third(3);
    // Destroyed in order: third, second, first

    // Test 8: Nested scope destruction
    scopeTest();

    // Test 9: Conditional creation
    Tracker cond1 = conditionalCreate(true);
    Tracker cond2 = conditionalCreate(false);

    // Test 10: Assignment from temporary
    Tracker t4;
    t4 = Tracker(60);

    // Test 11: Move from temporary (redundant but legal)
    Tracker t5;
    t5 = static_cast<Tracker&&>(Tracker(70));

    // Test 12: Multiple statements with temporaries
    {
        Tracker temp1(80);
        Tracker temp2(90);
        int sum = temp1.id + temp2.id;
        (void)sum;
        // Both destroyed at end of block
    }

    return 0;
    // All remaining objects destroyed in reverse order
}
