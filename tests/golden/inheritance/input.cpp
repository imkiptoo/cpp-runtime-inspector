// Class inheritance test
// Verify base class fields are properly included

class Base {
public:
    int baseVal;
    void setBase(int v) { baseVal = v; }
};

class Derived : public Base {
public:
    int derivedVal;
    void setDerived(int v) { derivedVal = v; }
};

int main() {
    Derived d{};
    d.baseVal = 10;
    d.derivedVal = 20;

    Base* bp = &d;
    bp->baseVal = 30;

    return 0;
}
