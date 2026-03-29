// STL unique_ptr test
// Verify std::unique_ptr is properly encoded

#include <memory>

struct Data {
    int value;
};

int main() {
    std::unique_ptr<Data> ptr = std::make_unique<Data>();
    ptr->value = 42;

    int v = ptr->value;

    ptr.reset();
    return 0;
}
