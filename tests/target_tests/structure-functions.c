typedef struct point { int x; int y; } Point;
Point make(int x, int y) {
    Point p = {x, y};
    return p;
}
Point shift(Point p, int by) {
    p.x += by;
    p.y += by;
    return p;
}
Point walk(Point p, int n) {
    if (n == 0) return p;
    p.x++;
    return walk(p, n - 1);
}
int main() {
    Point p = make(3, 4);
    Point q = shift(p, 2);
    Point path[2] = {{1, 2}, {3, 4}};
    printf("%d %d %d %d %d %d %d\n", p.x, p.y, q.x, q.y, make(7, 8).y, walk(p, 5).x, path[1].y);
    return 0;
}
