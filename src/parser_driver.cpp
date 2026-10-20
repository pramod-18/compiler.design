#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "ast.h"

extern int  yyparse();
extern FILE *yyin;

static void usage(const char *prog) {
    fprintf(stderr, "usage: %s [-t | -s | --ast] [file]\n", prog);
    fprintf(stderr, "  -t      print the token table only\n");
    fprintf(stderr, "  -s      print the symbol table only\n");
    fprintf(stderr, "  --ast   print the parse tree\n");
    fprintf(stderr, "  with no file argument, reads standard input\n");
}

int main(int argc, char **argv) {
    bool want_tokens = true, want_symbols = true, want_ast = false;
    const char *path = NULL;

    for (int i = 1; i < argc; i++) {
        if      (!strcmp(argv[i], "-t"))    want_symbols = false;
        else if (!strcmp(argv[i], "-s"))    want_tokens  = false;
        else if (!strcmp(argv[i], "--ast")) { want_tokens = false; want_symbols = false;
                                              want_ast = true; }
        else if (!strcmp(argv[i], "-h"))    { usage(argv[0]); return 0; }
        else if (argv[i][0] == '-')         { usage(argv[0]); return 2; }
        else                                path = argv[i];
    }

    if (path) {
        yyin = fopen(path, "r");
        if (!yyin) {
            fprintf(stderr, "error: cannot open '%s'\n", path);
            return 2;
        }
    }

    yyparse();

    if (lexical_error_count + syntax_error_count == 0) {
        if (want_tokens)  print_table("TOKEN TABLE", token_rows);
        if (want_symbols) print_symbol_table();
        if (want_ast) {
            printf("\n=== AST ===\n");
            dump_ast(g_root);
        }
        printf("\nParsing completed successfully: no errors.\n");
        return 0;
    }

    fprintf(stderr, "\nParsing failed: %d lexical error(s), %d syntax error(s).\n",
            lexical_error_count, syntax_error_count);
    return 1;
}
