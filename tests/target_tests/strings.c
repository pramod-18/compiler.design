int main() {
    char buffer[128], copied[128];
    int n, count;
    float f;
    char c, word[32];
    int written = sprintf(buffer, "%d %f %c %s %%", -17, 2.5, 'Q', "word");
    strcpy(copied, buffer);
    count = sscanf(copied, "%d %f %c %s %%", &n, &f, &c, word);
    printf("%d %d %d %f %c %s %d %d\n", written, count, n, f, c, word, strlen(copied), strcmp(buffer, copied));
    printf("%d %d\n", atoi("  -123x"), abs(-9));
    puts("done");
    putchar('!');
    putchar('\n');
    return 0;
}
