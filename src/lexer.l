%{

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "parser.tab.h"

int lexical_error_count = 0;
int line_number = 1;
int column_number = 1;
int comment_start_line = 0;
int comment_start_column = 0;


int token_line = 1;
int token_column = 1;

int tab_width = 4;  // this could vary

void handle_multiline_comment();
void handle_token();

// from parser.y
extern "C" int  is_typedef_name(const char *name);
extern "C" void record_token(const char *lexeme, const char *type);

static void begin_token(){
    token_line = line_number;
    token_column = column_number;
}

// common procedure for every token
#define TOK(code, name)                     \
    do {                                    \
        begin_token();                      \
        record_token(yytext, (name));       \
        handle_token();                     \
        return (code);                      \
    } while (0)

%}

%option noyywrap

L           [a-zA-Z_]
D           [0-9]
IS          ([uU]([lL]{1,2})?|[lL]{1,2}([uU])?)
FS          [fFlL]
EXP         [eE][+-]?{D}+


IDENTIFIER  {L}({L}|{D})*
INTEGER     {D}+{IS}?
FLOAT       (({D}*"."{D}+)|({D}+"."{D}*)){EXP}?{FS}?
CHARACTER   \'([^\\']|\\.)\'
STRING      \"([^\\"]|\\.)*\"
WHITESPACE  [ \t]+
NEWLINE     [\n]

%%

"/*"                { comment_start_line = line_number; comment_start_column = column_number; column_number += 2; handle_multiline_comment(); }
"//".*              { handle_token(); }                     // single line comment

^[ \t]*#.*          { handle_token(); }                     // ignore preprocessor line


"auto"              { TOK(AUTO, "AUTO"); }
"break"             { TOK(BREAK, "BREAK"); }
"case"              { TOK(CASE, "CASE"); }
"char"              { TOK(CHAR, "CHAR"); }
"const"             { TOK(CONST, "CONST"); }
"continue"          { TOK(CONTINUE, "CONTINUE"); }
"default"           { TOK(DEFAULT, "DEFAULT"); }
"do"                { TOK(DO, "DO"); }
"double"            { TOK(DOUBLE, "DOUBLE"); }
"else"              { TOK(ELSE, "ELSE"); }
"enum"              { TOK(ENUM, "ENUM"); }
"extern"            { TOK(EXTERN, "EXTERN"); }
"float"             { TOK(FLOAT, "FLOAT"); }
"for"               { TOK(FOR, "FOR"); }
"goto"              { TOK(GOTO, "GOTO"); }
"if"                { TOK(IF, "IF"); }
"inline"            { TOK(INLINE, "INLINE"); }
"int"               { TOK(INT, "INT"); }
"long"              { TOK(LONG, "LONG"); }
"register"          { TOK(REGISTER, "REGISTER"); }
"restrict"          { TOK(RESTRICT, "RESTRICT"); }
"return"            { TOK(RETURN, "RETURN"); }
"short"             { TOK(SHORT, "SHORT"); }
"signed"            { TOK(SIGNED, "SIGNED"); }
"sizeof"            { TOK(SIZEOF, "SIZEOF"); }
"static"            { TOK(STATIC, "STATIC"); }
"struct"            { TOK(STRUCT, "STRUCT"); }
"switch"            { TOK(SWITCH, "SWITCH"); }
"typedef"           { TOK(TYPEDEF, "TYPEDEF"); }
"union"             { TOK(UNION, "UNION"); }
"unsigned"          { TOK(UNSIGNED, "UNSIGNED"); }
"until"             { TOK(UNTIL, "UNTIL"); }
"void"              { TOK(VOID, "VOID"); }
"volatile"          { TOK(VOLATILE, "VOLATILE"); }
"while"             { TOK(WHILE, "WHILE"); }


{IDENTIFIER}        { begin_token();
                      if(is_typedef_name(yytext)){
                          record_token(yytext, "TYPE_NAME");
                          yylval.str = strdup(yytext);
                          handle_token();
                          return TYPE_NAME;
                      }
                      record_token(yytext, "IDENTIFIER");
                      yylval.str = strdup(yytext);
                      handle_token();
                      return IDENTIFIER; }

{INTEGER}           { yylval.str = strdup(yytext); TOK(INT_LIT, "INT_LIT"); }
{FLOAT}             { yylval.str = strdup(yytext); TOK(FLOAT_LIT, "FLOAT_LIT"); }

{CHARACTER}         { yylval.str = strdup(yytext); TOK(CHAR_LIT, "CHAR_LIT"); }
{STRING}            { yylval.str = strdup(yytext); TOK(STRING_LIT, "STRING_LIT"); }


"..."               { TOK(ELLIPSIS, "ELLIPSIS"); }
"<<="               { TOK(LEFT_ASSIGN, "LEFT_ASSIGN"); }
">>="               { TOK(RIGHT_ASSIGN, "RIGHT_ASSIGN"); }
"++"                { TOK(INC, "INC"); }
"--"                { TOK(DEC, "DEC"); }
"->"                { TOK(ARROW, "ARROW"); }
"+="                { TOK(ADD_ASSIGN, "ADD_ASSIGN"); }
"-="                { TOK(SUB_ASSIGN, "SUB_ASSIGN"); }
"*="                { TOK(MUL_ASSIGN, "MUL_ASSIGN"); }
"/="                { TOK(DIV_ASSIGN, "DIV_ASSIGN"); }
"%="                { TOK(MOD_ASSIGN, "MOD_ASSIGN"); }
"&="                { TOK(AND_ASSIGN, "AND_ASSIGN"); }
"^="                { TOK(XOR_ASSIGN, "XOR_ASSIGN"); }
"|="                { TOK(OR_ASSIGN, "OR_ASSIGN"); }
"=="                { TOK(EQ, "EQ"); }
"!="                { TOK(NE, "NE"); }
"<="                { TOK(LE, "LE"); }
">="                { TOK(GE, "GE"); }
"<<"                { TOK(LEFT_OP, "LEFT_OP"); }
">>"                { TOK(RIGHT_OP, "RIGHT_OP"); }
"&&"                { TOK(AND_OP, "AND_OP"); }
"||"                { TOK(OR_OP, "OR_OP"); }


"+"                 { TOK('+', "PLUS"); }
"-"                 { TOK('-', "MINUS"); }
"*"                 { TOK('*', "STAR"); }
"/"                 { TOK('/', "SLASH"); }
"%"                 { TOK('%', "PERCENT"); }
"<"                 { TOK('<', "LT"); }
">"                 { TOK('>', "GT"); }
"="                 { TOK('=', "ASSIGN"); }
"&"                 { TOK('&', "AMPERSAND"); }
"|"                 { TOK('|', "PIPE"); }
"^"                 { TOK('^', "CARET"); }
"~"                 { TOK('~', "TILDE"); }
"!"                 { TOK('!', "BANG"); }


";"                 { TOK(';', "SEMICOLON"); }
","                 { TOK(',', "COMMA"); }
"("                 { TOK('(', "LPAREN"); }
")"                 { TOK(')', "RPAREN"); }
"{"                 { TOK('{', "LBRACE"); }
"}"                 { TOK('}', "RBRACE"); }
"["                 { TOK('[', "LBRACKET"); }
"]"                 { TOK(']', "RBRACKET"); }
":"                 { TOK(':', "COLON"); }
"?"                 { TOK('?', "QUESTION"); }
"."                 { TOK('.', "DOT"); }

{WHITESPACE}        { handle_token(); }
{NEWLINE}           { handle_token(); }

.                   { printf("Error: Invalid token '%s' on line %d, column %d \n", yytext, line_number, column_number); lexical_error_count++; handle_token(); }

%%



const char *token_class(int tok){

    switch(tok){

        case IDENTIFIER: case TYPE_NAME:
            return "identifier";

        case INT_LIT:    return "integer";
        case FLOAT_LIT:  return "float";
        case CHAR_LIT:   return "character";
        case STRING_LIT: return "string";

        case AUTO: case BREAK: case CASE: case CHAR: case CONST: case CONTINUE:
        case DEFAULT: case DO: case DOUBLE: case ELSE: case ENUM: case EXTERN:
        case FLOAT: case FOR: case GOTO: case IF: case INLINE: case INT:
        case LONG: case REGISTER: case RESTRICT: case RETURN: case SHORT:
        case SIGNED: case SIZEOF: case STATIC: case STRUCT: case SWITCH:
        case TYPEDEF: case UNION: case UNSIGNED: case UNTIL: case VOID:
        case VOLATILE: case WHILE:
            return "keyword";

        case INC: case DEC: case ARROW:
        case ADD_ASSIGN: case SUB_ASSIGN: case MUL_ASSIGN: case DIV_ASSIGN:
        case MOD_ASSIGN: case LEFT_ASSIGN: case RIGHT_ASSIGN: case AND_ASSIGN:
        case XOR_ASSIGN: case OR_ASSIGN:
        case EQ: case NE: case LE: case GE: case LEFT_OP: case RIGHT_OP:
        case AND_OP: case OR_OP:
        case '+': case '-': case '*': case '/': case '%':
        case '<': case '>': case '=': case '&': case '|': case '^':
        case '~': case '!':
            return "operator";

        case ELLIPSIS:
        case ';': case ',': case '(': case ')': case '{': case '}':
        case '[': case ']': case ':': case '?': case '.':
            return "delimeter";

        default:
            return "unknown";
    }

}

void handle_token(){

    for(int i = 0; yytext[i] != '\0'; i++){

        // update line_no and col_no
        if(yytext[i] == '\n'){
            line_number++;
            column_number = 1;
        }
        else if(yytext[i] == '\t'){
            column_number += tab_width;
        }
        else column_number++;
    }

    return;

}

void handle_multiline_comment(){

    int curr = 0, prev;

    while(true){
        prev = curr;
        curr = yyinput();

        // if we have reached end-of-file and the comment hasnt been terminated yet.
        if(curr == EOF || curr == 0){
            printf("Error: Unterminated comment on line %d, column %d \n", comment_start_line, comment_start_column);
            lexical_error_count++;
            return;
        }

        // check for end-of-comment
        if(curr == '/' && prev == '*'){
            column_number++;
            comment_start_line = 0;
            comment_start_column = 0;
            return;
        }

        // update line_no and col_no
        if(curr == '\n'){
            line_number++;
            column_number = 1;
        }
        else if(curr == '\t'){
            column_number += tab_width;
        }
        else{
            column_number++;
        }
    }

    return;

}
