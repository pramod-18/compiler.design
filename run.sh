#!/usr/bin/env bash

VALID_DIR=tests/parser_tests/valid
INVALID_DIR=tests/parser_tests/invalid
IR_DIR=tests/ir_tests
IR_OUT=$IR_DIR/out
TARGET_DIR=tests/target_tests
TARGET_OUT=$TARGET_DIR/out
TARGET_INPUT=$TARGET_DIR/input

VERBOSE=0
MODE=parser
ONLY=
EXECUTE=0
COMPILER=
SPIM=${SPIM:-spim}

usage() {
    echo "usage: ./run.sh [--lexer | --ir | --target] [--execute] [--verbose] [--exe executable] [test-name | path/to/test.c]"
    echo "  --execute  run target assembly with SPIM and report execution errors"
}

while [ "$#" -gt 0 ]; do
    arg="$1"
    shift
    case "$arg" in
        --lexer)   MODE=lexer ;;
        --ir)      MODE=ir ;;
        --target|--mips) MODE=target ;;
        --execute) EXECUTE=1 ;;
        --exe)
            [ "$#" -gt 0 ] || { usage >&2; exit 2; }
            COMPILER="$1"; shift ;;
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

if [ "$EXECUTE" -eq 1 ]; then
    [ "$MODE" = target ] || { echo "--execute requires --target" >&2; exit 2; }
    command -v "$SPIM" >/dev/null 2>&1 || { echo "SPIM not found. Install spim, or set SPIM to its executable path." >&2; exit 2; }
fi

if [ -z "$COMPILER" ]; then
echo "Building..."
if ! make clean >/dev/null 2>&1 || ! make; then
    echo "Build failed." >&2
    exit 1
fi
echo "Build successful."
echo
else
    [ -x "$COMPILER" ] || { echo "Executable not found: $COMPILER" >&2; exit 2; }
fi

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
        "${COMPILER:-./src/lex}" < "$testfile"
        echo
    done

    echo "=============================================================="
    echo "Lex sweep complete."
    exit 0
fi

if [ "$MODE" = target ]; then
    COMPILER=${COMPILER:-./src/targetgen}
    if [ ! -x "$COMPILER" ] && [ -x "$COMPILER.exe" ]; then COMPILER="$COMPILER.exe"; fi
    [ -x "$COMPILER" ] || { echo "Error: targetgen executable not found in ./src/" >&2; exit 1; }
    mkdir -p "$TARGET_OUT"

    # Generate first; invalid programs must fail and leave no assembly, including stale output.
    target_one() {
        local testfile="$1" base name assembly out status log program_out input
        base=$(basename "$testfile")
        name="${base%.c}"
        assembly="$TARGET_OUT/$name.asm"
        rm -f "$assembly"
        out=$("$COMPILER" -o "$assembly" "$testfile" 2>&1)
        status=$?
        case "$base" in
            bad*)
                if [ "$status" -ne 0 ] && [ ! -f "$assembly" ]; then
                    PASS=$((PASS + 1))
                    printf 'PASS  %-20s rejected, no assembly written\n' "$base"
                else
                    FAIL=$((FAIL + 1))
                    printf 'FAIL  %-20s expected rejection without assembly\n' "$base"
                    printf '%s\n' "$out" | sed 's/^/        | /'
                fi
                if [ "$VERBOSE" -eq 1 ]; then printf '%s\n' "$out" | sed 's/^/        | /'; fi
                return ;;
        esac
        if [ "$status" -ne 0 ] || [ ! -s "$assembly" ]; then
            FAIL=$((FAIL + 1))
            printf 'FAIL  %-20s expected assembly, exit %d\n' "$base" "$status"
            printf '%s\n' "$out" | sed 's/^/        | /'
            return
        fi
        if [ "$EXECUTE" -eq 1 ]; then
            log="$TARGET_OUT/$name.log"
            program_out="$TARGET_OUT/$name.output"
            input="$PWD/$TARGET_INPUT/$name.stdin"
            [ -f "$input" ] || input=/dev/null
            if command -v timeout >/dev/null 2>&1; then
                (cd "$TARGET_OUT" && timeout 15 "$SPIM" -file "$name.asm" < "$input") > "$log" 2>&1
            else
                (cd "$TARGET_OUT" && "$SPIM" -file "$name.asm" < "$input") > "$log" 2>&1
            fi
            status=$?
            sed '1,/^Loaded:/d' "$log" > "$program_out"
            if [ "$status" -ne 0 ] || grep -qiE 'Exception occurred|Exception [0-9]|syntax error|undefined|Bad address|Unknown syscall|Attempt to execute|Memory address out of bounds' "$log"; then
                FAIL=$((FAIL + 1))
                printf 'FAIL  %-20s SPIM execution failed, exit %d\n' "$base" "$status"
                sed 's/^/        | /' "$log"
                return
            fi
        fi
        PASS=$((PASS + 1))
        printf 'PASS  %-20s -> %s' "$base" "$assembly"
        [ "$EXECUTE" -eq 0 ] || printf ' (SPIM ran successfully)'
        printf '\n'
        if [ "$VERBOSE" -eq 1 ]; then
            if [ "$EXECUTE" -eq 1 ]; then sed 's/^/        | /' "$program_out";
            else printf '%s\n' "$out" | sed 's/^/        | /'; fi
        fi
    }

    if [ -n "$ONLY" ]; then
        if [ -f "$ONLY" ]; then target="$ONLY";
        elif [ -f "$TARGET_DIR/${ONLY%.c}.c" ]; then target="$TARGET_DIR/${ONLY%.c}.c";
        else
            echo "No such target test: $ONLY" >&2
            ls "$TARGET_DIR"/*.c 2>/dev/null | sed 's|.*/|  |' >&2
            exit 2
        fi
        if [ "$EXECUTE" -eq 1 ]; then VERBOSE=1; fi
        target_one "$target"
    else
        echo "=============================================================="
        echo "MIPS programs -- valid feature tests and invalid bad*.c tests"
        echo "=============================================================="
        for testfile in "$TARGET_DIR"/*.c; do
            [ -e "$testfile" ] || continue
            target_one "$testfile"
        done
    fi
    echo "=============================================================="
    echo "Summary: $PASS/$((PASS + FAIL)) passed"
    echo "=============================================================="
    [ "$FAIL" -eq 0 ] || exit 1
    exit 0
fi

if [ "$MODE" = ir ]; then

    if [ ! -x ./src/irgen ] && [ ! -x ./src/irgen.exe ]; then
        echo "Error: irgen executable not found in ./src/" >&2
        exit 1
    fi

    mkdir -p "$IR_OUT"


    expect_for() {
        case "$(basename "$1")" in
            bad*) echo error ;;
            *)    echo clean ;;
        esac
    }


    ir_problems() {
        local ir="$1" problems="" tgt
        local labels gotos

        labels=$(grep -oE '^L[0-9]+:' "$ir" | tr -d ':' | sort -u)
        gotos=$(grep -oE 'GOTO L[0-9]+' "$ir" | sed 's/GOTO //' | sort -u)
        for tgt in $gotos; do
            printf '%s\n' "$labels" | grep -qx "$tgt" || problems="$problems dangling jump to $tgt;"
        done

        if grep -qE '\?\?\?|\(null\)|nan' "$ir"; then
            problems="$problems placeholder text;"
        fi
        printf '%s' "$problems"
    }

    ir_one() {
        local testfile="$1" expect="$2"
        local base name ir out status problems

        base=$(basename "$testfile")
        name="${base%.c}"
        ir="$IR_OUT/$name.ir"

        out=$("${COMPILER:-./src/irgen}" -o "$ir" "$testfile" 2>&1)
        status=$?

        if [ "$expect" = error ]; then
            if [ "$status" -ne 0 ] && [ ! -f "$ir" ]; then
                PASS=$((PASS + 1))
                printf 'PASS  %-12s rejected, %s semantic error(s)\n' "$base" \
                    "$(printf '%s\n' "$out" | grep -c 'error:')"
            else
                FAIL=$((FAIL + 1))
                if [ "$status" -eq 0 ]; then
                    printf 'FAIL  %-12s should have been rejected, exit 0\n' "$base"
                else
                    printf 'FAIL  %-12s rejected but still wrote %s\n' "$base" "$ir"
                fi
                printf '%s\n' "$out" | sed 's/^/        | /'
            fi

            if [ "$VERBOSE" -eq 1 ]; then
                printf '%s\n' "$out" | sed 's/^/        | /'
                echo
            fi
            return
        fi

        if [ "$status" -ne 0 ]; then
            FAIL=$((FAIL + 1))
            printf 'FAIL  %-12s expected IR, exit %d\n' "$base" "$status"
            printf '%s\n' "$out" | sed 's/^/        | /'
            return
        fi

        if [ ! -s "$ir" ]; then
            FAIL=$((FAIL + 1))
            printf 'FAIL  %-12s wrote an empty .ir\n' "$base"
            return
        fi

        problems=$(ir_problems "$ir")
        if [ -n "$problems" ]; then
            FAIL=$((FAIL + 1))
            printf 'FAIL  %-12s %s\n' "$base" "$problems"
            return
        fi

        PASS=$((PASS + 1))
        printf 'PASS  %-12s %s lines of IR -> %s\n' "$base" \
            "$(wc -l < "$ir" | tr -d ' ')" "$ir"

        if [ "$VERBOSE" -eq 1 ]; then
            sed 's/^/        | /' "$ir"
            echo
        fi
    }

    if [ -n "$ONLY" ]; then

        if [ -f "$ONLY" ]; then
            target="$ONLY"
        elif [ -f "$IR_DIR/${ONLY%.c}.c" ]; then
            target="$IR_DIR/${ONLY%.c}.c"
        else
            echo "No such IR test: $ONLY" >&2
            echo "available:" >&2
            ls "$IR_DIR"/*.c 2>/dev/null | sed 's|.*/|  |' >&2
            exit 2
        fi

        expect=$(expect_for "$target")
        echo "=============================================================="
        echo "$target  (expecting $([ "$expect" = clean ] && echo "IR" || echo "rejection"))"
        echo "=============================================================="
        VERBOSE=1
        ir_one "$target" "$expect"

        [ "$FAIL" -eq 0 ] || exit 1
        exit 0
    fi

    echo "=============================================================="
    echo "Valid programs -- must produce IR"
    echo "=============================================================="
    for testfile in "$IR_DIR"/*.c; do
        [ -e "$testfile" ] || continue
        [ -s "$testfile" ] || continue
        [ "$(expect_for "$testfile")" = clean ] || continue
        ir_one "$testfile" clean
    done

    echo
    echo "=============================================================="
    echo "Invalid programs -- must be rejected, with no .ir written"
    echo "=============================================================="
    for testfile in "$IR_DIR"/*.c; do
        [ -e "$testfile" ] || continue
        [ -s "$testfile" ] || continue
        [ "$(expect_for "$testfile")" = error ] || continue
        ir_one "$testfile" error
    done

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
    echo "./run.sh --ir test5      one test, printing the IR it made"
    echo "./run.sh --ir --verbose  IR for every test"
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
    out=$("${COMPILER:-./src/parser}" "$testfile" 2>&1)
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
echo "./run.sh --ir       for the phase 3 intermediate code"
echo "./run.sh --target   for MIPS assembly"
echo "./run.sh --target --execute  to run assembly with SPIM"
