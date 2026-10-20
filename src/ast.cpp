#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "ast.h"


void print_table(const char *title, const std::vector<TokenRow> &rows) {
    printf("\n=== %s ===\n", title);
    printf("%-20s %s\n", "Token", "Token_Type");
    printf("%-20s %s\n", "-------------------", "-----------------");
    for (size_t i = 0; i < rows.size(); i++)
        printf("%-20s %s\n", rows[i].lexeme.c_str(), rows[i].type.c_str());
}

void print_symbol_table() {
    // globals first, then locals in depth order
    std::vector<Symbol> sorted = symbols;
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const Symbol &a, const Symbol &b) { return a.depth < b.depth; });

    std::vector<TokenRow> rows;
    for (size_t i = 0; i < sorted.size(); i++)
        rows.push_back({ sorted[i].name, sorted[i].type });
    print_table("SYMBOL TABLE", rows);
}


// AST

const char *node_kind_name(NodeKind k) {
    switch (k) {
        case N_PROGRAM:  return "PROGRAM";
        case N_FUNC:     return "FUNC";
        case N_DECL:     return "DECL";
        case N_INT:      return "INT";
        case N_FLOAT:    return "FLOAT";
        case N_CHAR:     return "CHAR";
        case N_STR:      return "STR";
        case N_ID:       return "ID";
        case N_BIN:      return "BIN";
        case N_UN:       return "UN";
        case N_ASSIGN:   return "ASSIGN";
        case N_TERNARY:  return "TERNARY";
        case N_CALL:     return "CALL";
        case N_INDEX:    return "INDEX";
        case N_MEMBER:   return "MEMBER";
        case N_CAST:     return "CAST";
        case N_SIZEOF:   return "SIZEOF";
        case N_LIST:     return "LIST";
        case N_INITLIST: return "INITLIST";
        case N_IF:       return "IF";
        case N_WHILE:    return "WHILE";
        case N_UNTIL:    return "UNTIL";
        case N_DO:       return "DO";
        case N_FOR:      return "FOR";
        case N_BLOCK:    return "BLOCK";
        case N_EXPRSTMT: return "EXPRSTMT";
        case N_RETURN:   return "RETURN";
        case N_BREAK:    return "BREAK";
        case N_CONTINUE: return "CONTINUE";
        case N_GOTO:     return "GOTO";
        case N_LABEL:    return "LABEL";
    }
    return "?";
}

static std::string type_str(DeclInfo *d) {
    if (!d) return "";
    std::string s = d->spec ? d->spec->base : "";
    for (int i = 0; i < d->ptr_depth; i++) s += "*";
    for (size_t i = 0; i < d->dims.size(); i++)
        s += "[" + (d->dims[i].empty() ? std::string("?") : d->dims[i]) + "]";
    return s;
}

static void dump(Node *n, int indent) {
    if (!n) return;
    std::string pad(indent, ' ');

    printf("%s%s", pad.c_str(), node_kind_name(n->kind));
    if (!n->op.empty())     printf(" op='%s'", n->op.c_str());
    if (!n->name.empty())   printf(" name=%s", n->name.c_str());
    if (!n->value.empty())  printf(" value=%s", n->value.c_str());
    if (n->line)            printf(" [line %d]", n->line);

    if (n->kind == N_DECL) {
        printf(" :: ");
        for (size_t i = 0; i < n->decls.size(); i++) {
            printf("%s%s", i ? ", " : "", n->decls[i]->name.c_str());
            printf(":%s", type_str(n->decls[i]).c_str());
        }
    } else if (n->kind == N_FUNC) {
        printf(" :: %s", n->decl ? n->decl->name.c_str() : "?");
        std::string t = n->spec ? n->spec->base : "?";
        printf(" -> %s", t.c_str());
        if (n->decl) {
            printf(" (");
            for (size_t i = 0; i < n->decl->params.size(); i++)
                printf("%s%s", i ? ", " : "", type_str(n->decl->params[i]).c_str());
            printf(")");
        }
        if (n->spec && !n->spec->members.empty()) {
            printf(" { ");
            for (size_t i = 0; i < n->spec->members.size(); i++)
                printf("%s%s", i ? ", " : "", n->spec->members[i]->name.c_str());
            printf(" }");
        }
    } else if (n->kind == N_MEMBER) {
        printf(" .%s", n->name.c_str());
    }
    printf("\n");

    for (size_t i = 0; i < n->kids.size(); i++)
        dump(n->kids[i], indent + 2);
}

void dump_ast(Node *n, int indent) {
    dump(n, indent);
}
