int main() {
    int n = 7;
    char c;
    float f;
    printf("%d\n", sscanf("x", "%d", &n));
    printf("%d\n", sscanf("", "%d", &n));
    int count = sscanf("-12Q", "%d%c", &n, &c);
    printf("%d %d %c\n", count, n, c);
    count = sscanf("1.25e2", "%f", &f);
    printf("%d %f\n", count, f);
    return 0;
}
