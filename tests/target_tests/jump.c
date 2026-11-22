int count(int n) {
    int sum = 0;
again:
    if (n == 0) goto done;
    sum += n;
    n--;
    goto again;
done:
    return sum;
}
int main() {
    int x = count(4);
    goto done;
    x = 100;
done:
    printf("%d\n", x);
    return 0;
}
