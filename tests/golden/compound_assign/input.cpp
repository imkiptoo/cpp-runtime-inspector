// Compound assignment test
// Verify +=, -=, *=, /=, etc. are properly tracked

int main() {
    int x = 10;

    x += 5;  // 15
    x -= 3;  // 12
    x *= 2;  // 24
    x /= 4;  // 6

    int y = 8;
    y %= 3;  // 2
    y <<= 2; // 8
    y >>= 1; // 4
    y &= 5;  // 4
    y |= 2;  // 6
    y ^= 1;  // 7

    return 0;
}
