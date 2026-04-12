// Command-line arguments test
// Read argc and the user-supplied portion of argv (skipping argv[0],
// whose value is the runner's tmpdir path).

int main(int argc, char** argv) {
    int n = argc;

    int total_chars = 0;
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        int len = 0;
        while (a[len] != '\0') ++len;
        total_chars += len;
    }

    return 0;
}
