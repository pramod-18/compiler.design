static int counter = 0;             // static

extern int elsewhere;               // extern

enum colour { RED, GREEN, BLUE };   // enum

union value {                       // union
    int i;
    float f;
};

int main() {
    int x = 1;

    switch (x) {                    // switch / case / default
        case 1:
            x = 2;
            break;
        default:
            x = 0;
    }

    register int fast = 0;          // register
    auto int local = 0;             // auto

    return 0;
}
