%{

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

// from lexer.l
extern int yylex();
extern char *yytext;
extern FILE *yyin;
extern int lexical_error_count;
extern int token_line;      
extern int token_column;    

void yyerror(const char *msg);

int syntax_error_count = 0;



struct TypeInfo {
    std::string base;       // "INT", "STRUCT point", "UNSIGNED LONG", etc...
};

struct DeclInfo {
    std::string name;
    int  ptr_depth   = 0;                 // how many *
    bool is_function = false;
    std::vector<std::string> dims;        // array dims (outermost first)
};

struct DeclList {
    std::vector<DeclInfo *> items;
};

static TypeInfo *make_type(const std::string &base) {
    TypeInfo *t = new TypeInfo();
    t->base = base;
    return t;
}

// attaches specifiers together so "unsigned" + "long" + "int" is one thing
static TypeInfo *join_type(TypeInfo *a, TypeInfo *b) {
    a->base += " ";
    a->base += b->base;
    return a;
}

static DeclInfo *make_decl(const char *name) {
    DeclInfo *d = new DeclInfo();
    d->name = name ? name : "";
    return d;
}

// base type + declarator -> INT, CHAR[20], INT*, INT[3][4] 
static std::string type_string(const TypeInfo *t, const DeclInfo *d) {
    std::string s = t ? t->base : "?";
    for (int i = 0; i < d->ptr_depth; i++) s += "*";
    for (size_t i = 0; i < d->dims.size(); i++) s += "[" + d->dims[i] + "]";
    return s;
}



static std::set<std::string> typedef_names = { "FILE", "size_t" };

static int in_typedef = 0;   

extern "C" int is_typedef_name(const char *name) {
    return typedef_names.count(name) ? 1 : 0;
}

// Token table

struct TokenRow {
    std::string lexeme;
    std::string type;
};
static std::vector<TokenRow> token_rows;


extern "C" void record_token(const char *lexeme, const char *type) {
    token_rows.push_back({ lexeme, type });
}


static std::string last_bracket_text() {
    int end = -1;
    for (int i = (int)token_rows.size() - 1; i >= 0; i--)
        if (token_rows[i].lexeme == "]") { end = i; break; }
    if (end < 0) return "_";

    int depth = 0, start = -1;
    for (int i = end; i >= 0; i--) {
        if (token_rows[i].lexeme == "]") depth++;
        else if (token_rows[i].lexeme == "[") {
            depth--;
            if (depth == 0) { start = i; break; }
        }
    }
    if (start < 0) return "_";

    std::string s;
    for (int i = start + 1; i < end; i++) s += token_rows[i].lexeme;
    return s;
}

// Symbol table structure. Each symbol gets a scope_id

struct Symbol {
    std::string name;
    std::string type;   // the Token_Type column: INT, CHAR[20], PROCEDURE, etc...
    int scope_id;
    int depth;
    int line;
};
static std::vector<Symbol> symbols;

static int next_scope_id = 1;
static std::vector<int> scope_stack = { 0 };   

static void push_scope() { scope_stack.push_back(next_scope_id++); }
static void pop_scope()  { if (scope_stack.size() > 1) scope_stack.pop_back(); }

static void add_symbol(const std::string &name, const std::string &type, int line) {
    if (name.empty()) return;
    int scope = scope_stack.back();
    // check if it's already declared in the same scope
    for (size_t i = 0; i < symbols.size(); i++)
        if (symbols[i].name == name && symbols[i].scope_id == scope) return;
    symbols.push_back({ name, type, scope, (int)scope_stack.size() - 1, line });
}

// function params get declared before scope opens (with {), so store them and flush once scope opens.
static std::vector<Symbol> pending_params;

static void flush_pending_params() {
    for (size_t i = 0; i < pending_params.size(); i++)
        add_symbol(pending_params[i].name, pending_params[i].type, pending_params[i].line);
    pending_params.clear();
}

// records every declarator in a declaration, and registers typedef names
static void declare_all(TypeInfo *t, DeclList *l, int line) {
    if (!l) return;
    for (size_t i = 0; i < l->items.size(); i++) {
        DeclInfo *d = l->items[i];
        if (in_typedef) {
            typedef_names.insert(d->name);
            add_symbol(d->name, "TYPEDEF " + type_string(t, d), line);
        } else if (d->is_function) {
            add_symbol(d->name, "PROCEDURE", line);
        } else {
            add_symbol(d->name, type_string(t, d), line);
        }
    }
}


static std::vector<std::string> struct_stack;

static void push_struct(const char *tag) {
    struct_stack.push_back(tag ? tag : "<anonymous>");
}
static void pop_struct() {
    if (!struct_stack.empty()) struct_stack.pop_back();
}

static void declare_members(TypeInfo *t, DeclList *l, int line) {
    if (!l || struct_stack.empty()) return;
    for (size_t i = 0; i < l->items.size(); i++)
        add_symbol(struct_stack.back() + "." + l->items[i]->name,
                   type_string(t, l->items[i]), line);
}

%}


%code requires {
    struct TypeInfo;
    struct DeclInfo;
    struct DeclList;
}

%union {
    char     *str;
    int       ival;
    TypeInfo *type;
    DeclInfo *decl;
    DeclList *dlist;
}

%define parse.error verbose


%token <str> IDENTIFIER TYPE_NAME
%token <str> INT_LIT FLOAT_LIT CHAR_LIT STRING_LIT


%token BREAK CHAR CONST CONTINUE DO DOUBLE ELSE FLOAT FOR GOTO IF INT LONG
%token RETURN SHORT SIGNED SIZEOF STRUCT TYPEDEF UNSIGNED UNTIL VOID VOLATILE
%token WHILE
%token AUTO CASE DEFAULT ENUM EXTERN INLINE REGISTER RESTRICT STATIC
%token SWITCH UNION


%token INC DEC ARROW ELLIPSIS
%token LE GE EQ NE AND_OP OR_OP LEFT_OP RIGHT_OP
%token ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN
%token LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN XOR_ASSIGN OR_ASSIGN


%type <str>   struct_tag
%type <ival>  pointer
%type <type>  declaration_specifiers type_specifier type_qualifier
%type <type>  struct_specifier type_name
%type <decl>  declarator direct_declarator init_declarator abstract_declarator
%type <decl>  struct_declarator parameter_declaration
%type <dlist> init_declarator_list struct_declarator_list
%type <dlist> parameter_list parameter_type_list parameter_type_list_opt


// Precedence, lowest first. Resolves dangling if-else
%nonassoc IFX
%nonassoc ELSE

%right '=' ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN
%right LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN XOR_ASSIGN OR_ASSIGN
%right '?' ':'
%left  OR_OP
%left  AND_OP
%left  '|'
%left  '^'
%left  '&'
%left  EQ NE
%left  '<' '>' LE GE
%left  LEFT_OP RIGHT_OP
%left  '+' '-'
%left  '*' '/' '%'
%right UNARY_PREC

%start translation_unit

%%



translation_unit
    : external_declaration
    | translation_unit external_declaration
    ;

external_declaration
    : function_definition
    | declaration
    | error ';'   { yyerrok; }
    ;


function_definition
    : declaration_specifiers declarator compound_statement
      { add_symbol($2->name, "PROCEDURE", token_line); }
    ;



declaration
    : declaration_specifiers init_declarator_list ';'
      { declare_all($1, $2, token_line); in_typedef = 0; pending_params.clear(); }
    | typedef_kw declaration_specifiers init_declarator_list ';'
      { declare_all($2, $3, token_line); in_typedef = 0; pending_params.clear(); }
    | declaration_specifiers ';'          
      { in_typedef = 0; pending_params.clear(); }
    ;


typedef_kw
    : TYPEDEF   { in_typedef = 1; }
    ;

declaration_specifiers
    : type_specifier                          { $$ = $1; }
    | type_specifier declaration_specifiers   { $$ = join_type($1, $2); }
    | type_qualifier                          { $$ = $1; }
    | type_qualifier declaration_specifiers   { $$ = join_type($1, $2); }
    ;

type_specifier
    : VOID              { $$ = make_type("VOID"); }
    | CHAR              { $$ = make_type("CHAR"); }
    | SHORT             { $$ = make_type("SHORT"); }
    | INT               { $$ = make_type("INT"); }
    | LONG              { $$ = make_type("LONG"); }
    | FLOAT             { $$ = make_type("FLOAT"); }
    | DOUBLE            { $$ = make_type("DOUBLE"); }
    | SIGNED            { $$ = make_type("SIGNED"); }
    | UNSIGNED          { $$ = make_type("UNSIGNED"); }
    | struct_specifier  { $$ = $1; }
    | TYPE_NAME         { $$ = make_type($1); free($1); }
    ;

type_qualifier
    : CONST     { $$ = make_type("CONST"); }
    | VOLATILE  { $$ = make_type("VOLATILE"); }
    ;


struct_specifier
    : STRUCT struct_tag '{' { push_struct($2); } struct_declaration_list '}'
      { $$ = make_type(std::string("STRUCT ") + $2);
        add_symbol($2, "STRUCT", token_line);
        pop_struct(); free($2); }
    | STRUCT '{' { push_struct(NULL); } struct_declaration_list '}'
      { $$ = make_type("STRUCT <anonymous>"); pop_struct(); }
    | STRUCT struct_tag
      { $$ = make_type(std::string("STRUCT ") + $2); free($2); }
    ;

struct_tag
    : IDENTIFIER  { $$ = $1; }
    ;

struct_declaration_list
    : struct_declaration
    | struct_declaration_list struct_declaration
    ;

struct_declaration
    : declaration_specifiers struct_declarator_list ';'
      { declare_members($1, $2, token_line); }
    | declaration_specifiers ';'
    ;

struct_declarator_list
    : struct_declarator                            { $$ = new DeclList(); $$->items.push_back($1); }
    | struct_declarator_list ',' struct_declarator { $1->items.push_back($3); $$ = $1; }
    ;

struct_declarator
    : declarator  { $$ = $1; }
    ;

init_declarator_list
    : init_declarator                            { $$ = new DeclList(); $$->items.push_back($1); }
    | init_declarator_list ',' init_declarator   { $1->items.push_back($3); $$ = $1; }
    ;

init_declarator
    : declarator                  { $$ = $1; }
    | declarator '=' initializer  { $$ = $1; }
    ;

declarator
    : pointer direct_declarator   { $2->ptr_depth += $1; $$ = $2; }
    | direct_declarator           { $$ = $1; }
    ;


pointer
    : '*'                         { $$ = 1; }
    | '*' pointer                 { $$ = $2 + 1; }
    | '*' type_qualifier          { $$ = 1; }
    | '*' type_qualifier pointer  { $$ = $3 + 1; }
    ;


direct_declarator
    : IDENTIFIER                                       { $$ = make_decl($1); free($1); }
    | '(' declarator ')'                               { $$ = $2; }
    | direct_declarator '[' assignment_expression ']'  { $1->dims.push_back(last_bracket_text()); $$ = $1; }
    | direct_declarator '[' ']'                        { $1->dims.push_back(""); $$ = $1; }
    | direct_declarator '(' parameter_type_list_opt ')' { $1->is_function = true; $$ = $1; }
    ;

parameter_type_list_opt
    : /* empty */          { $$ = NULL; }
    | parameter_type_list  { $$ = $1; }
    ;

parameter_type_list
    : parameter_list                { $$ = $1; }
    | parameter_list ',' ELLIPSIS   { $$ = $1; }
    ;

parameter_list
    : parameter_declaration                     { $$ = new DeclList(); $$->items.push_back($1); }
    | parameter_list ',' parameter_declaration  { $1->items.push_back($3); $$ = $1; }
    ;

parameter_declaration
    : declaration_specifiers declarator
      { $$ = $2;
        pending_params.push_back({ $2->name, type_string($1, $2), 0, 0, token_line }); }
    | declaration_specifiers abstract_declarator  { $$ = $2; }
    | declaration_specifiers                      { $$ = make_decl(""); }
    ;


type_name
    : declaration_specifiers                      { $$ = $1; }
    | declaration_specifiers abstract_declarator  { $$ = $1; }
    ;

abstract_declarator
    : pointer                        { $$ = make_decl(""); $$->ptr_depth = $1; }
    | '[' ']'                        { $$ = make_decl(""); $$->dims.push_back(""); }
    | '[' assignment_expression ']'  { $$ = make_decl(""); $$->dims.push_back("_"); }
    | pointer '[' ']'                { $$ = make_decl(""); $$->ptr_depth = $1; $$->dims.push_back(""); }
    ;

initializer
    : assignment_expression
    | '{' initializer_list '}'
    | '{' initializer_list ',' '}'
    ;

initializer_list
    : initializer
    | initializer_list ',' initializer
    ;



statement
    : labeled_statement
    | compound_statement
    | expression_statement
    | selection_statement
    | iteration_statement
    | jump_statement
    | error ';'   { yyerrok; }   
    ;


labeled_statement
    : IDENTIFIER ':' statement   { free($1); }
    ;

compound_statement
    : '{' { push_scope(); flush_pending_params(); } block_item_list_opt '}'
      { pop_scope(); }
    ;


block_item_list_opt
    : /* empty */
    | block_item_list
    ;

block_item_list
    : block_item
    | block_item_list block_item
    ;

block_item
    : declaration
    | statement
    ;

expression_statement
    : ';'
    | expression ';'
    ;

selection_statement
    : IF '(' expression ')' statement %prec IFX
    | IF '(' expression ')' statement ELSE statement
    ;

iteration_statement
    : WHILE '(' expression ')' statement
    | DO statement WHILE '(' expression ')' ';'
    | UNTIL '(' expression ')' statement          // until loop syntax
    | FOR '(' for_init expression_statement ')' statement
    | FOR '(' for_init expression_statement expression ')' statement
    ;

for_init
    : expression_statement
    | declaration
    ;

jump_statement
    : GOTO IDENTIFIER ';'  { free($2); }
    | CONTINUE ';'
    | BREAK ';'
    | RETURN ';'
    | RETURN expression ';'
    ;



primary_expression
    : IDENTIFIER          { free($1); }
    | INT_LIT             { free($1); }
    | FLOAT_LIT           { free($1); }
    | CHAR_LIT            { free($1); }
    | string_literal
    | '(' expression ')'
    ;


string_literal
    : STRING_LIT                { free($1); }
    | string_literal STRING_LIT { free($2); }
    ;

postfix_expression
    : primary_expression
    | postfix_expression '[' expression ']'
    | postfix_expression '(' ')'
    | postfix_expression '(' argument_expression_list ')'
    | postfix_expression '.' IDENTIFIER      { free($3); }
    | postfix_expression ARROW IDENTIFIER    { free($3); }
    | postfix_expression INC
    | postfix_expression DEC
    ;


argument_expression_list
    : assignment_expression
    | argument_expression_list ',' assignment_expression
    ;

unary_expression
    : postfix_expression
    | INC unary_expression
    | DEC unary_expression
    | unary_operator cast_expression  %prec UNARY_PREC
    | SIZEOF unary_expression
    | SIZEOF '(' type_name ')'
    ;

unary_operator
    : '&' | '*' | '+' | '-' | '~' | '!'
    ;

cast_expression
    : unary_expression
    | '(' type_name ')' cast_expression
    ;


assignment_expression
    : cast_expression
    | assignment_expression '*' assignment_expression
    | assignment_expression '/' assignment_expression
    | assignment_expression '%' assignment_expression
    | assignment_expression '+' assignment_expression
    | assignment_expression '-' assignment_expression
    | assignment_expression LEFT_OP assignment_expression
    | assignment_expression RIGHT_OP assignment_expression
    | assignment_expression '<' assignment_expression
    | assignment_expression '>' assignment_expression
    | assignment_expression LE assignment_expression
    | assignment_expression GE assignment_expression
    | assignment_expression EQ assignment_expression
    | assignment_expression NE assignment_expression
    | assignment_expression '&' assignment_expression
    | assignment_expression '^' assignment_expression
    | assignment_expression '|' assignment_expression
    | assignment_expression AND_OP assignment_expression
    | assignment_expression OR_OP assignment_expression
    | assignment_expression '?' assignment_expression ':' assignment_expression
    | assignment_expression '=' assignment_expression
    | assignment_expression ADD_ASSIGN assignment_expression
    | assignment_expression SUB_ASSIGN assignment_expression
    | assignment_expression MUL_ASSIGN assignment_expression
    | assignment_expression DIV_ASSIGN assignment_expression
    | assignment_expression MOD_ASSIGN assignment_expression
    | assignment_expression LEFT_ASSIGN assignment_expression
    | assignment_expression RIGHT_ASSIGN assignment_expression
    | assignment_expression AND_ASSIGN assignment_expression
    | assignment_expression XOR_ASSIGN assignment_expression
    | assignment_expression OR_ASSIGN assignment_expression
    ;


expression
    : assignment_expression
    | expression ',' assignment_expression
    ;

%%


void yyerror(const char *msg) {
    syntax_error_count++;
    fprintf(stderr, "Syntax error at line %d, column %d: unexpected '%s' -- %s\n",
            token_line, token_column,
            (yytext && *yytext) ? yytext : "<end of file>", msg);
}



static void print_table(const char *title, const std::vector<TokenRow> &rows) {
    printf("\n=== %s ===\n", title);
    printf("%-20s %s\n", "Token", "Token_Type");
    printf("%-20s %s\n", "-------------------", "-----------------");
    for (size_t i = 0; i < rows.size(); i++)
        printf("%-20s %s\n", rows[i].lexeme.c_str(), rows[i].type.c_str());
}

static void print_symbol_table() {
    // prints globals and then locals in depth order
    std::vector<Symbol> sorted = symbols;
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const Symbol &a, const Symbol &b) { return a.depth < b.depth; });

    std::vector<TokenRow> rows;
    for (size_t i = 0; i < sorted.size(); i++)
        rows.push_back({ sorted[i].name, sorted[i].type });
    print_table("SYMBOL TABLE", rows);
}

static void usage(const char *prog) {
    fprintf(stderr, "usage: %s [-t | -s] [file]\n", prog);
    fprintf(stderr, "  -t   print the token table only\n");
    fprintf(stderr, "  -s   print the symbol table only\n");
    fprintf(stderr, "  with no file argument, reads standard input\n");
}

int main(int argc, char **argv) {
    bool want_tokens = true, want_symbols = true;
    const char *path = NULL;

    for (int i = 1; i < argc; i++) {
        if      (!strcmp(argv[i], "-t")) want_symbols = false;
        else if (!strcmp(argv[i], "-s")) want_tokens  = false;
        else if (!strcmp(argv[i], "-h")) { usage(argv[0]); return 0; }
        else if (argv[i][0] == '-')      { usage(argv[0]); return 2; }
        else                             path = argv[i];
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
        printf("\nParsing completed successfully: no errors.\n");
        return 0;
    }

    fprintf(stderr, "\nParsing failed: %d lexical error(s), %d syntax error(s).\n",
            lexical_error_count, syntax_error_count);
    return 1;
}
