// Rule-of-5 lifecycle tracking test
//
// Verify that the trace shows the correct lifecycle annotation for each
// special member function: default_ctor, copy_ctor, move_ctor, copy_assign,
// move_assign, and dtor.

struct Widget {
    int id;

    // Default constructor
    Widget() {
        this->id = 1;
    }

    // Copy constructor
    Widget(const Widget& other) {
        this->id = other.id + 100;
    }

    // Move constructor
    Widget(Widget&& other) {
        this->id = other.id + 200;
        other.id = -1;
    }

    // Copy assignment operator
    Widget& operator=(const Widget& other) {
        this->id = other.id + 1000;
        return *this;
    }

    // Move assignment operator
    Widget& operator=(Widget&& other) {
        this->id = other.id + 2000;
        other.id = -2;
        return *this;
    }

    // Destructor
    ~Widget() {
    }
};

int main() {
    Widget a;                                 // default_ctor
    Widget b = a;                             // copy_ctor
    Widget c = static_cast<Widget&&>(a);      // move_ctor
    Widget d;                                 // default_ctor
    d = b;                                    // copy_assign
    Widget e;                                 // default_ctor
    e = static_cast<Widget&&>(d);             // move_assign
    return 0;                                 // dtors for e, d, c, b, a
}
