#define PI 3.14159265

float safeSqrt(float x) {
    return sqrt(max(0, x));
}
