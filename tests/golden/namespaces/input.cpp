// Namespace test
// Nested namespaces, qualified vs unqualified access, using-directives.

namespace math {
    int square(int x) { return x * x; }

    namespace detail {
        int triple(int x) { return x + x + x; }
    }
}

namespace text {
    int length() { return 7; }
}

int main() {
    int s = math::square(4);
    int t = math::detail::triple(5);

    using namespace math;
    int s2 = square(6);

    int len = text::length();
    return 0;
}
