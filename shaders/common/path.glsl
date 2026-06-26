#ifndef PATH_GLSL
#define PATH_GLSL

#define MAX_PATH_LENGTH 128
struct Path {
    uint len;
    vec3 beta;
    PathVertex vertices[MAX_PATH_LENGTH];
    SampledSegment segments[MAX_PATH_LENGTH];
};
Path path;

void initPath() {
    path.len = 0;
    path.beta = vec3(1.0);
}

#endif