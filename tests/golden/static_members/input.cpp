// Static class members test
// A counter shared across instances; static method reads it.

struct Counter {
    static int total;
    int self;

    void bump() {
        ++self;
        ++total;
    }

    static int globalTotal() { return total; }
};

int Counter::total = 0;

int main() {
    Counter a{};
    Counter b{};
    a.bump();
    a.bump();
    b.bump();

    int t = Counter::globalTotal();
    int x = Counter::total;
    return 0;
}
