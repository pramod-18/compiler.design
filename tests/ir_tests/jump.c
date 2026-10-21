int main() {
    int i, j, n = 10, sum = 0;

    for (i = 0; i < n; i++) {
        if (i == 3) continue;
        if (i == 8) break;
        sum += i;
    }

    i = 0;
    while (i < 10) {
        i++;
        if (i % 2 == 0) continue;
        sum += i;
    }

    for (i = 0; i < 5; i++) {
        for (j = 0; j < 5; j++) {
            if (j == 3) break;
            sum += 1;
        }
    }

    for (i = 0; i < 10; i++) {
        if (i == 5) continue;
        sum += i;
    }

    i = 0;
    do {
        i++;
        if (i == 2) continue;
        sum += 1;
    } while (i < 4);

    i = 0;
retry:
    i++;
    if (i < 3) goto retry;

    if (sum > 0) goto done;
    sum = 0;

done:
    return sum;
}
