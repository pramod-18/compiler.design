// for, while, do-while

int main() {
    int i, j, n = 10, sum = 0;

    // declaration in the initialiser
    for (int k = 0; k < n; k++) {
        sum += k;
    }

    // comma operator, empty body
    for (i = 0, j = n; i < j; i++, j--) ;

    // every section left out
    for (;;) {
        break;
    }

    i = 0;
    while (i < n) {
        sum = sum + i;
        i++;
    }

    // body runs at least once
    i = 0;
    do {
        sum--;
        i++;
    } while (i < n);

    // nested
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            sum += i * j;
        }
    }

    return 0;
}
