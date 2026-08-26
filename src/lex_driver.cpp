#include <stdio.h>

#include "parser.tab.h"

extern int yylex();
extern char *yytext;
extern int lexical_error_count;
extern const char *token_class(int tok);

YYSTYPE yylval;

extern "C" int is_typedef_name(const char *name) {
    (void)name;
    return 0;
}

extern "C" void record_token(const char *lexeme, const char *type) {
    (void)lexeme;
    (void)type;
}

int main() {

    printf("\n%-16s%s\n", "Lexeme", "Token");
    printf("%-16s%s\n", "------", "-----");

    int tok;
    while ((tok = yylex()) != 0) {
        printf("%-15s %s\n", yytext, token_class(tok));
    }

    if (lexical_error_count > 0) {
        fprintf(stderr, "\nTotal errors: %d\n", lexical_error_count);
    }

    return 0;
}
