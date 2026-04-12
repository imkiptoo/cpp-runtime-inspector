// STL shared_ptr test
// Verify std::shared_ptr encoding and shared ownership.

#include <memory>

struct Resource {
    int id;
    int payload;
};

int main() {
    std::shared_ptr<Resource> a = std::make_shared<Resource>();
    a->id = 1;
    a->payload = 100;

    // Second owner
    std::shared_ptr<Resource> b = a;
    int via_b = b->payload;

    // Reset one owner, value still alive through `a`
    b.reset();
    int via_a = a->payload;

    a.reset();
    return 0;
}
