int main() {
    int a = 10, b = 3, c;
    float f = 2.5;

    c = a + b;
    c = a - b;
    c = a * b;
    c = a / b;
    c = a % b;

    c = a + b * 2 - 6 / 3;
    c = (a + b) * (a - b);
    c = a - b - 1;

    c = -a;
    c = +b;
    c = ~a;
    c = !a;

    c = a < b;
    c = a >= b;
    c = a == b;
    c = a != b;
    c = a && b;
    c = a || b;
    c = !(a && b) || (a > b);

    c = a & b;
    c = a | b;
    c = a ^ b;
    c = a << 2;
    c = a >> 1;

    a += 5;
    a -= 2;
    a *= 2;
    a /= 4;
    a %= 3;
    a <<= 1;
    a >>= 1;
    a &= 12;
    a |= 3;
    a ^= 1;

    c = a > b ? a : b;
    c = a > b ? (a > 0 ? a : -a) : b;

    int arr[4];
    c = a++;
    c = ++a;
    c = b--;
    c = --b;
    arr[1] = 5;
    arr[1]++;

    a = b = c = 7;

    f = f * 2.0 + 1.5;
    f = a / 2.0;

    return c;
}
