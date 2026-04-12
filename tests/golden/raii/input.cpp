// RAII test
//
// Verify that constructors and destructors fire predictably across nested
// scopes, and that the side-effects of dtors (releasing a resource via a
// counter) show up in the trace at the right moment.

struct Counter {
    int active;
    int totalCreated;
};

static Counter g_counter{};

struct Guard {
    int id;

    Guard(int i) : id(i) {
        g_counter.active = g_counter.active + 1;
        g_counter.totalCreated = g_counter.totalCreated + 1;
    }

    ~Guard() {
        g_counter.active = g_counter.active - 1;
    }
};

int main() {
    {
        Guard g1(1);   // active 1
        {
            Guard g2(2);   // active 2
            Guard g3(3);   // active 3
        }                  // ~g3, ~g2 -> active 1
    }                      // ~g1 -> active 0

    Guard final_guard(99); // active 1, total 4
    return 0;              // ~final_guard at scope exit
}
