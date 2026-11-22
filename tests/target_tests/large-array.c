int main() {
    int a[9000];
    int n = 9;
    a[8999] = 77;
    printf("%d %d %d\n", a[8999], n, sizeof(a));
    return 0;
}
