// Rule-of-5 lifecycle tracking: Nested objects
//
// Test cases:
// 1. Class containing another class with Rule-of-5
// 2. Multiple member objects
// 3. Array of objects as member
// 4. Pointer member (shallow vs deep copy)
// 5. Member initialization order

// Inner class with full Rule-of-5
struct Inner {
    int value;
    static int instance_count;

    Inner() {
        this->value = 0;
    }

    Inner(int v) {
        this->value = v;
    }

    Inner(const Inner& o) {
        this->value = o.value + 1000;  // Mark as copied
    }

    Inner(Inner&& o) {
        this->value = o.value + 2000;  // Mark as moved
        o.value = -999;
    }

    Inner& operator=(const Inner& o) {
        this->value = o.value + 3000;
        return *this;
    }

    Inner& operator=(Inner&& o) {
        this->value = o.value + 4000;
        o.value = -888;
        return *this;
    }

    ~Inner() {
    }
};

// Outer class containing Inner
struct Outer {
    Inner member;
    int outer_id;

    Outer() : member() {
        this->outer_id = 1;
    }

    Outer(int inner_val) : member(inner_val) {
        this->outer_id = 2;
    }

    Outer(const Outer& o) : member(o.member) {
        this->outer_id = o.outer_id + 100;
    }

    Outer(Outer&& o) : member(static_cast<Inner&&>(o.member)) {
        this->outer_id = o.outer_id + 200;
        o.outer_id = -1;
    }

    Outer& operator=(const Outer& o) {
        this->member = o.member;
        this->outer_id = o.outer_id + 300;
        return *this;
    }

    Outer& operator=(Outer&& o) {
        this->member = static_cast<Inner&&>(o.member);
        this->outer_id = o.outer_id + 400;
        o.outer_id = -2;
        return *this;
    }

    ~Outer() {
    }
};

// Class with multiple members (tests initialization order)
struct MultiMember {
    Inner first;
    Inner second;
    Inner third;

    MultiMember() : first(1), second(2), third(3) {}

    MultiMember(const MultiMember& o)
        : first(o.first), second(o.second), third(o.third) {}

    MultiMember(MultiMember&& o)
        : first(static_cast<Inner&&>(o.first)),
          second(static_cast<Inner&&>(o.second)),
          third(static_cast<Inner&&>(o.third)) {}

    MultiMember& operator=(const MultiMember& o) {
        this->first = o.first;
        this->second = o.second;
        this->third = o.third;
        return *this;
    }

    MultiMember& operator=(MultiMember&& o) {
        this->first = static_cast<Inner&&>(o.first);
        this->second = static_cast<Inner&&>(o.second);
        this->third = static_cast<Inner&&>(o.third);
        return *this;
    }

    ~MultiMember() {}
};

// Class with array member
struct WithArray {
    Inner arr[3];

    WithArray() {
        this->arr[0] = Inner(10);
        this->arr[1] = Inner(20);
        this->arr[2] = Inner(30);
    }

    WithArray(const WithArray& o) {
        this->arr[0] = o.arr[0];
        this->arr[1] = o.arr[1];
        this->arr[2] = o.arr[2];
    }

    WithArray& operator=(const WithArray& o) {
        this->arr[0] = o.arr[0];
        this->arr[1] = o.arr[1];
        this->arr[2] = o.arr[2];
        return *this;
    }

    ~WithArray() {}
};

// Deeply nested: Outer contains Inner, Container contains Outer
struct Container {
    Outer outer;
    int container_id;

    Container() : outer() {
        this->container_id = 9000;
    }

    Container(const Container& o) : outer(o.outer) {
        this->container_id = o.container_id + 100;
    }

    Container(Container&& o) : outer(static_cast<Outer&&>(o.outer)) {
        this->container_id = o.container_id + 200;
        o.container_id = -9000;
    }

    ~Container() {}
};

int main() {
    // Test 1: Simple nested - outer copy triggers inner copy
    Outer o1;
    Outer o2 = o1;  // Outer copy ctor -> Inner copy ctor

    // Test 2: Nested move
    Outer o3 = static_cast<Outer&&>(o1);  // Outer move -> Inner move

    // Test 3: Nested copy assignment
    Outer o4;
    o4 = o2;  // Outer copy assign -> Inner copy assign

    // Test 4: Nested move assignment
    Outer o5;
    o5 = static_cast<Outer&&>(o4);  // Outer move assign -> Inner move assign

    // Test 5: Multiple members - order of initialization
    MultiMember mm1;
    MultiMember mm2 = mm1;

    // Test 6: Multiple member move
    MultiMember mm3 = static_cast<MultiMember&&>(mm1);

    // Test 7: Multiple member assignment
    MultiMember mm4;
    mm4 = mm2;

    // Test 8: Array member
    WithArray wa1;
    WithArray wa2 = wa1;
    WithArray wa3;
    wa3 = wa2;

    // Test 9: Deep nesting (3 levels)
    Container c1;
    Container c2 = c1;
    Container c3 = static_cast<Container&&>(c1);

    return 0;
}
