// Floating point test
// Verify float/double values are properly encoded

#include <cmath>

int main() {
    float f = 3.14f;
    double d = 2.71828;

    double sum = f + d;
    double product = f * d;

    // Test special values
    double inf = 1.0 / 0.0;
    double neg_inf = -1.0 / 0.0;
    double nan_val = 0.0 / 0.0;

    return 0;
}
