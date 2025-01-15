#define PI 3.14159265

// Random number generation using pcg32i_random_t, using inc = 1. Our random state is a uint.
uint stepRNG(uint rngState) {
    return rngState * 747796405 + 1;
}

// Steps the RNG and returns a floating-point value between 0 and 1 inclusive.
float stepAndOutputRNGFloat(inout uint rngState) {
    rngState = stepRNG(rngState);
    uint word = ((rngState >> ((rngState >> 28) + 4)) ^ rngState) * 277803737;
    word      = (word >> 22) ^ word;
    return float(word) / 4294967295.0f;
}

vec3 sampleCosHemisphere(inout uint rngState) {
    vec2 u = vec2(stepAndOutputRNGFloat(rngState), stepAndOutputRNGFloat(rngState));

    float theta = acos(sqrt(u.x));
    float phi = 2.0 * PI * u.y;

    float x = sin(theta) * cos(phi);
    float y = sin(theta) * sin(phi);
    float z = cos(theta);

    return vec3(x, y, z);
}