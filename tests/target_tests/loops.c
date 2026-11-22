int main() {
    int i, sum = 0;
    for (i = 1; i <= 5; i++) {
        if (i == 3) continue;
        if (i == 5) break;
        sum += i;
    }
    i = 0;
    while (i < 4) {
        i++;
        if (i == 2) continue;
        sum += i;
    }
    do {
        i--;
        if (i == 2) continue;
        sum += i;
    } while (i > 0);
    until (i >= 3) {
        sum += i;
        i++;
    }
    for (;;) break;
    for (int j = 0; j < 2; j++) sum += j;
    printf("%d %d\n", sum, i);
    return 0;
}
