#ifndef PATH_VERTEX_GLSL
#define PATH_VERTEX_GLSL

struct SampledSegment {
    vec3 dir;
    float dist;
    float pdf;
    vec3 brdf;
};

struct EvaluatedMaterial {
    vec3 emission_color;
    float emission_power;
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    float eta;
};

struct EvaluatedVolume {
    float scattering;
    float absorption;
    float null_scattering;
    float majorant;
    float g;
};

struct PathVertex {
    vec3 P;
    vec3 geom_N;
    vec3 N;
    vec3 T;
    vec3 V;
    vec2 uv;
    uint material_idx;
    int volume_idx;
    mat4x3 volume_world_to_object;
    bool is_valid;
};

#endif