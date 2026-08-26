// arrays: int, char, multi-dimensional, initialiser lists

int main() {
    int a[5];
    int b[5] = {1, 2, 3, 4, 5};
    int c[] = {10, 20, 30};          // size inferred

    char word[20];
    char hi[6] = {'h', 'e', 'l', 'l', 'o', '\0'};
    char msg[] = "hello world";

    int grid[3][4];
    int cube[2][2][2];
    int m[2][3] = {{1, 2, 3}, {4, 5, 6}};

    int i, j, sum = 0;

    a[0] = 1;
    a[4] = a[0] + b[2];
    a[b[0]] = c[2];                  // index is itself an expression

    grid[1][2] = 7;
    grid[2][3] = grid[1][2] * 2;
    cube[0][1][1] = 3;
    m[1][2] = m[0][0];

    word[0] = hi[0];

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 4; j++) {
            grid[i][j] = i * 4 + j;
            sum += grid[i][j];
        }
    }

    return 0;
}
