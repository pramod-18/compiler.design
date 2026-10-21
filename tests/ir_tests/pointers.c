int main() {
    int x = 42;
    int a[5] = {1, 2, 3, 4, 5};
    char msg[] = "hello";

    int *p = &x;
    int *q;
    char *s = msg;

    *p = 100;
    q = p;
    *q = *p + 1;
    x = *p;

    p = &a[2];
    p = a;

    p = p + 1;
    p = p - 1;
    p++;
    p--;
    x = *(p + 2);
    x = *(a + 1);

    int *table[3];
    table[0] = &x;
    table[1] = p;
    table[2] = q;
    x = *table[0];
    *table[1] = 5;

    int **pp;
    pp = &p;
    **pp = 7;
    x = **pp;
    *pp = &x;

    if (p != 0) {
        x = x + 1;
    }

    while (*s != '\0') {
        s++;
    }

    return x;
}
