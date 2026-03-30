// Globals and statics test
// Verify global and static variables are tracked

int globalCounter = 0;
double globalPi = 3.14159;

void incrementGlobal() {
    globalCounter += 1;
}

int getNext() {
    static int counter = 0;
    counter += 1;
    return counter;
}

int main() {
    int a = getNext();  // 1
    int b = getNext();  // 2
    int c = getNext();  // 3

    incrementGlobal();
    incrementGlobal();
    int g = globalCounter;  // 2

    return 0;
}
