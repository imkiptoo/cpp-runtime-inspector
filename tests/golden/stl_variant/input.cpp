// std::variant test
// Verify the runtime index changes as different alternatives become active.

#include <variant>

int main() {
    std::variant<int, double> v;     // initial: holds first alternative (int=0)

    int idx0 = static_cast<int>(v.index());

    v = 42;
    int idx_after_int = static_cast<int>(v.index());

    v = 3.14;
    int idx_after_double = static_cast<int>(v.index());

    return 0;
}
