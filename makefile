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
TARGETGEN := $(SRC)/targetgen$(EXE)

.PHONY: all lexer parser irgen targetgen test ir target target-check tools check-grammar clean help

all: lexer parser irgen targetgen

lexer: $(LEXER)
parser: $(PARSER)
irgen: $(IRGEN)
targetgen: $(TARGETGEN)


$(SRC)/parser.tab.c: $(SRC)/parser.y
	$(BISON) -d -o $(SRC)/parser.tab.c $(SRC)/parser.y

$(SRC)/parser.tab.h: $(SRC)/parser.tab.c
	@test -f $@ || $(BISON) -d -o $(SRC)/parser.tab.c $(SRC)/parser.y

$(SRC)/lex.yy.c: $(SRC)/lexer.l $(SRC)/parser.tab.h
	$(FLEX) -o $(SRC)/lex.yy.c $(SRC)/lexer.l

$(LEXER): $(SRC)/lex.yy.c $(SRC)/lex_driver.cpp $(SRC)/ast.h $(SRC)/parser.tab.h
	$(CXX) $(CXXFLAGS) -I$(SRC) $(filter %.c %.cpp,$^) -o $@

$(PARSER): $(SRC)/lex.yy.c $(SRC)/parser.tab.c $(SRC)/ast.cpp $(SRC)/parser_driver.cpp $(SRC)/ast.h
	$(CXX) $(CXXFLAGS) -I$(SRC) $(filter %.c %.cpp,$^) -o $@

$(IRGEN): $(SRC)/lex.yy.c $(SRC)/parser.tab.c $(SRC)/ast.cpp \
          $(SRC)/semantic.cpp $(SRC)/irgen.cpp $(SRC)/ir_driver.cpp $(SRC)/ast.h $(SRC)/semantic.h $(SRC)/irgen.h
	$(CXX) $(CXXFLAGS) -I$(SRC) $(filter %.c %.cpp,$^) -o $@

$(TARGETGEN): $(SRC)/lex.yy.c $(SRC)/parser.tab.c $(SRC)/ast.cpp \
              $(SRC)/semantic.cpp $(SRC)/targetgen.cpp $(SRC)/target_runtime.cpp $(SRC)/target_driver.cpp \
              $(SRC)/ast.h $(SRC)/semantic.h $(SRC)/targetgen.h $(SRC)/target_runtime.h
	$(CXX) $(CXXFLAGS) -I$(SRC) $(filter %.c %.cpp,$^) -o $@

test: all
	bash ./run.sh

ir: all
	bash ./run.sh --ir

target: all
	bash ./run.sh --target

target-check: all
	bash ./run.sh --target --execute


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
	rm -f $(SRC)/targetgen $(SRC)/targetgen.exe
	rm -f $(SRC)/lex.yy.c $(SRC)/parser.tab.c $(SRC)/parser.tab.h $(SRC)/parser.output
	rm -f tests/ir_tests/out/*.ir
	rm -f tests/target_tests/out/*.asm tests/target_tests/out/*.output tests/target_tests/out/*.actual tests/target_tests/out/*.log tests/target_tests/out/*.txt

help:
	@echo "make          build all four executables"
	@echo "make lexer    lexical analyzer    -> $(LEXER)"
	@echo "make parser   syntax analyzer     -> $(PARSER)"
	@echo "make irgen    IR generator        -> $(IRGEN)"
	@echo "make targetgen MIPS generator     -> $(TARGETGEN)"
	@echo "make test     build, then run ./run.sh"
	@echo "make ir       build, then run ./run.sh --ir"
	@echo "make target   generate tests/target_tests/out/*.asm"
	@echo "make target-check execute target tests with SPIM and check for execution errors"
	@echo "make tools    show/check the detected flex, bison and g++"
	@echo "make clean    remove generated files"
	@echo ""
	@echo "BISON = $(BISON)"
	@echo "FLEX  = $(FLEX)"
