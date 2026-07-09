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
    vec3 null_scattering;
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
    int similarity_idx; // precomputed table index for the altered phase function
    float similarity_alpha; // scattering scale of the matched similarity table
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
    int type;
    bool is_specular;
};

PathVertex createNewPathVertex() {
    PathVertex vertex;
    vertex.P = vec3(0);
    vertex.geom_N = vec3(0, 0, 1);
    vertex.N = vec3(0, 0, 1);
    vertex.T = vec3(1, 0, 0);
    vertex.V = vec3(0, 0, 1);
    vertex.uv = vec2(0);
    vertex.material_idx = 0;
    vertex.volume_idx = -1;
    vertex.type = INVALID_TYPE;
    vertex.is_specular = false;
    return vertex;
}

// --- packed path vertex ------------------------------------------------------
// Storage form of PathVertex (32 B instead of the original 96 B): V is derived
// from the previous vertex position (V = normalize(prev_P - P), i.e. -ray_dir),
// the volume-local position from P via the volume's world_to_object transform,
// the direction vectors are octahedral-encoded to 2x16-bit snorm, uv is 2x f16,
// and material_idx / volume_idx / type / is_specular share one uint.

vec2 octWrap(vec2 v) {
    return (1.0 - abs(v.yx)) * vec2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

uint packDirectionOct(vec3 n) {
    n /= abs(n.x) + abs(n.y) + abs(n.z);
    vec2 p = n.z >= 0.0 ? n.xy : octWrap(n.xy);
    return packSnorm2x16(p);
}

vec3 unpackDirectionOct(uint bits) {
    vec2 p = unpackSnorm2x16(bits);
    vec3 n = vec3(p, 1.0 - abs(p.x) - abs(p.y));
    if (n.z < 0.0) n.xy = octWrap(n.xy);
    return normalize(n);
}

// meta layout: material_idx in bits 0-15, volume_idx+1 in bits 16-23,
// type in bits 24-26, is_specular in bit 27
uint packVertexMeta(uint material_idx, int volume_idx, int type, bool is_specular) {
    return (material_idx & 0xFFFFu)
         | ((uint(volume_idx + 1) & 0xFFu) << 16)
         | ((uint(type) & 0x7u) << 24)
         | (is_specular ? (1u << 27) : 0u);
}

struct PackedPathVertex {
    vec3 P;
    uint meta;
    uint geom_N_oct;
    uint N_oct;
    uint T_oct;
    uint uv_half;
};

uint getVertexMaterialIdx(PackedPathVertex v) { return v.meta & 0xFFFFu; }
int  getVertexVolumeIdx(PackedPathVertex v)   { return int((v.meta >> 16) & 0xFFu) - 1; }
int  getVertexType(PackedPathVertex v)        { return int((v.meta >> 24) & 0x7u); }
bool isVertexSpecular(PackedPathVertex v)     { return (v.meta & (1u << 27)) != 0u; }
vec3 getVertexGeomN(PackedPathVertex v)       { return unpackDirectionOct(v.geom_N_oct); }
vec3 getVertexN(PackedPathVertex v)           { return unpackDirectionOct(v.N_oct); }
vec3 getVertexT(PackedPathVertex v)           { return unpackDirectionOct(v.T_oct); }
vec2 getVertexUV(PackedPathVertex v)          { return unpackHalf2x16(v.uv_half); }
// V points from the vertex toward the previous one (== -ray_dir of the segment)
vec3 getVertexV(PackedPathVertex v, vec3 prev_P) { return normalize(prev_P - v.P); }

PackedPathVertex packPathVertex(PathVertex v) {
    PackedPathVertex p;
    p.P = v.P;
    p.meta = packVertexMeta(v.material_idx, v.volume_idx, v.type, v.is_specular);
    p.geom_N_oct = packDirectionOct(v.geom_N);
    p.N_oct = packDirectionOct(v.N);
    p.T_oct = packDirectionOct(v.T);
    p.uv_half = packHalf2x16(v.uv);
    return p;
}

PathVertex unpackPathVertex(PackedPathVertex p, vec3 prev_P) {
    PathVertex v;
    v.P = p.P;
    v.geom_N = getVertexGeomN(p);
    v.N = getVertexN(p);
    v.T = getVertexT(p);
    v.V = getVertexV(p, prev_P);
    v.uv = getVertexUV(p);
    v.material_idx = getVertexMaterialIdx(p);
    v.volume_idx = getVertexVolumeIdx(p);
    v.type = getVertexType(p);
    v.is_specular = isVertexSpecular(p);
    return v;
}

SampledSegment createNewSegment() {
    SampledSegment segment;
    segment.bsdf = vec3(1);
    segment.null_scattering = vec3(1);
    segment.transmittance = vec3(1);
    segment.dist_pdf = vec3(1.0);
    segment.delta_pdf = vec3(1.0);
    segment.dir_pdf = 1.0;
    segment.rr_pdf = 1.0;
    return segment;
}

vec3 getPreEvaluationBeta(SampledSegment segment) {
    return segment.null_scattering * segment.transmittance / segment.delta_pdf / segment.dist_pdf;
}

vec3 getPostEvaluationBeta(SampledSegment segment) {
    return segment.bsdf / segment.dir_pdf / segment.rr_pdf;
}

#endif
