// Range-based for loop test
// Iterate over an array and accumulate.

int main() {
    int nums[5] = {1, 2, 3, 4, 5};
    int sum = 0;
    for (int n : nums) {
        sum += n;
    }
    return 0;
}
