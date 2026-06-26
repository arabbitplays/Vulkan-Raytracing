#ifndef PATH_VERTEX_GLSL
#define PATH_VERTEX_GLSL

const int SURFACE_TYPE         = 0;
const int VOLUME_TYPE          = 1;
const int VOLUME_BOUNDARY_TYPE = 2;
const int ENVIRONMENT_TYPE     = 3;
const int INVALID_TYPE         = 4;

struct SampledSegment {
    vec3 pre_eval_beta;
    vec3 post_eval_beta;
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
    vec3 scattering;
    vec3 absorption;
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
    vec3 local_volume_pos;
    int type;
};

#endif
