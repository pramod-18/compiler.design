// file manipulation.
// FILE is pre-registered as a type name in parser.y -- we don't run a
// preprocessor, so stdio.h is never actually read.

#include <stdio.h>

int main() {
    FILE *fp;
    int i, n = 5;
    char line[80];

    // write
    fp = fopen("output.txt", "w");
    if (fp == 0) {
        printf("cannot open for writing\n");
        return 1;
    }
    fprintf(fp, "count = %d\n", n);
    for (i = 0; i < n; i++) {
        fprintf(fp, "%d\n", i * i);
    }
    fclose(fp);

    // read it back
    fp = fopen("output.txt", "r");
    if (fp != 0) {
        fscanf(fp, "%d", &i);
        fgets(line, 80, fp);
        fclose(fp);
    }

    return 0;
}
