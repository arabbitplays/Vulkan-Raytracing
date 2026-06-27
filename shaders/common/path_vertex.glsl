#ifndef PATH_VERTEX_GLSL
#define PATH_VERTEX_GLSL

const int SURFACE_TYPE         = 0;
const int VOLUME_TYPE          = 1;
const int VOLUME_BOUNDARY_TYPE = 2;
const int ENVIRONMENT_TYPE     = 3;
const int INVALID_TYPE         = 4;

struct SampledSegment {
    vec3 dist_pdf;
    vec3 delta_pdf; // apply before evaluation
    float dir_pdf;
    float rr_pdf; // apply after evaluation
    vec3 bsdf; // also phase function
    vec3 transmittance; // transmittance before the vertex
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
    bool is_specular;
};

PathVertex createNewPathVertex() {
    PathVertex vertex;
    vertex.volume_idx = -1;
    vertex.type = INVALID_TYPE;
    return vertex;
}

SampledSegment createNewSegment() {
    SampledSegment segment;
    segment.bsdf = vec3(1);
    segment.transmittance = vec3(1);
    segment.dist_pdf = vec3(1.0);
    segment.delta_pdf = vec3(1.0);
    segment.dir_pdf = 1.0;
    segment.rr_pdf = 1.0;
    return segment;
}

vec3 getPreEvaluationBeta(SampledSegment segment) {
    return segment.transmittance / segment.delta_pdf / segment.dist_pdf;
}

vec3 getPostEvaluationBeta(SampledSegment segment) {
    return segment.bsdf / segment.dir_pdf / segment.rr_pdf;
}

#endif
