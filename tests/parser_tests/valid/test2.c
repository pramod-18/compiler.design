// relational, logical, bitwise, compound assignment, ternary, ++ and --

int main() {
    int a = 10, b = 3, c = 0;
    int flag;

    // relational
    flag = a < b;
    flag = a >= b;
    flag = a == b;
    flag = a != b;

    // logical
    flag = a && b;
    flag = a || b;
    flag = !(a && b) || (a > b);

    // bitwise
    c = a & b;
    c = a | b;
    c = a ^ b;
    c = ~a;
    c = a << 2;
    c = a >> 1;

    // compound assignment
    a += 5;
    a -= 2;
    a *= 2;
    a /= 4;
    a %= 3;
    a <<= 1;
    a &= 12;
    a |= 3;

    // ternary, plain and nested
    c = a > b ? a : b;
    c = a > b ? (a > 0 ? a : -a) : b;

    // pre and post
    c = a++;
    c = ++a;
    c = b--;
    c = --b;

    // chained assignment is right associative
    a = b = c = 7;

    return 0;
}
