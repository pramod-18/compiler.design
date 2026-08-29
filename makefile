# Build recipes.
#
#   make            build both executables
#   make lexer      src/lex     (lexeme/class table)
#   make parser     src/parser  (syntax analyzer)
#   make test       build, then run ./run.sh
#   make tools      show which flex/bison this makefile picked
#   make clean      remove generated files
#
# Needs g++, flex and bison. If you don't have flex/bison:
#   linux    sudo apt install flex bison
#   mac      brew install flex bison
#   windows  winget install WinFlexBison.win_flex_bison

# Where winget puts WinFlexBison. Empty/unmatched everywhere else, which is the
# point -- $(wildcard) on a path that doesn't exist just vanishes.

WINGET_PKGS := $(LOCALAPPDATA)/Microsoft/WinGet/Packages

# Tries in order: win_bison on PATH, the winget dir, then plain bison. Names
# found on PATH stay bare so the OS resolves them -- a path out of 'command -v'
# would be bash-style, which CreateProcess can't exec.
#
# The last entry is unconditional, so this is never empty. On linux/mac the
# first two can't match and you get plain 'bison'. If it isn't installed you
# get a normal "bison: command not found" instead of make guessing. Override
# any time:  make BISON=/usr/bin/bison FLEX=/usr/bin/flex

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

.PHONY: all lexer parser test tools check-grammar clean help

all: lexer parser

lexer: $(LEXER)
parser: $(PARSER)


$(SRC)/parser.tab.c $(SRC)/parser.tab.h: $(SRC)/parser.y
	$(BISON) -d -o $(SRC)/parser.tab.c $(SRC)/parser.y

$(SRC)/lex.yy.c: $(SRC)/lexer.l $(SRC)/parser.tab.h
	$(FLEX) -o $(SRC)/lex.yy.c $(SRC)/lexer.l

$(LEXER): $(SRC)/lex.yy.c $(SRC)/lex_driver.cpp
	$(CXX) $(CXXFLAGS) -I$(SRC) $^ -o $@

$(PARSER): $(SRC)/lex.yy.c $(SRC)/parser.tab.c
	$(CXX) $(CXXFLAGS) -I$(SRC) $^ -o $@

test: all
	./run.sh


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
	rm -f $(SRC)/lex.yy.c $(SRC)/parser.tab.c $(SRC)/parser.tab.h $(SRC)/parser.output

help:
	@echo "make          build both executables"
	@echo "make lexer    lexical analyzer -> $(LEXER)"
	@echo "make parser   syntax analyzer  -> $(PARSER)"
	@echo "make test     build, then run ./run.sh"
	@echo "make tools    show/check the detected flex, bison and g++"
	@echo "make clean    remove generated files"
	@echo ""
	@echo "BISON = $(BISON)"
	@echo "FLEX  = $(FLEX)"
