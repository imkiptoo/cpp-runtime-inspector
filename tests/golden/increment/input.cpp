// Increment/decrement test
// Verify ++x, x++, --x, x-- are properly tracked

int main() {
    int x = 5;

    ++x;  // 6
    x++;  // 7
    --x;  // 6
    x--;  // 5

    int y = 10;
    int a = ++y;  // y=11, a=11
    int b = y++;  // b=11, y=12
    int c = --y;  // y=11, c=11
    int d = y--;  // d=11, y=10

    return 0;
}
