// Nested structs test
// Verify nested struct encoding

struct Inner {
    int x;
    int y;
};

struct Outer {
    Inner inner;
    int z;
};

int main() {
    Outer o{};
    o.inner.x = 1;
    o.inner.y = 2;
    o.z = 3;

    Inner* pi = &o.inner;
    pi->x = 10;

    return 0;
}
