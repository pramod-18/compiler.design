int main() {
    int x = 5, y = 0;
    if (x > 3) {
        if (x == 5) y = 7;
        else y = 8;
    } else y = 9;
    if (x < 0) y = 100;
    printf("%d %d\n", y, x > 0 ? (x > 4 ? 11 : 12) : 13);
    return 0;
}
