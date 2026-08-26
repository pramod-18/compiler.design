int x y;                    // no comma between the declarators

struct box {
    int w                   // missing ; after this member
    int h;
};

int f(int a, ) {            // trailing comma leaves an empty parameter
    return a;
}

int g(int a int b) {        // no comma between parameters
    return a + b;
}

int main() {
    int * ;                 // pointer with nothing to name
    char [10] buf;          // dims shd appear after the name
    int 3count;             // wrong lex
    return 0;
}
