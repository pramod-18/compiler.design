typedef int Counter;
typedef int *IntPtr;
typedef char Byte;
typedef int Row[3];
typedef struct point { int x; int y; } Point;
typedef float Real;
int main() {
    Counter n = 5;
    IntPtr p = &n;
    Byte ch = 'a';
    Row row = {2, 3, 4};
    Point a;
    Real r = 1.25;
    a.x = 7;
    a.y = 8;
    *p += row[1];
    printf("%d %c %d %d %f %d\n", n, ch, row[2], a.x + a.y, r, sizeof(Row));
    return 0;
}
