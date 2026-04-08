// Special-member-function lifecycle test
//
// Verify the trace shows distinct entries for each rule-of-5 special
// member: default ctor, copy ctor, move ctor, copy assign, move assign
// and dtor. Each member sets the receiver's `mode` field so the trace
// captures which one fired.

struct Tracer {
    int mode;          // 0=default, 1=copy, 2=move, 3=copy=, 4=move=
    int sourceId;
    int id;

    Tracer() {
        this->mode = 0;
        this->sourceId = -1;
        this->id = 1;
    }

    Tracer(const Tracer& o) {
        this->mode = 1;
        this->sourceId = o.id;
        this->id = 100 + o.id;
    }

    Tracer(Tracer&& o) {
        this->mode = 2;
        this->sourceId = o.id;
        this->id = 200 + o.id;
        o.id = -1;          // moved-from sentinel
    }

    Tracer& operator=(const Tracer& o) {
        this->mode = 3;
        this->sourceId = o.id;
        return *this;
    }

    Tracer& operator=(Tracer&& o) {
        this->mode = 4;
        this->sourceId = o.id;
        o.id = -2;          // move-assigned-from sentinel
        return *this;
    }

    ~Tracer() {}
};

// Helper that takes by value (forces a copy or move depending on caller).
Tracer takeByValue(Tracer t) {
    return t;
}

int main() {
    Tracer a;                  // default ctor
    Tracer b = a;              // copy ctor
    Tracer c = static_cast<Tracer&&>(a);  // move ctor; a.id -> -1

    Tracer d;                  // another default
    d = b;                     // copy assignment
    Tracer e;
    e = static_cast<Tracer&&>(d);   // move assignment; d.id -> -2

    return 0;                  // 5 dtors fire in reverse declaration order
}
