#!/bin/bash
# Build, then run the tests.
#
#   ./run.sh                 all of tests/parser_tests
#   ./run.sh test5           just that one test, full output
#   ./run.sh err3            works for invalid tests too
#   ./run.sh path/to/x.c     or give a path directly
#   ./run.sh --verbose       whole sweep, showing each parser's full output
#   ./run.sh --lexer         all of tests/lexer_tests
#
# valid/   must parse cleanly   -> exit 0
# invalid/ must report an error -> exit non-zero
#
# Both directions are checked, since an invalid test that parses is just as broken as a valid one that doesn't.
# Exits non-zero if anything is off.

VALID_DIR=tests/parser_tests/valid
INVALID_DIR=tests/parser_tests/invalid

VERBOSE=0
MODE=parser
ONLY=

usage() {
    echo "usage: ./run.sh [--lexer] [--verbose] [test-name | path/to/test.c]"
}

for arg in "$@"; do
    case "$arg" in
        --lexer)   MODE=lexer ;;
        --verbose) VERBOSE=1 ;;
        -h|--help) usage; exit 0 ;;
        -*)
            echo "unknown option: $arg" >&2
            usage >&2
            exit 2 ;;
        *)
            ONLY="$arg" ;;
    esac
done


echo "Building..."
if ! make clean >/dev/null 2>&1 || ! make; then
    echo "Build failed." >&2
    exit 1
fi
echo "Build successful."
echo

PASS=0
FAIL=0


if [ "$MODE" = lexer ]; then

    if [ ! -x ./src/lex ] && [ ! -x ./src/lex.exe ]; then
        echo "Error: lexer executable not found in ./src/" >&2
        exit 1
    fi

    for testfile in tests/lexer_tests/*.txt; do
        [ -e "$testfile" ] || continue
        echo "=============================================================="
        echo "Lexing: $(basename "$testfile")"
        echo "--------------------------------------------------------------"
        ./src/lex < "$testfile"
        echo
    done

    echo "=============================================================="
    echo "Lex sweep complete."
    exit 0
fi


if [ ! -x ./src/parser ] && [ ! -x ./src/parser.exe ]; then
    echo "Error: parser executable not found in ./src/" >&2
    exit 1
fi


run_one() {
    local testfile="$1" expect="$2"
    local base out status

    base=$(basename "$testfile")
    out=$(./src/parser "$testfile" 2>&1)
    status=$?

    if { [ "$expect" = clean ] && [ "$status" -eq 0 ]; } ||
       { [ "$expect" = error ] && [ "$status" -ne 0 ]; }; then
        PASS=$((PASS + 1))
        if [ "$expect" = clean ]; then
            printf 'PASS  %-12s parsed cleanly\n' "$base"
        else
            printf 'PASS  %-12s rejected, %s syntax error(s)\n' "$base" \
                "$(printf '%s\n' "$out" | grep -c '^Syntax error')"
        fi
    else
        FAIL=$((FAIL + 1))
        if [ "$expect" = clean ]; then
            printf 'FAIL  %-12s expected a clean parse, got exit %d\n' "$base" "$status"
        else
            printf 'FAIL  %-12s expected a syntax error, parsed cleanly\n' "$base"
        fi
        
        printf '%s\n' "$out" | sed 's/^/        | /'
        return
    fi

    if [ "$VERBOSE" -eq 1 ]; then
        printf '%s\n' "$out" | sed 's/^/        | /'
        echo
    fi
}

run_group() {
    local testfile
    for testfile in "$1"/*.c; do
        [ -e "$testfile" ] || continue
        run_one "$testfile" "$2"
    done
}


if [ -n "$ONLY" ]; then

    if [ -f "$ONLY" ]; then
        target="$ONLY"
    elif [ -f "$VALID_DIR/${ONLY%.c}.c" ]; then
        target="$VALID_DIR/${ONLY%.c}.c"
    elif [ -f "$INVALID_DIR/${ONLY%.c}.c" ]; then
        target="$INVALID_DIR/${ONLY%.c}.c"
    else
        echo "No such test: $ONLY" >&2
        echo "available:" >&2
        ls "$VALID_DIR" "$INVALID_DIR" 2>/dev/null | sed 's/^/  /' >&2
        exit 2
    fi

    case "$target" in
        *invalid*) expect=error ;;
        *)         expect=clean ;;
    esac

    echo "=============================================================="
    echo "$target  (expecting a $([ "$expect" = clean ] && echo "clean parse" || echo "syntax error"))"
    echo "=============================================================="
    VERBOSE=1                     
    run_one "$target" "$expect"

    [ "$FAIL" -eq 0 ] || exit 1
    exit 0
fi


echo "=============================================================="
echo "Valid programs -- must parse cleanly"
echo "=============================================================="
run_group "$VALID_DIR" clean

echo
echo "=============================================================="
echo "Invalid programs -- must be rejected"
echo "=============================================================="
run_group "$INVALID_DIR" error

TOTAL=$((PASS + FAIL))
echo
echo "=============================================================="
echo "Summary: $PASS/$TOTAL as expected"
echo "=============================================================="

if [ "$FAIL" -ne 0 ]; then
    echo "$FAIL test(s) did not behave as expected." >&2
    exit 1
fi

echo
echo "./run.sh test5      to run one test and see its full output"
echo "./run.sh --verbose  to see full output for everything"
echo "./run.sh --lexer    for the phase 1 lexeme tables"
