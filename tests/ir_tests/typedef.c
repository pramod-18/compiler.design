typedef int Counter;
typedef unsigned long Size;
typedef char Byte;
typedef int *IntPtr;
typedef float Real;


typedef struct node {
    int value;
    struct node *next;
} Node;


struct point {
    int x;
    int y;
};
typedef struct point Point;


typedef int Row[4];

int main() {
    Counter count = 0;
    Size total = 0;
    Byte ch = 'a';
    Real ratio = 1.5;

    Node n1, n2;
    Node *head;
    Node pool[10];

    Point p;
    Point *pp;

    IntPtr ip;
    Row cells;


    count = count + 1;
    total = total + count;
    ch = ch + 1;
    ratio = ratio * 2.0;


    n1.value = 7;
    n2.value = 8;
    n1.next = &n2;
    n2.next = 0;
    head = &n1;
    head->value = head->value + 1;
    head->next->value = 20;

    pool[0].value = 1;
    pool[0].next = &pool[1];

    p.x = 1;
    pp = &p;
    pp->y = 2;


    ip = &count;
    *ip = 9;
    cells[0] = pp->y;
    cells[1] = cells[0] * 2;

    return count;
}
