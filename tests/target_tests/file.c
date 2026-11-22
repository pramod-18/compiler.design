int main() {
    FILE *fp = fopen("target-file.txt", "w");
    int n, count;
    float f;
    char c, word[32], line[64];
    if (fp == 0) return 1;
    fprintf(fp, "%d %f %c %s\n", 41, 2.5, 'Z', "hello");
    fclose(fp);
    fp = fopen("target-file.txt", "a");
    fputs("second", fp);
    fputc('\n', fp);
    fclose(fp);
    fp = fopen("target-file.txt", "r");
    count = fscanf(fp, "%d %f %c %s", &n, &f, &c, word);
    printf("%d %d %f %c %s\n", count, n, f, c, word);
    fgets(line, 64, fp);
    fgets(line, 64, fp);
    printf("%s%d\n", line, fgetc(fp));
    fclose(fp);
    fp = fopen("no-such-target-file.txt", "r");
    printf("%d\n", fp == 0);
    return 0;
}
