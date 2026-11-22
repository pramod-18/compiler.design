int touches;
int touch() { touches++; return 1; }
int main() {
    int a[3] = {4, 5, 6};
    int i = 0, x;
    x = 0 && touch();
    x = 1 || touch();
    x = 1 && touch();
    x = 0 || touch();
    a[i++] += 10;
    x = a[i++]++;
    printf("%d %d %d %d %d\n", touches, i, a[0], a[1], x);
    x = (touch(), 7);
    printf("%d %d\n", touches, x);
    return 0;
}
