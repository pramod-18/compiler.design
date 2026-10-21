int factorial(int n);
int fib(int n);
int gcd(int a, int b);
void banner(void);
void swap(int *a, int *b);
int add3(int a, int b, int c);

int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int fib(int n) {
    if (n < 2) return n;
    return fib(n - 1) + fib(n - 2);
}

int gcd(int a, int b) {
    if (b == 0) return a;
    return gcd(b, a % b);
}

void banner(void) {
    int i;
    for (i = 0; i < 3; i++) {
        i = i + 0;
    }
    return;
}

void swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

int add3(int a, int b, int c) {
    return a + b + c;
}

int twice(int x) {
    return add3(x, x, 0);
}

int main() {
    int x = 12, y = 18, r;

    banner();

    r = factorial(5);
    r = gcd(factorial(3), fib(6));
    r = factorial(4) + fib(5) * 2;

    if (gcd(x, y) == 6) r = r + 1;

    int i;
    for (i = 0; i < factorial(3); i++) {
        r = r + twice(i);
    }

    swap(&x, &y);
    return r;
}
