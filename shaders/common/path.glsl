#ifndef PATH_GLSL
#define PATH_GLSL

#include "path_vertex.glsl"

// Upper bound on stored path vertices, specialized at pipeline creation
// (MetalRoughMaterial::buildPipelines sets constant_id 0 and rebuilds the
// pipeline when recursion_depth grows beyond it). Keep the default tight:
// the path lives in per-thread scratch memory.
layout(constant_id = 0) const uint MAX_PATH_LENGTH = 32;
struct Path {
    vec3 origin;
    uint len;
    vec3 beta;
    PackedPathVertex vertices[MAX_PATH_LENGTH];
    SampledSegment segments[MAX_PATH_LENGTH];
};
Path path;

void initPath(vec3 origin) {
    path.origin = origin;
    path.len = 0;
    path.beta = vec3(1.0);
}

vec3 getPathVertexPrevP(int idx) {
    return idx == 0 ? path.origin : path.vertices[idx - 1].P;
}

// Unpacks the stored vertex, deriving V from the previous vertex position.
PathVertex getPathVertex(int idx) {
    return unpackPathVertex(path.vertices[idx], getPathVertexPrevP(idx));
}

#endif