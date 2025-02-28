#define PI 3.14159265

float safeSqrt(float x) {
    return sqrt(max(0, x));
}

float sqr(float x) {
    return x * x;
}

float lengthSquared(vec3 w) {
    return sqr(w.x) + sqr(w.y) + sqr(w.z);
}

float lengthSquared(vec2 w) {
    return sqr(w.x) + sqr(w.y);
}

float cos2Theta(vec3 w) {
    return w.z * w.z;
}

float cosTheta(vec3 w) {
    return w.z;
}

float absCosTheta(vec3 w) {
    return abs(w.z);
}

float sin2Theta(vec3 w) {
    return max(0, 1 - cos2Theta(w));
}

float sinTheta(vec3 w) {
    return sqrt(sin2Theta(w));
}

float tan2Theta(vec3 w) {
    return sin2Theta(w) / cos2Theta(w);
}

float tanTheta(vec3 w) {
    return sinTheta(w) / cosTheta(w);
}

float sinPhi(vec3 w) {
    float sinTheta = sinTheta(w);
    return (sinTheta == 0) ? 0 : clamp(w.y / sinTheta, -1, 1);
}

float cosPhi(vec3 w) {
    float sinTheta = sinTheta(w);
    return (sinTheta == 0) ? 1 : clamp(w.x / sinTheta, -1, 1);
}

bool sameHemisphere(vec3 w, vec3 v) {
    return w.z * v.z > 0;
}

float lerp(float x, float a, float b) {
    return (1 - x) * a + x * b;
}