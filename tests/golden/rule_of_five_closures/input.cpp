// Rule-of-5 lifecycle tracking: Lambdas and closures
//
// Test cases:
// 1. Lambda capturing object by value (triggers copy)
// 2. Lambda capturing object by reference (no copy)
// 3. Copying a lambda that captured by value
// 4. Mutable lambda modifying captured copy
// 5. Multiple captures with different semantics
//
// Note: Init-captures ([x = expr]) are not tested here due to
// instrumentation limitations. They're a C++14 feature that requires
// special handling.

struct Capturable {
    int value;

    Capturable() {
        this->value = 0;
    }

    Capturable(int v) {
        this->value = v;
    }

    Capturable(const Capturable& o) {
        this->value = o.value + 100;
    }

    Capturable(Capturable&& o) {
        this->value = o.value + 200;
        o.value = -1;
    }

    Capturable& operator=(const Capturable& o) {
        this->value = o.value + 1000;
        return *this;
    }

    Capturable& operator=(Capturable&& o) {
        this->value = o.value + 2000;
        o.value = -2;
        return *this;
    }

    ~Capturable() {
    }

    int get() const { return this->value; }
    void set(int v) { this->value = v; }
};

// Helper to invoke a callable
template<typename F>
int invoke(F f) {
    return f();
}

// Helper to copy a callable (useful for testing lambda copy)
template<typename F>
F copyCallable(F f) {
    return f;
}

int main() {
    // Test 1: Capture by value - triggers copy ctor
    Capturable c1(10);
    auto lambda1 = [c1]() {
        return c1.get();
    };
    int result1 = lambda1();
    (void)result1;

    // Test 2: Capture by reference - no copy
    Capturable c2(20);
    auto lambda2 = [&c2]() {
        return c2.get();
    };
    int result2 = lambda2();
    (void)result2;

    // Test 3: Capture all by value - triggers copy ctor
    Capturable c3(30);
    auto lambda3 = [=]() {
        return c3.get();
    };
    int result3 = lambda3();
    (void)result3;

    // Test 4: Capture all by reference - no copy
    Capturable c4(40);
    auto lambda4 = [&]() {
        return c4.get();
    };
    int result4 = lambda4();
    (void)result4;

    // Test 5: Copy the lambda (copies captured values)
    Capturable c5(50);
    auto lambda5a = [c5]() {
        return c5.get();
    };
    auto lambda5b = lambda5a;  // Copy lambda, copies internal Capturable
    int result5a = lambda5a();
    int result5b = lambda5b();
    (void)result5a;
    (void)result5b;

    // Test 6: Mutable lambda modifying captured copy
    Capturable c6(60);
    auto lambda6 = [c6]() mutable {
        c6.set(c6.get() + 1);
        return c6.get();
    };
    int result6a = lambda6();
    int result6b = lambda6();
    (void)result6a;
    (void)result6b;

    // Test 7: Multiple captures (by value and by reference)
    Capturable c7a(70);
    Capturable c7b(71);
    auto lambda7 = [c7a, &c7b]() {
        return c7a.get() + c7b.get();
    };
    int result7 = lambda7();
    (void)result7;

    // Test 8: Capturing and passing to function (lambda copied)
    Capturable c8(80);
    auto lambda8 = [c8]() {
        return c8.get();
    };
    int result8 = invoke(lambda8);  // Lambda passed by value
    (void)result8;

    // Test 9: Copying lambda via helper function
    Capturable c9(90);
    auto lambda9a = [c9]() {
        return c9.get();
    };
    auto lambda9b = copyCallable(lambda9a);  // Explicit copy
    int result9 = lambda9b();
    (void)result9;

    // Test 10: Nested lambdas (inner lambda captured by outer)
    Capturable c10(100);
    auto inner = [c10]() {
        return c10.get();
    };
    auto outer = [inner]() {
        return inner();
    };
    int result10 = outer();
    (void)result10;

    // Test 11: Lambda capturing multiple objects
    Capturable c11a(110);
    Capturable c11b(111);
    Capturable c11c(112);
    auto lambda11 = [c11a, c11b, c11c]() {
        return c11a.get() + c11b.get() + c11c.get();
    };
    int result11 = lambda11();
    (void)result11;

    // Test 12: Mixed capture with modification
    Capturable c12a(120);
    Capturable c12b(121);
    auto lambda12 = [c12a, &c12b]() mutable {
        c12b.set(c12b.get() + 1);  // Modify by-ref capture
        return c12a.get();          // Read by-value capture
    };
    int result12 = lambda12();
    (void)result12;

    return 0;
}
