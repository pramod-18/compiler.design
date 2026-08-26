// structs: members, nesting, -> through a pointer, arrays of structs

struct point {
    int x;
    int y;
};

// members that are structs themselves
struct rect {
    struct point topleft;
    struct point bottomright;
    int area;
};

int main() {
    struct point p, q;
    struct rect r;
    struct point path[4];
    int i;

    // dot access
    p.x = 3;
    p.y = 4;
    q.x = p.x + 1;

    // nested
    r.topleft.x = 0;
    r.topleft.y = 0;
    r.bottomright.x = 10;
    r.bottomright.y = 5;
    r.area = (r.bottomright.x - r.topleft.x) * (r.bottomright.y - r.topleft.y);

    // arrow access
    struct point *pp = &p;
    pp->x = 99;
    pp->y = pp->x + 1;
    q.x = pp->x;

    // array of structs
    for (i = 0; i < 4; i++) {
        path[i].x = i;
        path[i].y = i * i;
    }

    return 0;
}
