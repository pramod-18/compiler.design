// pointers: & and *, pointer arithmetic, array of pointers

int main() {
    int x = 42;
    int a[5] = {1, 2, 3, 4, 5};
    char msg[] = "hello";

    int *p = &x;
    int *q;
    char *s = msg;

    // deref and address-of
    *p = 100;
    q = p;
    *q = *p + 1;
    x = *p;

    // point into an array
    p = &a[2];
    p = a;

    // pointer arithmetic
    p = p + 1;
    p = p - 1;
    p++;
    p--;
    x = *(p + 2);
    x = *(a + 1);

    // array of pointers
    int *table[3];
    table[0] = &x;
    table[1] = p;
    table[2] = q;
    x = *table[0];
    *table[1] = 5;

    // walk a string
    while (*s != '\0') {
        s++;
    }

    return 0;
}
