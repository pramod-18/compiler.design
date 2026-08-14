%{

#include<stdio.h>
#include<string.h>
#include<stdbool.h>

int lexical_error_count = 0;
int line_number = 1;
int column_number = 1;
int comment_start_line = 0;
int comment_start_column = 0;

int tab_width = 4;  // this could vary

void handle_multiline_comment();
void handle_token();

%}

%option noyywrap

L           [a-zA-Z_]
D           [0-9]
S           [+-]
IS          ([uU]([lL]{1,2})?|[lL]{1,2}([uU])?)
FS          [fFlL]
EXP         [eE]{S}?{D}+


KEYWORD     auto|break|case|char|const|continue|default|do|double|else|enum|extern|float|for|goto|if|inline|int|long|register|restrict|return|short|signed|sizeof|static|struct|switch|typedef|union|unsigned|void|volatile|while
IDENTIFIER  {L}({L}|{D})*
INTEGER     {S}?{D}+{IS}?
FLOAT       {S}?(({D}*"."{D}+)|({D}+"."{D}*)){EXP}?{FS}?
CHARACTER   \'([^\\']|\\.)\'
STRING      \"([^\\"]|\\.)*\"
OPERATOR    (\+\+|\-\-|\+=|\-=|\*=|\/=|%=|<<=|>>=|&=|\^=|\|=|==|!=|<=|>=|<<|>>|&&|\|\||\+|\-|\*|\/|%|<|>|=|&|\||\^|~|!|->)
DELIMITER   ([;,\(\)\{\}\[\]:\?\.]|\.\.\.)
WHITESPACE  [ \t]+
NEWLINE     [\n]

%%

"/*"                { comment_start_line = line_number; comment_start_column = column_number; column_number += 2; handle_multiline_comment(); }
"//".*[\n]          { line_number++; column_number = 1; }           /*single line comment*/

{KEYWORD}           { handle_token(); printf("%-15s keyword\n", yytext); }
{IDENTIFIER}        { handle_token(); printf("%-15s identifier\n", yytext); }

{INTEGER}           { handle_token(); printf("%-15s integer\n", yytext); }
{FLOAT}             { handle_token(); printf("%-15s float\n", yytext); }

{CHARACTER}         { handle_token(); printf("%-15s character\n", yytext); }
{STRING}            { handle_token(); printf("%-15s string\n", yytext); }

{OPERATOR}          { handle_token(); printf("%-15s operator\n", yytext); }
{DELIMITER}         { handle_token(); printf("%-15s delimeter\n", yytext); }
{WHITESPACE}        { handle_token(); }
{NEWLINE}           { handle_token(); }

.                   { printf("Error: Invalid token '%s' on line %d, column %d \n", yytext, line_number, column_number); lexical_error_count++; handle_token(); }

%%


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

int main(){

    printf("\n%-16s%s\n", "Lexeme", "Token");
    printf("%-16s%s\n", "------", "-----");

    yylex();

    if(lexical_error_count > 0){
        fprintf(stderr, "\nTotal errors: %d\n", lexical_error_count);
    }

    return 0;
}
