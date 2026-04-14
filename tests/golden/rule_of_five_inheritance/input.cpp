// Rule-of-5 lifecycle tracking: Inheritance scenarios
//
// Test cases:
// 1. Single inheritance - base ctor/dtor called from derived
// 2. Virtual destructor in polymorphic hierarchy
// 3. Multiple inheritance
// 4. Deep inheritance chain (3 levels)
// 5. Copy/move through base pointer (slicing)

// Base class with full Rule-of-5
struct Base {
    int base_id;

    Base() {
        this->base_id = 100;
    }

    Base(const Base& o) {
        this->base_id = o.base_id + 1;
    }

    Base(Base&& o) {
        this->base_id = o.base_id + 2;
        o.base_id = -100;
    }

    Base& operator=(const Base& o) {
        this->base_id = o.base_id + 10;
        return *this;
    }

    Base& operator=(Base&& o) {
        this->base_id = o.base_id + 20;
        o.base_id = -200;
        return *this;
    }

    virtual ~Base() {
        this->base_id = -1;
    }
};

// Derived class that calls base
struct Derived : public Base {
    int derived_id;

    Derived() : Base() {
        this->derived_id = 200;
    }

    Derived(const Derived& o) : Base(o) {
        this->derived_id = o.derived_id + 1;
    }

    Derived(Derived&& o) : Base(static_cast<Base&&>(o)) {
        this->derived_id = o.derived_id + 2;
        o.derived_id = -200;
    }

    Derived& operator=(const Derived& o) {
        Base::operator=(o);
        this->derived_id = o.derived_id + 10;
        return *this;
    }

    Derived& operator=(Derived&& o) {
        Base::operator=(static_cast<Base&&>(o));
        this->derived_id = o.derived_id + 20;
        o.derived_id = -300;
        return *this;
    }

    ~Derived() override {
        this->derived_id = -2;
    }
};

// Multiple inheritance - two bases
struct MixinA {
    int a_val;
    MixinA() { this->a_val = 10; }
    MixinA(const MixinA& o) { this->a_val = o.a_val + 1; }
    ~MixinA() { this->a_val = -10; }
};

struct MixinB {
    int b_val;
    MixinB() { this->b_val = 20; }
    MixinB(const MixinB& o) { this->b_val = o.b_val + 1; }
    ~MixinB() { this->b_val = -20; }
};

struct MultiDerived : public MixinA, public MixinB {
    int m_val;
    MultiDerived() : MixinA(), MixinB() { this->m_val = 30; }
    MultiDerived(const MultiDerived& o) : MixinA(o), MixinB(o) {
        this->m_val = o.m_val + 1;
    }
    ~MultiDerived() { this->m_val = -30; }
};

// Deep inheritance chain
struct Level0 {
    int l0;
    Level0() { this->l0 = 0; }
    Level0(const Level0& o) { this->l0 = o.l0 + 1; }
    virtual ~Level0() { this->l0 = -1; }
};

struct Level1 : public Level0 {
    int l1;
    Level1() : Level0() { this->l1 = 1; }
    Level1(const Level1& o) : Level0(o) { this->l1 = o.l1 + 1; }
    ~Level1() override { this->l1 = -1; }
};

struct Level2 : public Level1 {
    int l2;
    Level2() : Level1() { this->l2 = 2; }
    Level2(const Level2& o) : Level1(o) { this->l2 = o.l2 + 1; }
    ~Level2() override { this->l2 = -1; }
};

int main() {
    // Test 1: Single inheritance - derived construction
    Derived d1;

    // Test 2: Derived copy construction (triggers base copy ctor too)
    Derived d2 = d1;

    // Test 3: Derived move construction
    Derived d3 = static_cast<Derived&&>(d1);

    // Test 4: Derived copy assignment
    Derived d4;
    d4 = d2;

    // Test 5: Derived move assignment
    Derived d5;
    d5 = static_cast<Derived&&>(d4);

    // Test 6: Multiple inheritance
    MultiDerived m1;
    MultiDerived m2 = m1;

    // Test 7: Deep inheritance chain
    Level2 lv1;
    Level2 lv2 = lv1;

    return 0;
    // Destructors fire in reverse order
}
