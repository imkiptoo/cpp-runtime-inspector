// Advanced ghost frame tests for temporary destruction
//
// Test cases:
// 1. Temporaries in operator expressions
// 2. Temporaries in conditional operator (?:)
// 3. Temporaries with explicit destructor calls
// 4. Temporary lifetime extension via const ref (no ghost dtor expected)
// 5. Temporaries in function argument evaluation
// 6. Temporaries from implicit conversions
// 7. Temporaries in comma expressions
// 8. Temporaries in array subscript
// 9. Temporaries in pointer member access
// 10. Temporaries from overloaded operators

struct Counter {
    int id;

    Counter() {
        this->id = 1;
    }

    Counter(int v) {
        this->id = v;
    }

    Counter(const Counter& o) {
        this->id = o.id + 1000;
    }

    Counter(Counter&& o) {
        this->id = o.id + 2000;
        o.id = -1;
    }

    ~Counter() {
    }

    int get() const { return this->id; }

    Counter operator+(const Counter& o) const {
        return Counter(this->id + o.id);
    }

    Counter operator*(int n) const {
        return Counter(this->id * n);
    }
};

// Implicit conversion test
struct Convertible {
    int val;

    Convertible(int v) {
        this->val = v;
    }

    operator int() const {
        return this->val;
    }

    ~Convertible() {}
};

// Array access test
struct WithArray {
    int arr[3];

    WithArray() {
        this->arr[0] = 10;
        this->arr[1] = 20;
        this->arr[2] = 30;
    }

    int& operator[](int i) {
        return this->arr[i];
    }

    ~WithArray() {}
};

// Function returning multiple via pair-like struct
struct Pair {
    int first;
    int second;

    Pair(int a, int b) {
        this->first = a;
        this->second = b;
    }

    ~Pair() {}
};

Pair makePair(int a, int b) {
    return Pair(a, b);
}

// Multi-argument function
int sumThree(Counter a, Counter b, Counter c) {
    return a.get() + b.get() + c.get();
}

int main() {
    // Test 1: Temporary in overloaded operator+
    (Counter(1) + Counter(2)).get();

    // Test 2: Temporary in overloaded operator*
    (Counter(3) * 5).get();

    // Test 3: Chained operators creating multiple temporaries
    (Counter(4) + Counter(5) + Counter(6)).get();

    // Test 4: Temporary in function with multiple args
    sumThree(Counter(7), Counter(8), Counter(9));

    // Test 5: Implicit conversion from temporary
    int val = Convertible(100);
    (void)val;

    // Test 6: Array access on temporary (if array subscript creates temp)
    WithArray()[1];

    // Test 7: Multiple temporaries in comma expression
    Counter(10).get(), Counter(11).get();

    // Test 8: Temporary destruction order - right to left evaluation
    // Arguments are evaluated in unspecified order, but destruction is LIFO
    sumThree(Counter(12), Counter(13), Counter(14));

    // Test 9: Nested function calls with temporaries
    (Counter(Counter(15).get())).get();

    // Test 10: Pair destruction
    makePair(20, 21).first + makePair(22, 23).second;

    return 0;
}
