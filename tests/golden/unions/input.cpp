// Union test
// Verify a union's storage is shared between members.

union Number {
    int as_int;
    float as_float;
};

int main() {
    Number n{};
    n.as_int = 0x40490FDB; // bit pattern for ~3.14159 as float

    int i = n.as_int;
    float f = n.as_float;

    n.as_float = 1.5f;
    int reinterpreted = n.as_int;

    return 0;
}
