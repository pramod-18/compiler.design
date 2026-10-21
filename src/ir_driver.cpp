#include <cstdio>
#include <cstring>
#include <string>

#include "ast.h"
#include "semantic.h"
#include "irgen.h"

extern int  yyparse();
extern FILE *yyin;

static void usage(const char *prog) {
    fprintf(stderr, "usage: %s [-o <file>] [-s] [file]\n", prog);
    fprintf(stderr, "  -o <file>  write the IR to <file> instead of stdout\n");
    fprintf(stderr, "  -s         also print the symbol table\n");
    fprintf(stderr, "  -h         this message\n");
    fprintf(stderr, "  with no file argument, reads standard input\n");
}

int main(int argc, char **argv) {
    const char *path = NULL, *outpath = NULL;
    bool want_symbols = false;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h")) { usage(argv[0]); return 0; }
        else if (!strcmp(argv[i], "-s")) want_symbols = true;
        else if (!strcmp(argv[i], "-o")) {
            if (i + 1 >= argc) { usage(argv[0]); return 2; }
            outpath = argv[++i];
        }
        else if (argv[i][0] == '-') { usage(argv[0]); return 2; }
        else path = argv[i];
    }

    if (path) {
        yyin = fopen(path, "r");
        if (!yyin) {
            fprintf(stderr, "error: cannot open '%s'\n", path);
            return 2;
        }
    }

    yyparse();

    if (lexical_error_count + syntax_error_count > 0) {
        fprintf(stderr, "\n%d lexical error(s), %d syntax error(s) -- no IR written.\n",
                lexical_error_count, syntax_error_count);
        return 1;
    }

    int sem_errors = semantic_check(g_root, path ? path : "<stdin>");
    if (sem_errors > 0) {
        fprintf(stderr, "\n%d semantic error(s) -- no IR written.\n", sem_errors);
        return 1;
    }

    if (want_symbols) print_symbol_table();

    std::string ir = generate_ir(g_root);

    if (outpath) {
        FILE *f = fopen(outpath, "w");
        if (!f) {
            fprintf(stderr, "error: cannot write '%s'\n", outpath);
            return 2;
        }
        fputs(ir.c_str(), f);
        fclose(f);
        fprintf(stderr, "wrote %s\n", outpath);
    } else {
        fputs(ir.c_str(), stdout);
    }

    return 0;
}
