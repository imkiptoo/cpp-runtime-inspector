// Bitfields test
// Verify the runtime extracts the right bits for narrow integer fields.

struct Flags {
    unsigned int a : 1;
    unsigned int b : 2;
    unsigned int c : 5;
    unsigned int d : 8;
    int signed_field : 4;   // signed bitfield with sign extension
};

int main() {
    Flags f{};
    f.a = 1;
    f.b = 3;
    f.c = 30;
    f.d = 255;
    f.signed_field = -3;

    return 0;
}
