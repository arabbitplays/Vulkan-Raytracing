#ifndef DEBUG_GLSL
#define DEBUG_GLSL

#define MAX_DEPTH 20
vec3 getDepthDebugColor(uint depth) {
    float t = float(depth) / MAX_DEPTH;
    vec3 blue  = vec3(0.0, 0.0, 1.0);
    vec3 green = vec3(0.0, 1.0, 0.0);
    vec3 red   = vec3(1.0, 0.0, 0.0);
    vec3 c = mix(mix(blue, green, clamp(t * 2.0, 0.0, 1.0)),
               red, clamp(t * 2.0 - 1.0, 0.0, 1.0));
    return c;
}

#endif
