// printf / scanf, and dynamic memory with malloc / sizeof / casts / free

#include <stdio.h>
#include <stdlib.h>

int main() {
    int n, i;
    float ratio;
    char name[32];
    int *arr;
    char *buf;
    size_t bytes;

    printf("enter a count: ");
    printf("%d %s %f %c\n", 1, "text", 2.5, 'z');
    printf("no arguments\n");

    scanf("%d", &n);
    scanf("%s", name);
    scanf("%d %f", &i, &ratio);

    // sizeof on a type, on an expression, and bare
    bytes = sizeof(int);
    bytes = sizeof(char *);
    bytes = sizeof n;
    bytes = sizeof(arr[0]);

    // malloc with a cast, and the usual null check
    arr = (int *)malloc(n * sizeof(int));
    if (arr == 0) {
        printf("out of memory\n");
        return 1;
    }

    buf = (char *)calloc(64, sizeof(char));
    arr = (int *)realloc(arr, 2 * n * sizeof(int));

    for (i = 0; i < n; i++) {
        arr[i] = i * i;
    }

    free(arr);
    free(buf);
    return 0;
}
