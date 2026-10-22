WINGET_PKGS := $(LOCALAPPDATA)/Microsoft/WinGet/Packages

BISON ?= $(firstword \
	$(shell command -v win_bison >/dev/null 2>&1 && echo win_bison) \
	$(wildcard $(WINGET_PKGS)/WinFlexBison*/win_bison.exe) \
	bison)

FLEX ?= $(firstword \
	$(shell command -v win_flex >/dev/null 2>&1 && echo win_flex) \
	$(wildcard $(WINGET_PKGS)/WinFlexBison*/win_flex.exe) \
	flex)

CXX      := g++
CXXFLAGS := -std=c++17 -Wno-register

SRC := src
EXE := $(if $(findstring Windows,$(OS)),.exe,)

LEXER  := $(SRC)/lex$(EXE)
PARSER := $(SRC)/parser$(EXE)
IRGEN  := $(SRC)/irgen$(EXE)

.PHONY: all lexer parser irgen test tools check-grammar clean help

all: lexer parser irgen

lexer: $(LEXER)
parser: $(PARSER)
irgen: $(IRGEN)


$(SRC)/parser.tab.c $(SRC)/parser.tab.h: $(SRC)/parser.y
	$(BISON) -d -o $(SRC)/parser.tab.c $(SRC)/parser.y

$(SRC)/lex.yy.c: $(SRC)/lexer.l $(SRC)/parser.tab.h
	$(FLEX) -o $(SRC)/lex.yy.c $(SRC)/lexer.l

$(LEXER): $(SRC)/lex.yy.c $(SRC)/lex_driver.cpp
	$(CXX) $(CXXFLAGS) -I$(SRC) $^ -o $@

$(PARSER): $(SRC)/lex.yy.c $(SRC)/parser.tab.c $(SRC)/ast.cpp $(SRC)/parser_driver.cpp
	$(CXX) $(CXXFLAGS) -I$(SRC) $^ -o $@

$(IRGEN): $(SRC)/lex.yy.c $(SRC)/parser.tab.c $(SRC)/ast.cpp \
          $(SRC)/semantic.cpp $(SRC)/irgen.cpp $(SRC)/ir_driver.cpp
	$(CXX) $(CXXFLAGS) -I$(SRC) $^ -o $@

test: all
	./run.sh

ir: all
	./run.sh --ir


tools:
	@echo "BISON = $(BISON)"
	@echo "FLEX  = $(FLEX)"
	@echo "CXX   = $(CXX)"
	@$(BISON) --version 2>/dev/null | head -1 || echo "  !! $(BISON) doesn't run -- install bison"
	@$(FLEX)  --version 2>/dev/null | head -1 || echo "  !! $(FLEX) doesn't run -- install flex"
	@$(CXX)   --version 2>/dev/null | head -1 || echo "  !! $(CXX) doesn't run -- install g++"


check-grammar: $(SRC)/parser.y
	$(BISON) -d -Wcounterexamples -v -o $(SRC)/parser.tab.c $(SRC)/parser.y

clean:
	rm -f $(SRC)/lex $(SRC)/lex.exe $(SRC)/parser $(SRC)/parser.exe
	rm -f $(SRC)/irgen $(SRC)/irgen.exe
	rm -f $(SRC)/lex.yy.c $(SRC)/parser.tab.c $(SRC)/parser.tab.h $(SRC)/parser.output
	rm -f tests/ir_tests/out/*.ir

help:
	@echo "make          build all three executables"
	@echo "make lexer    lexical analyzer    -> $(LEXER)"
	@echo "make parser   syntax analyzer     -> $(PARSER)"
	@echo "make irgen    IR generator        -> $(IRGEN)"
	@echo "make test     build, then run ./run.sh"
	@echo "make ir       build, then run ./run.sh --ir"
	@echo "make tools    show/check the detected flex, bison and g++"
	@echo "make clean    remove generated files"
	@echo ""
	@echo "BISON = $(BISON)"
	@echo "FLEX  = $(FLEX)"
