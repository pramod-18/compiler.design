typedef struct item {
    char name[16];
    int qty;
    int price;
} Item;

int total_value(Item *items, int count);
void bump(Item *it, int by);

int total_value(Item *items, int count) {
    int sum = 0;
    int i;
    for (i = 0; i < count; i++) {
        sum += items[i].qty * items[i].price;
    }
    return sum;
}

void bump(Item *it, int by) {
    it->qty += by;
    if (it->qty < 0) {
        it->qty = 0;
    }
}

int main() {
    Item shelf[4];
    int i, grand = 0;
    char *tag = "shelf";
    int *scratch;
    int n = 4;

    scratch = (int *)malloc(n * sizeof(int));
    if (scratch == 0) return 1;

    for (i = 0; i < n; i++) {
        shelf[i].qty = i + 1;
        shelf[i].price = (i + 1) * 10;
        shelf[i].name[0] = 'a' + i;
        shelf[i].name[1] = '\0';
        scratch[i] = i;
    }

    grand = total_value(shelf, n);

    i = 0;
    until (i >= n) {
        bump(&shelf[i], 1);
        i++;
    }

    grand = total_value(shelf, n);

    printf("%s total = %d\n", tag, grand);

    for (i = 0; i < n; i++) {
        int j = 0;
        while (j < n) {
            if (scratch[j] > 2) break;
            grand += scratch[j];
            j++;
        }
        if (grand > 1000) goto finish;
    }

finish:
    free(scratch);
    return grand;
}
