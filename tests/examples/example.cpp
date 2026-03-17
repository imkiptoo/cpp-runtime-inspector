// A trivial program to instrument. We deliberately keep this to plain `int`
// variables and basic assignments so the PoC plugin can handle it.

int square(int n)
{
    int result = n * n;
    return result;
}

int main()
{
    int a = 3;
    int b = 4;
    int sum = a + b;     // sum = 7
    sum = sum + 1;       // exercise var_update: sum should become 8
    int sq = square(sum); // sq = 64
    return 0;
}
