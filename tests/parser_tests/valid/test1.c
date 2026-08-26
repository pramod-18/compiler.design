// arithmetic operators and precedence

int main() {
    int a = 10, b = 3, c;
    float f = 2.5;

    c = a + b;
    c = a - b;
    c = a * b;
    c = a / b;
    c = a % b;

    c = -a;
    c = +b;

    // precedence: * and / bind tighter than + and -
    c = a + b * 2 - 6 / 3;
    c = (a + b) * (a - b);

    // left associative
    c = a - b - 1;

    // used to lex wrong -- `a-5` came out as `a` then `-5`
    c = a - 5;

    f = f * 2.0 + 1.5;
    f = a / 2.0;

    return 0;
}
