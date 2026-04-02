#ifndef SHADOW_PAYLOAD_GLSL
#define SHADOW_PAYLOAD_GLSL

struct ShadowPayload {
    vec3 transmittance;
    float dist_left;
    vec3 direction;
    int current_volume_idx;
    mat4x3 volume_world_to_object;
};

#endif
