int main() {
    int i, j, n = 10, sum = 0;

    i = 0;
    while (i < n) {
        sum += i;
        i++;
    }

    while (sum > 100) sum = sum - 10;

    for (int k = 0; k < 5; k++) {
        sum = sum + k;
    }

    for (i = 0; i < n; i++) sum += 1;
    for (; i > 0; ) i--;
    for (;;) break;

    for (i = 0, j = 10; i < j; i++, j--) {
        sum = sum + 1;
    }

    i = 100;
    do {
        sum = sum + 1;
        i--;
    } while (i > 0);


    i = 0;
    until (i >= n) {
        sum += 2;
        i++;
    }


    for (i = 0; i < 3; i++) {
        j = 0;
        while (j < 3) {
            sum += i * j;
            j++;
        }
    }


    do sum--; while (sum > 50);

    return sum;
}
