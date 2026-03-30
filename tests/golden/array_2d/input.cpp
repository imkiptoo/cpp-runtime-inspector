// 2D array test
// Verify multi-dimensional arrays are properly encoded

int main() {
    int arr[3][4] = {{0}};  // Zero-initialize to avoid garbage values

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            arr[i][j] = i * 4 + j;
        }
    }

    int val = arr[1][2];
    return 0;
}
