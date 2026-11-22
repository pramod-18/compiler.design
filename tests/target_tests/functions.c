int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
int fib(int n) {
    if (n < 2) return n;
    return fib(n - 1) + fib(n - 2);
}
int weighted(int a, int b, int c, int d, int e, int f) {
    return a + 2*b + 3*c + 4*d + 5*e + 6*f;
}
void swap(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}
void banner(void) {
    printf("functions\n");
    return;
}
int main(void) {
    int a = 7, b = 9;
    banner();
    swap(&a, &b);
    printf("%d %d %d %d %d\n", factorial(6), fib(6), weighted(1, 2, 3, 4, 5, 6), a, b);
    printf("%d\n", weighted(fib(3), factorial(3), fib(4), factorial(2), fib(2), 1));
    return 0;
}
