// Test constexpr and const global variables

enum class Season : int {
    spring = 0,
    summer = 4,
    autumn = 8,
    winter = 12
};

constexpr double pi = 3.14259;
Season one = Season::summer;

int main() {
    Season first = Season::spring;
    return 0;
}
