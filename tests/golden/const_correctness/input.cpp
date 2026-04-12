// const correctness test
// const local, const reference parameter, const member function.

struct Counter {
    int n;
    int value() const { return n; }
};

int doubled(const int& x) {
    return x + x;
}

int main() {
    const int limit = 10;
    int twice = doubled(limit);

    Counter c{};
    c.n = 7;
    int v = c.value();

    return 0;
}
