LEXER = src/lex
SRC = src/lexer.l

all: $(LEXER)

$(LEXER): $(SRC)
	cd src && flex lexer.l
	cd src && g++ lex.yy.c -o lex -std=c++17 -Wno-register

clean:
	cd src && rm -f lex lex.yy.c