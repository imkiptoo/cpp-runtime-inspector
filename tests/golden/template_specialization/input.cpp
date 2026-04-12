// Template specialization test
// Full specialization for one type, primary template for the rest.

template <class T>
T identity(T x) { return x; }

// Full specialization that doubles ints instead of returning them.
template <>
int identity<int>(int x) { return x + x; }

int main() {
    int a = identity<int>(5);       // 10 via specialization
    double b = identity<double>(2.5); // 2.5 via primary
    char c = identity<char>('A');     // 'A' via primary
    return 0;
}
