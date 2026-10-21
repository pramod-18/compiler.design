int main() {
    int n = 10, r = 0;

    if (n > 0) r = 1;

    if (n > 100) {
        r = 1;
    } else {
        r = 2;
    }

    if (n > 100) {
        r = 1;
    } else if (n > 10) {
        r = 2;
    } else if (n > 5) {
        r = 3;
    } else {
        r = 4;
    }

    if (n > 0)
        if (n > 5) r = 4;
        else r = 5;

    if (n > 0) {
        if (n > 5) {
            r = 6;
        } else {
            r = 7;
        }
    }

    if (n > 0 && r < 100) {
        r = r + 1;
    }

    if (n) { } else { }

    return r;
}
