int main() {
    int a = 10, b = 3;
    printf("%d %d %d %d %d\n", a + b, a - b, a * b, a / b, a % b);
    printf("%d %d %d %d %d %d\n", a < b, a > b, a <= b, a >= b, a == b, a != b);
    printf("%d %d %d %d %d %d %d\n", a & b, a | b, a ^ b, a << 2, a >> 1, ~a, -b);
    printf("%d %d %d %d\n", !0, !a, a && b, 0 || b);
    a += 5;
    a -= 2;
    a *= 2;
    a /= 4;
    a %= 3;
    a <<= 2;
    a >>= 1;
    a |= 3;
    a &= 6;
    a ^= 1;
    printf("%d %d\n", a, a > b ? a : b);
    return 0;
}
