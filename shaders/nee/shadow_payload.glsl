#ifndef SHADOW_PAYLOAD_GLSL
#define SHADOW_PAYLOAD_GLSL

struct ShadowPayload {
    uvec4 rng_state;
    vec3 transmittance;
    float dist_left;
    float next_distance;
    vec3 next_origin;
    vec3 direction;
    int current_volume_idx;
    mat4x3 volume_world_to_object;
};

#endif
