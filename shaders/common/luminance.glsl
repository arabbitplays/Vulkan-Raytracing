#ifndef LUMINANCE_GLSL
#define LUMINANCE_GLSL

float luminance(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

#endif
