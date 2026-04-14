// Test constexpr and const global variables with type aliases
#include <cstdint>
#include <cstddef>

enum class Season : int {
    spring = 0,
    summer = 4,
    autumn = 8,
    winter = 12
};

// Constexpr globals with various types
constexpr double pi = 3.14259;
constexpr int32_t magic = 42;
constexpr uint8_t byte_val = 0xFF;
constexpr int64_t big_num = 9223372036854775807LL;
constexpr float ratio = 1.618f;
constexpr bool flag = true;

// Non-constexpr globals
Season current_season = Season::summer;
size_t buffer_size = 1024;

int main() {
    Season first = Season::spring;
    int32_t local = magic * 2;
    return 0;
}
