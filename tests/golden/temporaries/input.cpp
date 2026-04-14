// Ghost frames for temporary destruction
//
// Test cases:
// 1. Basic temporary in expression
// 2. Temporary from function return
// 3. Multiple temporaries in single expression
// 4. Temporary passed to function
// 5. Chained temporaries
// 6. Temporary with destructor side effects
// 7. Nested temporary construction
// 8. Multiple temporaries of different types
// 9. Temporary in arithmetic expression
// 10. Temporary lifetime extension (binding to const ref)

struct TempA {
    int id;

    TempA() {
        this->id = 100;
    }

    TempA(int v) {
        this->id = v;
    }

    TempA(const TempA& o) {
        this->id = o.id + 1;
    }

    TempA(TempA&& o) {
        this->id = o.id + 10;
        o.id = -1;
    }

    ~TempA() {
        this->id = -999;
    }

    int get() const { return this->id; }
    TempA chain() { return TempA(this->id * 2); }
};

struct TempB {
    int val;

    TempB() {
        this->val = 200;
    }

    TempB(int v) {
        this->val = v;
    }

    TempB(const TempB& o) {
        this->val = o.val + 5;
    }

    ~TempB() {
        this->val = -888;
    }

    int get() const { return this->val; }
};

// Function returning by value
TempA makeTempA(int v) {
    return TempA(v);
}

TempB makeTempB(int v) {
    return TempB(v);
}

// Function taking by value (consumes temporary)
int consumeA(TempA t) {
    return t.get();
}

int consumeB(TempB t) {
    return t.get();
}

// Function taking two temporaries
int consumeBoth(TempA a, TempB b) {
    return a.get() + b.get();
}

// Function returning sum of two temporaries
int addTemps(TempA a, TempB b) {
    return a.id + b.val;
}

int main() {
    // Test 1: Basic temporary - created and destroyed in expression
    int r1 = TempA(10).get();
    (void)r1;

    // Test 2: Temporary from function return
    int r2 = makeTempA(20).get();
    (void)r2;

    // Test 3: Multiple temporaries in expression
    int r3 = TempA(30).get() + TempB(31).get();
    (void)r3;

    // Test 4: Temporary passed to function
    int r4 = consumeA(TempA(40));
    (void)r4;

    // Test 5: Chained temporary operations
    int r5 = TempA(50).chain().get();
    (void)r5;

    // Test 6: Two temporaries passed to function
    int r6 = consumeBoth(TempA(60), TempB(61));
    (void)r6;

    // Test 7: Temporary from return value passed to another function
    int r7 = consumeA(makeTempA(70));
    (void)r7;

    // Test 8: Multiple function returns creating temporaries
    int r8 = addTemps(makeTempA(80), makeTempB(81));
    (void)r8;

    // Test 9: Nested temporary construction
    TempA a9(TempA(90).get());
    int r9 = a9.get();
    (void)r9;

    // Test 10: Temporary with side effects through destructor
    {
        int r10 = TempA(100).get();
        (void)r10;
        // TempA destroyed at end of full-expression
    }

    // Test 11: Expression statement with temporary (ghost dtor should be injected)
    TempA(110).get();

    // Test 12: Multiple temporaries in expression statement
    TempA(120).get() + TempB(121).get();

    // Test 13: Chained call as expression statement
    TempA(130).chain().get();

    // Test 14: Function consuming temporary as expression statement
    consumeA(TempA(140));

    return 0;
}
