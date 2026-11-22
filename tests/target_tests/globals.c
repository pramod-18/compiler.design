int total = (2 + 3) * 4;
int a[3] = {1, 2, 3};
char message[] = "global";
int *p = &total;
int bump(int n) { total += n; return total; }
int main() {
    printf("%d %d %s\n", total, a[0] + a[1] + a[2], message);
    {
        int total = 99;
        printf("%d\n", total);
    }
    int updated = bump(2);
    printf("%d %d\n", updated, *p);
    return 0;
}
