int main() {
    int a[4] = {10, 20, 30, 40};
    int *p = a;
    int *q = &a[3];
    *p = 11;
    p++;
    *p += 2;
    p += 1;
    p -= 1;
    printf("%d %d %d %d\n", a[0], *p, q - p, *(a + 2));
    char text[] = "abcd";
    char *s = text;
    s++;
    *s = 'X';
    printf("%s %c\n", text, *(s + 1));
    return 0;
}
