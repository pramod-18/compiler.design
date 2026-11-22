int main() {
    int n;
    float f;
    double d;
    char c, word[32];
    int count = scanf("%d %f %lf %c %s", &n, &f, &d, &c, word);
    printf("%d %d %f %f %c %s %d\n", count, n, f, d, c, word, getchar());
    return 0;
}
