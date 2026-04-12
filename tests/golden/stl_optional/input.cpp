// std::optional test
// Verify has_value() vs empty + payload value over time.

#include <optional>

int main() {
    std::optional<int> empty;
    std::optional<int> with_value = 42;

    int v = with_value.value();

    with_value.reset();

    std::optional<int> assigned;
    assigned = 7;

    return 0;
}
