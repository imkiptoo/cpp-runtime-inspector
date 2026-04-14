// Rule-of-5 lifecycle tracking: Template classes
//
// Test cases:
// 1. Simple template class with Rule-of-5
// 2. Template with multiple type parameters
// 3. Template specialization
// 4. Nested templates
// 5. CRTP pattern (Curiously Recurring Template Pattern)

// Simple template wrapper
template <typename T>
struct Box {
    T value;

    Box() {
        // Default-initialize value
    }

    Box(T v) : value(v) {}

    Box(const Box& o) {
        this->value = o.value;
    }

    Box(Box&& o) {
        this->value = static_cast<T&&>(o.value);
    }

    Box& operator=(const Box& o) {
        this->value = o.value;
        return *this;
    }

    Box& operator=(Box&& o) {
        this->value = static_cast<T&&>(o.value);
        return *this;
    }

    ~Box() {}
};

// Template with multiple parameters
template <typename T, typename U>
struct Pair {
    T first;
    U second;

    Pair() {}

    Pair(T a, U b) : first(a), second(b) {}

    Pair(const Pair& o) : first(o.first), second(o.second) {}

    Pair(Pair&& o) : first(static_cast<T&&>(o.first)),
                     second(static_cast<U&&>(o.second)) {}

    Pair& operator=(const Pair& o) {
        this->first = o.first;
        this->second = o.second;
        return *this;
    }

    Pair& operator=(Pair&& o) {
        this->first = static_cast<T&&>(o.first);
        this->second = static_cast<U&&>(o.second);
        return *this;
    }

    ~Pair() {}
};

// Inner type for testing nested templates
struct Inner {
    int val;

    Inner() { this->val = 0; }
    Inner(int v) : val(v) {}
    Inner(const Inner& o) { this->val = o.val + 1; }
    Inner(Inner&& o) { this->val = o.val; o.val = -1; }
    Inner& operator=(const Inner& o) { this->val = o.val + 10; return *this; }
    Inner& operator=(Inner&& o) { this->val = o.val; o.val = -10; return *this; }
    ~Inner() {}
};

// CRTP base
template <typename Derived>
struct CRTPBase {
    int crtp_id;

    CRTPBase() { this->crtp_id = 0; }
    CRTPBase(const CRTPBase& o) { this->crtp_id = o.crtp_id + 1; }
    CRTPBase(CRTPBase&& o) { this->crtp_id = o.crtp_id; o.crtp_id = -1; }
    ~CRTPBase() {}

    Derived& derived() { return static_cast<Derived&>(*this); }
};

struct CRTPDerived : public CRTPBase<CRTPDerived> {
    int derived_id;

    CRTPDerived() : CRTPBase<CRTPDerived>() { this->derived_id = 100; }
    CRTPDerived(const CRTPDerived& o) : CRTPBase<CRTPDerived>(o) {
        this->derived_id = o.derived_id + 1;
    }
    CRTPDerived(CRTPDerived&& o) : CRTPBase<CRTPDerived>(static_cast<CRTPBase<CRTPDerived>&&>(o)) {
        this->derived_id = o.derived_id;
        o.derived_id = -100;
    }
    ~CRTPDerived() {}
};

int main() {
    // Test 1: Simple template instantiation with int
    Box<int> bi1;
    Box<int> bi2(42);
    Box<int> bi3 = bi2;          // copy ctor
    Box<int> bi4 = static_cast<Box<int>&&>(bi1);  // move ctor

    // Test 2: Template with different type
    Box<double> bd1;
    Box<double> bd2(3.14);
    bd1 = bd2;                   // copy assign

    // Test 3: Template with user-defined type
    Box<Inner> bin1;
    Box<Inner> bin2;
    bin2.value = Inner(50);
    Box<Inner> bin3 = bin2;      // triggers Inner copy too

    // Test 4: Multi-parameter template
    Pair<int, double> p1;
    Pair<int, double> p2(10, 20.5);
    Pair<int, double> p3 = p2;
    p1 = p3;

    // Test 5: Nested template (Box of Pair)
    Box<Pair<int, int>> nested1;
    nested1.value = Pair<int, int>(1, 2);
    Box<Pair<int, int>> nested2 = nested1;

    // Test 6: CRTP pattern
    CRTPDerived cd1;
    CRTPDerived cd2 = cd1;
    CRTPDerived cd3 = static_cast<CRTPDerived&&>(cd1);

    return 0;
}
