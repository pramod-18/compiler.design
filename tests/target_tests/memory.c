int main() {
    int *p = (int *)malloc(3 * sizeof(int));
    int *z = (int *)calloc(3, sizeof(int));
    if (p == 0 || z == 0) return 1;
    p[0] = 3;
    p[1] = 4;
    p[2] = 5;
    p = (int *)realloc(p, 5 * sizeof(int));
    if (p == 0) return 1;
    p[3] = 6;
    p[4] = 7;
    printf("%d %d\n", p[0] + p[1] + p[2] + p[3] + p[4], z[0] + z[1] + z[2]);
    free(p);
    free(z);
    free(0);
    return 0;
}
