int matrix_sum(int a[][3], int rows) {
    int sum = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < 3; j++) sum += a[i][j];
    }
    return sum + sizeof(a);
}
int main() {
    int m[][3] = {{1, 2, 3}, {4, 5, 6}};
    int flat[][3] = {1, 2, 3, 4, 5, 6};
    printf("%d %d %d\n", matrix_sum(m, 2), flat[1][2], sizeof(flat));
    return 0;
}
