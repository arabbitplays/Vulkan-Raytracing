#ifndef PAYLOAD_GLSL
#define PAYLOAD_GLSL

struct Payload {
    vec3 light;
    int depth;
    uvec4 rng_state;
    vec3 next_direction;
    vec3 next_origin;
    float next_distance;
    vec3 beta;
    float eta_scale; // used for russian roulette
    bool specular_bounce;
    int current_volume_idx;
    mat4x3 volume_world_to_object;
};

#endif