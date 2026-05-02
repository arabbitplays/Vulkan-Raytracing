#ifndef PAYLOAD_GLSL
#define PAYLOAD_GLSL

#include "path_vertex.glsl"

struct Payload {
    vec3 light;
    int depth;
    uvec4 rng_state;
    PathVertex next_vertex;
    vec3 next_origin;
    vec3 next_direction;
    float next_distance;
    vec3 beta;
    float eta_scale; // used for russian roulette
    bool specular_bounce;
    int current_volume_idx;
    mat4x3 volume_world_to_object;
};

#endif