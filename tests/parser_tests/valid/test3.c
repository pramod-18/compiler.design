// if / else if / else, nesting, and the dangling else

int main() {
    int n = 10, r = 0;

    if (n > 100) {
        r = 1;
    } else if (n > 10) {
        r = 2;
    } else {
        r = 3;
    }

    // no braces
    if (n) r = r + 1;

    // the else belongs to the inner if
    if (n > 0)
        if (n > 5) r = 4;
        else r = 5;

    // nested with braces, so it's unambiguous
    if (n > 0) {
        if (n > 5) {
            r = 6;
        } else {
            r = 7;
        }
    }

    return 0;
}
