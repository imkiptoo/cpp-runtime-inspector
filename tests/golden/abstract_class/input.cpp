// Abstract class test
//
// Verify pure-virtual functions can't instantiate a base, derived classes
// override them, and polymorphic delete via a base pointer dispatches to
// the correct dtor.

struct Animal {
    int kind;

    Animal() { this->kind = 0; }
    virtual ~Animal() {}

    virtual int legs() const = 0;     // pure virtual
    virtual int sound() const = 0;
};

struct Dog : Animal {
    int sentinel;

    Dog() { this->kind = 1; this->sentinel = 99; }
    ~Dog() override { this->sentinel = -1; }

    int legs() const override { return 4; }
    int sound() const override { return 100; }   // "woof" stand-in
};

struct Cat : Animal {
    int whiskers;

    Cat() { this->kind = 2; this->whiskers = 6; }
    ~Cat() override { this->whiskers = -1; }

    int legs() const override { return 4; }
    int sound() const override { return 200; }   // "meow" stand-in
};

int main() {
    Animal* a = new Dog();
    int l1 = a->legs();
    int s1 = a->sound();
    delete a;             // virtual dtor: ~Dog then ~Animal

    Animal* b = new Cat();
    int l2 = b->legs();
    int s2 = b->sound();
    delete b;             // virtual dtor: ~Cat then ~Animal

    return 0;
}
