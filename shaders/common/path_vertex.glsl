#ifndef PATH_VERTEX_GLSL
#define PATH_VERTEX_GLSL

struct PathVertex {
    vec3 P;
    vec3 geom_N;
    vec3 N;
    vec3 T;
    vec3 V;
    vec2 uv;
    uint material_idx;
    int volume_idx;
};

#endif