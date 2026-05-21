#ifndef PATH_SAMPLE_GLSL
#define PATH_SAMPLE_GLSL

#include "../common/random.glsl"
#include "../common/constants.glsl"
#include "../common/payload.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(location = 0) rayPayloadEXT Payload payload;

#define MAX_PATH_LENGTH 32
struct Path {
    uint len;
    vec3 beta;
    vec3 light;
    PathVertex vertices[MAX_PATH_LENGTH];
    SampledSegment segments[MAX_PATH_LENGTH];
};
Path path;

void initPayload(vec3 origin, vec3 direction) {
    payload.next_vertex.P = origin;
    payload.next_vertex.volume_idx = -1;
    payload.next_segment.dir = direction;
    payload.next_segment.dist = INFINITY;
    payload.light = vec3(0.0);
    payload.depth = 0;
    payload.beta = vec3(1.0);
    payload.eta_scale = 1;
    payload.specular_bounce = false;
}

void initPath() {
    path.len = 0;
    path.beta = vec3(1.0);
    path.light = vec3(0.0);
}

void addVertexToPath(PathVertex vertex, SampledSegment segment) {
    if (path.len == MAX_PATH_LENGTH)
        return;
    path.vertices[path.len] = vertex;
    path.segments[path.len] = segment;
    path.len++;
}

void continuePath() {
    float tmin = EPSILON;

    traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, payload.next_vertex.P, tmin, payload.next_segment.dir, INFINITY, 0);
    addVertexToPath(payload.next_vertex, payload.next_segment);

    if (payload.next_vertex.is_valid) {
        payload.depth++;
    }
}

vec3 takeSample(uint max_depth) {
    initPath();
    while (payload.depth < max_depth && payload.next_segment.dir != vec3(0.0) && payload.next_segment.dist > 0 && length(payload.beta) > 0) {
        continuePath();
    }
    return payload.light;
}


#endif
