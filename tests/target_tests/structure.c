struct point { int x; int y; };
struct record {
    char tag;
    int value;
    char name[5];
    double weight;
    struct point pos;
};
int main() {
    struct record r, copy;
    struct point path[3];
    struct record *p = &r;
    r.tag = 'R';
    r.value = 42;
    strcpy(r.name, "row");
    r.weight = 2.5;
    r.pos.x = 7;
    p->pos.y = 9;
    copy = r;
    p->value++;
    for (int i = 0; i < 3; i++) {
        path[i].x = i;
        path[i].y = i * i;
    }
    printf("%c %d %d %s %f %d %d %d\n", copy.tag, copy.value, p->value, copy.name, copy.weight, copy.pos.x + copy.pos.y, path[2].y, sizeof(r));
    return 0;
}
