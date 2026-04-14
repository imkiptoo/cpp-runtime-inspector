// Edge cases for ghost frame temporary destruction
//
// Test cases:
// 1. Temporary in boolean short-circuit (&&, ||)
// 2. Temporary destruction with exception (not throw, just normal flow)
// 3. Deeply nested temporary creation
// 4. Temporary as function pointer call result
// 5. Temporary with virtual destructor
// 6. Temporary in static_cast
// 7. Temporary destruction order in complex expressions

struct Base {
    int val;

    Base() {
        this->val = 0;
    }

    Base(int v) {
        this->val = v;
    }

    Base(const Base& o) {
        this->val = o.val + 100;
    }

    virtual ~Base() {
    }

    virtual int get() const { return this->val; }
};

struct Derived : Base {
    int extra;

    Derived() : Base() {
        this->extra = 0;
    }

    Derived(int v) : Base(v) {
        this->extra = v * 2;
    }

    Derived(const Derived& o) : Base(o) {
        this->extra = o.extra + 10;
    }

    ~Derived() override {
    }

    int get() const override { return this->val + this->extra; }
};

// For boolean expression tests
struct BoolTemp {
    int id;

    BoolTemp(int v) {
        this->id = v;
    }

    BoolTemp(const BoolTemp& o) {
        this->id = o.id + 1;
    }

    ~BoolTemp() {
    }

    bool asBool() const { return this->id > 0; }
    operator bool() const { return this->id > 0; }
};

// Deep nesting helper
struct Nester {
    int depth;

    Nester(int d) {
        this->depth = d;
    }

    Nester(const Nester& o) {
        this->depth = o.depth + 1;
    }

    ~Nester() {
    }

    Nester nest() const {
        return Nester(this->depth + 1);
    }

    int getDepth() const { return this->depth; }
};

// Function pointer type
typedef int (*IntFunc)();

struct FuncProvider {
    int val;

    FuncProvider(int v) {
        this->val = v;
    }

    ~FuncProvider() {
    }

    int call() const { return this->val; }
};

int globalFunc() { return 42; }

int main() {
    // Test 1: Temporary in && short-circuit (second operand not evaluated if first is false)
    BoolTemp(5).asBool() && BoolTemp(6).asBool();

    // Test 2: Temporary in || short-circuit (second operand not evaluated if first is true)
    BoolTemp(7).asBool() || BoolTemp(8).asBool();

    // Test 3: Both operands evaluated
    BoolTemp(-1).asBool() && BoolTemp(9).asBool();

    // Test 4: Deeply nested temporary creation
    Nester(0).nest().nest().nest().getDepth();

    // Test 5: Temporary with virtual destructor (polymorphic)
    static_cast<const Base&>(Derived(10)).get();

    // Test 6: Multiple static_casts
    static_cast<int>(static_cast<const Base&>(Derived(11)).get());

    // Test 7: Complex expression with multiple temporaries
    // All temporaries should be destroyed after the full expression
    (Nester(1).nest().getDepth() + Nester(2).nest().getDepth());

    // Test 8: Temporary from function call result used immediately
    FuncProvider(50).call();

    // Test 9: Temporary bool conversion
    BoolTemp(100) ? 1 : 0;

    // Test 10: Mixing different temporary types in one expression
    Nester(3).getDepth() + FuncProvider(4).call();

    return 0;
}
