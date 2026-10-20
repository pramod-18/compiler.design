%{

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

#include "ast.h"

// from lexer.l
extern int yylex();
extern char *yytext;
extern FILE *yyin;
extern int lexical_error_count;

void yyerror(const char *msg);

int syntax_error_count = 0;


Node *g_root = NULL;


#define BIN(a, b, o)   mk_bin((o), (a), (b), token_line)


static Node *empty_stmt(int line) { return mk_node(N_BLOCK, line); }

static void append2(Node *list, Node *item, int line) {
    if (list) list->kids.push_back(item ? item : empty_stmt(line));
}


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
std::vector<TokenRow> token_rows;


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

// Each symbol gets a scope_id
std::vector<Symbol> symbols;

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
    #include "ast.h"
}

%union {
    char     *str;
    int       ival;
    TypeInfo *type;
    DeclInfo *decl;
    DeclList *dlist;
    Node     *node;
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
%type <dlist> struct_declaration struct_declaration_list
%type <dlist> parameter_list parameter_type_list parameter_type_list_opt

%type <node>  translation_unit external_declaration function_definition
%type <node>  declaration
%type <node>  initializer initializer_list
%type <node>  statement labeled_statement compound_statement block_item
%type <node>  block_item_list block_item_list_opt for_init
%type <node>  expression_statement selection_statement iteration_statement
%type <node>  jump_statement
%type <node>  primary_expression string_literal postfix_expression
%type <node>  argument_expression_list unary_expression cast_expression
%type <node>  assignment_expression expression
%type <ival>  unary_operator


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
      { $$ = mk_node(N_PROGRAM, token_line);
        append($$, $1);
        g_root = $$; }
    | translation_unit external_declaration
      { append($1, $2); $$ = $1; g_root = $$; }
    ;

external_declaration
    : function_definition   { $$ = $1; }
    | declaration           { $$ = $1; }
    | error ';'             { yyerrok; $$ = NULL; }
    ;


function_definition
    : declaration_specifiers declarator compound_statement
      { add_symbol($2->name, "PROCEDURE", token_line);
        $$ = mk_node(N_FUNC, token_line);
        $$->spec = $1;
        $$->decl = $2;
        append($$, $3); }
    ;


declaration
    : declaration_specifiers init_declarator_list ';'
      { declare_all($1, $2, token_line);
        $$ = mk_node(N_DECL, token_line);
        $$->spec = $1;
        for (size_t i = 0; i < $2->items.size(); i++) {
            $2->items[i]->spec = $1;
            $2->items[i]->is_typedef = in_typedef;
            $$->decls.push_back($2->items[i]);
        }
        in_typedef = 0; pending_params.clear(); }
    | typedef_kw declaration_specifiers init_declarator_list ';'
      { declare_all($2, $3, token_line);
        $$ = mk_node(N_DECL, token_line);
        $$->spec = $2;
        for (size_t i = 0; i < $3->items.size(); i++) {
            $3->items[i]->spec = $2;
            $3->items[i]->is_typedef = true;
            $$->decls.push_back($3->items[i]);
        }
        in_typedef = 0; pending_params.clear(); }
    | declaration_specifiers ';'
      { in_typedef = 0; pending_params.clear();
        
        if ($1 && !$1->members.empty()) {
            $$ = mk_node(N_DECL, token_line);
            $$->spec = $1;
        } else {
            $$ = NULL;
        } }
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
        $$->members = $5->items;
        add_symbol($2, "STRUCT", token_line);
        pop_struct(); free($2); }
    | STRUCT '{' { push_struct(NULL); } struct_declaration_list '}'
      { $$ = make_type("STRUCT <anonymous>");
        $$->members = $4->items;
        pop_struct(); }
    | STRUCT struct_tag
      { $$ = make_type(std::string("STRUCT ") + $2); free($2); }
    ;

struct_tag
    : IDENTIFIER  { $$ = $1; }
    ;

struct_declaration_list
    : struct_declaration                            { $$ = $1; }
    | struct_declaration_list struct_declaration    { $1->items.insert($1->items.end(),
                                                         $2->items.begin(), $2->items.end());
                                                      $$ = $1; }
    ;

struct_declaration
    : declaration_specifiers struct_declarator_list ';'
      { declare_members($1, $2, token_line);
        for (size_t i = 0; i < $2->items.size(); i++) {
            $2->items[i]->spec = $1;
            $2->items[i]->line = token_line;
        }
        $$ = $2; }
    | declaration_specifiers ';'
      { $$ = new DeclList(); }
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
    | declarator '=' initializer  { $1->init = $3; $$ = $1; }
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
    : IDENTIFIER                                       { $$ = make_decl($1);
                                                         $$->line = token_line;
                                                         free($1); }
    | '(' declarator ')'                               { $$ = $2; }
    | direct_declarator '[' assignment_expression ']'  { $1->dims.push_back(last_bracket_text());
                                                         $1->dim_exprs.push_back($3);
                                                         $$ = $1; }
    | direct_declarator '[' ']'                        { $1->dims.push_back("");
                                                         $1->dim_exprs.push_back(NULL);
                                                         $$ = $1; }
    | direct_declarator '(' parameter_type_list_opt ')' { $1->is_function = true;
                                                          if ($3) $1->params = $3->items;
                                                          $$ = $1; }
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
      { $2->spec = $1; $2->line = token_line;
        $$ = $2;
        pending_params.push_back({ $2->name, type_string($1, $2), 0, 0, token_line }); }
    | declaration_specifiers abstract_declarator  { $2->spec = $1; $$ = $2; }
    | declaration_specifiers                      { $$ = make_decl(""); $$->spec = $1; }
    ;


type_name
    : declaration_specifiers                      { $$ = $1; }
    | declaration_specifiers abstract_declarator  { $$ = $1;
                                                    $$->ptr_depth = $2->ptr_depth;
                                                    $$->dims      = $2->dims; }
    ;

abstract_declarator
    : pointer                        { $$ = make_decl(""); $$->ptr_depth = $1; }
    | '[' ']'                        { $$ = make_decl(""); $$->dims.push_back(""); }
    | '[' assignment_expression ']'  { $$ = make_decl(""); $$->dims.push_back("_"); }
    | pointer '[' ']'                { $$ = make_decl(""); $$->ptr_depth = $1; $$->dims.push_back(""); }
    ;

initializer
    : assignment_expression   { $$ = $1; }
    | '{' initializer_list '}'       { $$ = $2; }
    | '{' initializer_list ',' '}'   { $$ = $2; }
    ;

initializer_list
    : initializer                              { $$ = mk_node(N_INITLIST, token_line);
                                                 append($$, $1); }
    | initializer_list ',' initializer         { append($1, $3); $$ = $1; }
    ;



statement
    : labeled_statement     { $$ = $1; }
    | compound_statement    { $$ = $1; }
    | expression_statement  { $$ = $1; }
    | selection_statement   { $$ = $1; }
    | iteration_statement   { $$ = $1; }
    | jump_statement        { $$ = $1; }
    | error ';'   { yyerrok; $$ = NULL; }
    ;


labeled_statement
    : IDENTIFIER ':' statement   { $$ = mk_node(N_LABEL, token_line);
                                   $$->name = $1;
                                   append($$, $3);
                                   free($1); }
    ;


compound_statement
    : '{' { push_scope(); flush_pending_params(); } block_item_list_opt '}'
      { pop_scope();
        
        $$ = $3 ? $3 : mk_node(N_BLOCK, token_line); }
    ;


block_item_list_opt
    : /* empty */      { $$ = NULL; }
    | block_item_list  { $$ = $1; }
    ;


block_item_list
    : block_item                    { $$ = mk_node(N_BLOCK, token_line);
                                      append($$, $1); }
    | block_item_list block_item    { append($1, $2); $$ = $1; }
    ;

block_item
    : declaration    { $$ = $1; }
    | statement      { $$ = $1; }
    ;

expression_statement
    : ';'                 { $$ = NULL; }
    | expression ';'      { $$ = mk_node(N_EXPRSTMT, token_line);
                            append($$, $1); }
    ;

selection_statement
    : IF '(' expression ')' statement %prec IFX
      { $$ = mk_node(N_IF, token_line);
        append($$, $3);        // cond
        append($$, $5); }      // then
    | IF '(' expression ')' statement ELSE statement
      { $$ = mk_node(N_IF, token_line);
        append($$, $3);        // cond
        append($$, $5);        // then
        append($$, $7); }      // else
    ;

iteration_statement
    : WHILE '(' expression ')' statement
      { $$ = mk_node(N_WHILE, token_line);
        append($$, $3); append($$, $5); }
    | DO statement WHILE '(' expression ')' ';'
      { $$ = mk_node(N_DO, token_line);
        append($$, $2);        // body
        append($$, $5); }      // cond
    | UNTIL '(' expression ')' statement          // until loop syntax
      { $$ = mk_node(N_UNTIL, token_line);
        append($$, $3); append($$, $5); }
    | FOR '(' for_init expression_statement ')' statement
      { $$ = mk_node(N_FOR, token_line);
        append2($$, $3, token_line);   // init
        append2($$, $4, token_line);   // cond  (may be empty)
        append2($$, NULL, token_line); // step
        append($$, $6); }              // body
    | FOR '(' for_init expression_statement expression ')' statement
      { $$ = mk_node(N_FOR, token_line);
        append2($$, $3, token_line);   // init
        append2($$, $4, token_line);   // cond  (may be empty)
        append2($$, $5, token_line);   // step
        append($$, $7); }              // body
    ;

for_init
    : expression_statement  { $$ = $1; }
    | declaration           { $$ = $1; }
    ;

jump_statement
    : GOTO IDENTIFIER ';'  { $$ = mk_node(N_GOTO, token_line);
                             $$->name = $2;
                             free($2); }
    | CONTINUE ';'         { $$ = mk_node(N_CONTINUE, token_line); }
    | BREAK ';'            { $$ = mk_node(N_BREAK, token_line); }
    | RETURN ';'           { $$ = mk_node(N_RETURN, token_line); }
    | RETURN expression ';' { $$ = mk_node(N_RETURN, token_line);
                              append($$, $2); }
    ;



primary_expression
    : IDENTIFIER          { $$ = mk_id($1, token_line); free($1); }
    | INT_LIT             { $$ = mk_int($1, token_line); free($1); }
    | FLOAT_LIT           { $$ = mk_float($1, token_line); free($1); }
    | CHAR_LIT            { $$ = mk_char($1, token_line); free($1); }
    | string_literal      { $$ = $1; }
    | '(' expression ')'  { $$ = $2; }
    ;


string_literal
    : STRING_LIT                { $$ = mk_str($1, token_line); free($1); }
    | string_literal STRING_LIT { $1->value += $2; $$ = $1; free($2); }
    ;

postfix_expression
    : primary_expression                        { $$ = $1; }
    | postfix_expression '[' expression ']'     { $$ = mk_node(N_INDEX, token_line);
                                                  append($$, $1); append($$, $3); }
    | postfix_expression '(' ')'                { $$ = mk_node(N_CALL, token_line);
                                                  append($$, $1); }
    | postfix_expression '(' argument_expression_list ')'
                                                { $$ = mk_node(N_CALL, token_line);
                                                  append($$, $1);   // callee
                                                  append($$, $3); } // N_LIST of args
    | postfix_expression '.' IDENTIFIER         { $$ = mk_node(N_MEMBER, token_line);
                                                  $$->name = $3;
                                                  $$->op = ".";
                                                  append($$, $1); free($3); }
    | postfix_expression ARROW IDENTIFIER       { $$ = mk_node(N_MEMBER, token_line);
                                                  $$->name = $3;
                                                  $$->op = "->";
                                                  append($$, $1); free($3); }
    | postfix_expression INC                    { $$ = mk_node(N_UN, token_line);
                                                  $$->op = "++";
                                                  append($$, $1); }
    | postfix_expression DEC                    { $$ = mk_node(N_UN, token_line);
                                                  $$->op = "--";
                                                  append($$, $1); }
    ;


argument_expression_list
    : assignment_expression                      { $$ = mk_list(token_line);
                                                   append($$, $1); }
    | argument_expression_list ',' assignment_expression
                                                 { append($1, $3); $$ = $1; }
    ;


unary_expression
    : postfix_expression                   { $$ = $1; }
    | INC unary_expression                 { $$ = mk_node(N_UN, token_line);
                                             $$->op = "++";
                                             append($$, $2); }
    | DEC unary_expression                 { $$ = mk_node(N_UN, token_line);
                                             $$->op = "--";
                                             append($$, $2); }
    | unary_operator cast_expression %prec UNARY_PREC
                                           { $$ = mk_un((char)$1, $2, token_line); }
    | SIZEOF unary_expression              { $$ = mk_node(N_SIZEOF, token_line);
                                             append($$, $2); }
    | SIZEOF '(' type_name ')'             { $$ = mk_node(N_SIZEOF, token_line);
                                             $$->spec = $3; }
    ;

unary_operator
    : '&'   { $$ = '&'; }
    | '*'   { $$ = '*'; }
    | '+'   { $$ = '+'; }
    | '-'   { $$ = '-'; }
    | '~'   { $$ = '~'; }
    | '!'   { $$ = '!'; }
    ;

cast_expression
    : unary_expression                        { $$ = $1; }
    | '(' type_name ')' cast_expression       { $$ = mk_node(N_CAST, token_line);
                                                $$->spec = $2;
                                                append($$, $4); }
    ;


// each operator is its own production
assignment_expression
    : cast_expression                              { $$ = $1; }
    | assignment_expression '*' assignment_expression   { $$ = BIN($1, $3, "*"); }
    | assignment_expression '/' assignment_expression   { $$ = BIN($1, $3, "/"); }
    | assignment_expression '%' assignment_expression   { $$ = BIN($1, $3, "%"); }
    | assignment_expression '+' assignment_expression   { $$ = BIN($1, $3, "+"); }
    | assignment_expression '-' assignment_expression   { $$ = BIN($1, $3, "-"); }
    | assignment_expression LEFT_OP assignment_expression  { $$ = BIN($1, $3, "<<"); }
    | assignment_expression RIGHT_OP assignment_expression { $$ = BIN($1, $3, ">>"); }
    | assignment_expression '<' assignment_expression   { $$ = BIN($1, $3, "<"); }
    | assignment_expression '>' assignment_expression   { $$ = BIN($1, $3, ">"); }
    | assignment_expression LE assignment_expression    { $$ = BIN($1, $3, "<="); }
    | assignment_expression GE assignment_expression    { $$ = BIN($1, $3, ">="); }
    | assignment_expression EQ assignment_expression    { $$ = BIN($1, $3, "=="); }
    | assignment_expression NE assignment_expression    { $$ = BIN($1, $3, "!="); }
    | assignment_expression '&' assignment_expression   { $$ = BIN($1, $3, "&"); }
    | assignment_expression '^' assignment_expression   { $$ = BIN($1, $3, "^"); }
    | assignment_expression '|' assignment_expression   { $$ = BIN($1, $3, "|"); }
    | assignment_expression AND_OP assignment_expression { $$ = BIN($1, $3, "&&"); }
    | assignment_expression OR_OP  assignment_expression { $$ = BIN($1, $3, "||"); }
    | assignment_expression '?' assignment_expression ':' assignment_expression
                                     { $$ = mk_node(N_TERNARY, token_line);
                                       append($$, $1); append($$, $3); append($$, $5); }
    | assignment_expression '=' assignment_expression        { $$ = mk_assign($1, $3, token_line); }
    | assignment_expression ADD_ASSIGN assignment_expression  { $$ = mk_assign($1, $3, token_line); $$->op = "+="; }
    | assignment_expression SUB_ASSIGN assignment_expression  { $$ = mk_assign($1, $3, token_line); $$->op = "-="; }
    | assignment_expression MUL_ASSIGN assignment_expression  { $$ = mk_assign($1, $3, token_line); $$->op = "*="; }
    | assignment_expression DIV_ASSIGN assignment_expression  { $$ = mk_assign($1, $3, token_line); $$->op = "/="; }
    | assignment_expression MOD_ASSIGN assignment_expression  { $$ = mk_assign($1, $3, token_line); $$->op = "%="; }
    | assignment_expression LEFT_ASSIGN assignment_expression { $$ = mk_assign($1, $3, token_line); $$->op = "<<="; }
    | assignment_expression RIGHT_ASSIGN assignment_expression { $$ = mk_assign($1, $3, token_line); $$->op = ">>="; }
    | assignment_expression AND_ASSIGN assignment_expression  { $$ = mk_assign($1, $3, token_line); $$->op = "&="; }
    | assignment_expression XOR_ASSIGN assignment_expression  { $$ = mk_assign($1, $3, token_line); $$->op = "^="; }
    | assignment_expression OR_ASSIGN assignment_expression   { $$ = mk_assign($1, $3, token_line); $$->op = "|="; }
    ;


expression
    : assignment_expression                    { $$ = $1; }
    | expression ',' assignment_expression     { $$ = BIN($1, $3, ","); }
    ;

%%


void yyerror(const char *msg) {
    syntax_error_count++;
    fprintf(stderr, "Syntax error at line %d, column %d: unexpected '%s' -- %s\n",
            token_line, token_column,
            (yytext && *yytext) ? yytext : "<end of file>", msg);
}
