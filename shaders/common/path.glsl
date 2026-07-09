#ifndef PATH_GLSL
#define PATH_GLSL

#include "path_vertex.glsl"

// Hard cap on stored path vertices; sampling clamps to it and the CPU-side
// recursion_depth option mirrors it (RaytracingRenderer::MAX_PATH_LENGTH).
// Keep it tight: the path lives in per-thread scratch memory.
#define MAX_PATH_LENGTH 32
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