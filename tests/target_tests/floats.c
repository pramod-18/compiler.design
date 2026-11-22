double twice(double x) { return x * 2.0; }
float sum(float x, float y) { return x + y; }
int main() {
    float f = 1.25;
    double d = twice(2.5);
    f += 0.5;
    f *= 2;
    d /= 2;
    int n = (int)(-3.75);
    printf("%f %f %d %d %d\n", f, d, n, f > d, sum(1.5, 2) == 3.5);
    printf("%f %f %f %d\n", -0.125, (double)7, f++, !0.0);
    printf("%f %f\n", f, -d);
    return 0;
}
