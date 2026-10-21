#include <stdio.h>

int main() {
    FILE *fp;
    int i, n = 5;
    char line[80];
    float f = 1.25;

    fp = fopen("output.txt", "w");
    if (fp == 0) {
        printf("cannot open for writing\n");
        return 1;
    }
    fprintf(fp, "count = %d\n", n);
    for (i = 0; i < n; i++) {
        fprintf(fp, "%d\n", i * i);
    }
    fprintf(fp, "%s %f %c\n", "done", f, '!');
    fclose(fp);

    fp = fopen("output.txt", "r");
    if (fp != 0) {
        fscanf(fp, "%d", &i);
        fgets(line, 80, fp);

        FILE *log = fopen("log.txt", "w");
        fprintf(log, "read %d\n", i);
        fclose(log);

        fclose(fp);
    }

    return 0;
}
