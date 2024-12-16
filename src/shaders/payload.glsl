struct Payload {
    vec3 color;
    vec4 intersection; // {x, y, z, intersectionType}
    vec4 normal; // {nx, ny, nz, distance}
};