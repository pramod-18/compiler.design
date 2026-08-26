// functions: prototypes, args, void, recursion

int factorial(int n);
int fib(int n);
int gcd(int a, int b);
void banner(void);
void swap(int *a, int *b);

// one recursive call per level
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// two per level
int fib(int n) {
    if (n < 2) return n;
    return fib(n - 1) + fib(n - 2);
}

// tail recursion
int gcd(int a, int b) {
    if (b == 0) return a;
    return gcd(b, a % b);
}

void banner(void) {
    return;
}

// pass by pointer, so the caller sees the change
void swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

int main() {
    int x = 12, y = 18, r;

    banner();

    // nested calls, and calls as operands
    r = factorial(5);
    r = gcd(factorial(3), fib(6));
    r = factorial(4) + fib(5) * 2;

    if (gcd(x, y) == 6) r = r + 1;

    swap(&x, &y);
    return 0;
}
