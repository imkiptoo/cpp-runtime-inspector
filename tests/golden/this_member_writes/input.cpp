// Writes through `this->` test
//
// Closes the Tier 1 caveat about static_members: every flavour of write
// through `this->member` (assign, compound assign, ++/--) must be visible
// in the trace so that the receiver object's fields update step-by-step.

struct Counter {
    int value;
    int prev;

    void set(int v) {
        this->prev = this->value;
        this->value = v;
    }

    void bump() {
        ++this->value;
    }

    void add(int n) {
        this->value += n;
    }

    void decAndBump() {
        --this->prev;
        this->value++;
    }
};

int main() {
    Counter c{};
    c.set(10);
    c.bump();
    c.add(5);
    c.decAndBump();
    return 0;
}
