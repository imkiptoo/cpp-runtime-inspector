// Class template test
// Verify a generic Box<T> instantiated with multiple types.

template <class T>
struct Box {
    T value;
    T get() const { return value; }
    void set(T v) { value = v; }
};

int main() {
    Box<int> bi{};
    bi.set(42);
    int xi = bi.get();

    Box<double> bd{};
    bd.set(3.14);
    double xd = bd.get();

    return 0;
}
