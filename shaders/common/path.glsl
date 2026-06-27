#ifndef PATH_GLSL
#define PATH_GLSL

#define MAX_PATH_LENGTH 128
struct Path {
    vec3 origin;
    uint len;
    vec3 beta;
    PathVertex vertices[MAX_PATH_LENGTH];
    SampledSegment segments[MAX_PATH_LENGTH];
};
Path path;

void initPath(vec3 origin) {
    path.origin = origin;
    path.len = 0;
    path.beta = vec3(1.0);
}

#endif