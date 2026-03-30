// Template function test
// Verify template instantiations are properly instrumented

template<typename T>
T add(T a, T b) {
    T result = a + b;
    return result;
}

int main() {
    int i = add(1, 2);
    double d = add(1.5, 2.5);
    return 0;
}
