#ifndef MATH_GLSL
#define MATH_GLSL

#define PI 3.14159265
#define INV_4_PI 0.07957747

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

vec3 faceForward(vec3 n, vec3 v) {
    return (dot(n, v) < 0) ? -n : n;
}

float lerp(float x, float a, float b) {
    return (1 - x) * a + x * b;
}

struct Frame {
    vec3 x, y, z;
};

void coordinateSystem(vec3 v1, inout vec3 v2, inout vec3 v3) {
    float sign = sign(v1.z);
    float a = -1.0 / (sign + v1.z);
    float b = v1.x * v1.y * a;
    v2 = vec3(1.0 + sign * sqr(v1.x) * a, sign * b, -sign * v1.x);
    v3 = vec3(b, sign + sqr(v1.y) * a, -v1.y);
}

Frame frameFromZ(vec3 z) {
    vec3 x = vec3(0);
    vec3 y = vec3(0);
    coordinateSystem(z, x, y);
    return Frame(x, y, z);
}

vec3 fromLocal(vec3 v, Frame frame) {
    return v.x * frame.x + v.y * frame.y + v.z * frame.z;
}

vec3 sphericalDirection(float sinTheta, float cosTheta, float phi) {
    return vec3(clamp(sinTheta, -1.0, 1.0) * cos(phi), clamp(sinTheta, -1.0, 1.0) * sin(phi), clamp(cosTheta, -1, 1));
}

#endif