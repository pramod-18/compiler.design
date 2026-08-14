@echo off
make

for %%f in (tests\lexer_tests\*.txt) do (
    echo.
    echo Running test case: %%f
    src\lex < %%f
    echo ---------------------------------
)
pause