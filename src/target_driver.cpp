#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "semantic.h"
#include "targetgen.h"

extern int yyparse();
extern FILE *yyin;

static void usage(const char *prog) {
    fprintf(stderr, "usage: %s [-o <file.asm>] [-s] [source.c]\n", prog);
    fprintf(stderr, "  -o <file>  store SPIM MIPS32 assembly (default: stdout)\n");
    fprintf(stderr, "  -s         also print the symbol table\n");
    fprintf(stderr, "  -h         show help; no source argument reads stdin\n");
}

static bool same_file(const char *a, const char *b) {
    if (!a || !b) return false;
    std::error_code error;
    if (std::filesystem::equivalent(a, b, error)) return true;
    return std::filesystem::weakly_canonical(a, error) == std::filesystem::weakly_canonical(b, error);
}

int main(int argc, char **argv) {
    const char *path = nullptr, *outpath = nullptr;
    bool symbols_requested = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) { usage(argv[0]); return 0; }
        if (!strcmp(argv[i], "-s")) symbols_requested = true;
        else if (!strcmp(argv[i], "-o") && i + 1 < argc) outpath = argv[++i];
        else if (argv[i][0] == '-' || path) { usage(argv[0]); return 2; }
        else path = argv[i];
    }
    if (same_file(path, outpath)) {
        fprintf(stderr, "error: source and output paths must differ\n");
        return 2;
    }
    if (path) {
        yyin = fopen(path, "r");
        if (!yyin) { fprintf(stderr, "error: cannot open '%s'\n", path); return 2; }
    }
    int parsed = yyparse();
    if (path) fclose(yyin);
    std::string assembly;
    try {
        if (parsed || lexical_error_count || syntax_error_count)
            throw std::runtime_error("lexical/syntax errors -- no assembly written");
        if (semantic_check(g_root, path ? path : "<stdin>"))
            throw std::runtime_error("semantic errors -- no assembly written");
        assembly = generate_target(g_root);
    } catch (const std::exception &e) {
        if (outpath) remove(outpath);
        fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    if (symbols_requested) print_symbol_table();
    FILE *out = outpath ? fopen(outpath, "w") : stdout;
    if (!out) { fprintf(stderr, "error: cannot write '%s'\n", outpath); return 2; }
    bool failed = fwrite(assembly.data(), 1, assembly.size(), out) != assembly.size();
    if (outpath) failed = fclose(out) != 0 || failed;
    if (failed) { if (outpath) remove(outpath); fprintf(stderr, "error: writing assembly failed\n"); return 2; }
    if (outpath) fprintf(stderr, "wrote %s (SPIM MIPS32)\n", outpath);
    return 0;
}
