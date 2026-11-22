typedef struct item { char name[8]; int qty; int price; } Item;
int value(Item *items, int count) {
    int sum = 0;
    for (int i = 0; i < count; i++) sum += items[i].qty * items[i].price;
    return sum;
}
void bump(Item *it) { it->qty++; }
int main() {
    Item shelf[4];
    int *scratch = (int *)malloc(4 * sizeof(int));
    if (scratch == 0) return 1;
    for (int i = 0; i < 4; i++) {
        shelf[i].name[0] = 'a' + i;
        shelf[i].name[1] = '\0';
        shelf[i].qty = i + 1;
        shelf[i].price = (i + 1) * 10;
        scratch[i] = i;
    }
    int i = 0;
    until (i >= 4) {
        bump(&shelf[i]);
        i++;
    }
    int total = value(shelf, 4);
    printf("%d %s\n", total, shelf[3].name);
    for (i = 0; i < 4; i++) {
        int j = 0;
        while (j < 4) {
            if (scratch[j] > 2) break;
            total += scratch[j];
            j++;
        }
    }
    free(scratch);
    printf("%d\n", total);
    return 0;
}
