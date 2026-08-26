// until loop, plus break / continue / goto

int main() {
    int i, n = 10, sum = 0;

    // until is pre-test: keeps going while the condition is false,
    // so this is the same as while (!(i >= n))
    i = 0;
    until (i >= n) {
        sum += 2;
        i++;
    }

    // no braces
    until (sum > 100) sum = sum + 10;

    // nested
    i = 0;
    until (i >= 3) {
        int j = 0;
        until (j >= 3) {
            sum += i * j;
            j++;
        }
        i++;
    }

    // break and continue
    for (i = 0; i < n; i++) {
        if (i == 3) continue;
        if (i == 8) break;
        sum += i;
    }

    // goto backwards, then forwards
    i = 0;
retry:
    i++;
    if (i < 3) goto retry;

    if (sum > 0) goto done;
    sum = 0;

done:
    return 0;
}
