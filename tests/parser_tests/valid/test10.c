// typedef, in its three usual shapes

// of a basic type
typedef int Counter;
typedef unsigned long Size;
typedef char Byte;

// of a struct defined right here; refers to itself through the tag
typedef struct node {
    int value;
    struct node *next;
} Node;

// of a struct that already exists
struct point {
    int x;
    int y;
};
typedef struct point Point;

int main() {
    Counter count = 0;
    Size total = 0;
    Byte ch = 'a';

    Node n1, n2;
    Node *head;
    Node pool[10];

    Point p;
    Point *pp;

    // typedef'd names behave like any other type
    count = count + 1;
    total = total + count;
    ch = ch + 1;

    // a two-node list
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

    return 0;
}
