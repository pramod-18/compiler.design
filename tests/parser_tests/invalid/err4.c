int main() {
    int a = 1, b = 2, c = 0;

    c = a + ;               // no right operand
    c = a * / b;            // / can't start an operand
    c = a ? b;              // ternary with no : branch
    c = ;                   // nothing on the right at all
    c = a b;                // two operands, no operator

    if a > b {              // condition not in parens
        c = 1;
    }

    while () {              // empty condition
        c = 2;
    }

    for (a = 0 a < 5; a++) {    // missing ; after the initialiser
        c = c + a;
    }

    return c;
}
