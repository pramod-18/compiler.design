struct point {
    int x;
    int y;
};

struct rect {
    struct point topleft;
    struct point bottomright;
    int area;
};

struct row {
    int cells[4];
    char label[8];
    struct point *home;
};

int main() {
    struct point p, q;
    struct rect r;
    struct point path[4];
    struct row line;
    int i;

    p.x = 3;
    p.y = 4;
    q.x = p.x + 1;

    r.topleft.x = 0;
    r.topleft.y = 0;
    r.bottomright.x = 10;
    r.bottomright.y = 5;
    r.area = (r.bottomright.x - r.topleft.x) * (r.bottomright.y - r.topleft.y);

    struct point *pp = &p;
    pp->x = 99;
    pp->y = pp->x + 1;
    q.x = pp->x;

    line.home = &q;
    line.home->y = 8;
    line.cells[0] = pp->x;
    line.cells[1] = line.cells[0] + 1;
    line.label[0] = 'a';

    for (i = 0; i < 4; i++) {
        path[i].x = i;
        path[i].y = i * i;
    }

    struct point origin;
    origin.x = path[0].x;
    origin.y = path[0].y;

    return r.area;
}
