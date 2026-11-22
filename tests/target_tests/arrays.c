int sumrow(int row[], int count) {
    int sum = 0;
    for (int i = 0; i < count; i++) sum += row[i];
    return sum;
}
int main() {
    int a[] = {1, 2, 3, 4};
    int m[2][3] = {{1, 2}, {4, 5, 6}};
    int cube[2][2][2] = {1, 2, 3, 4, 5, 6, 7, 8};
    char word[] = "hi";
    char chars[4] = {'a', 'b', 'c'};
    a[1] += 10;
    m[0][2] = a[1];
    cube[1][0][1]++;
    word[1] = 'o';
    printf("%d %d %d %s %s %d\n", sumrow(a, 4), m[0][2] + m[1][2], cube[1][0][1], word, chars, sizeof(m));
    return 0;
}
