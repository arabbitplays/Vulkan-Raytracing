#ifndef PRIMARY_RAYS_GLSL
#define PRIMARY_RAYS_GLSL

#include "random.glsl"
#include "scene_data.glsl"

vec2 getPixelCenterOffset(inout uvec4 rng_state) {
    return vec2(stepAndOutputRNGFloat(rng_state), stepAndOutputRNGFloat(rng_state));
}

void generatePrimaryRay(ivec2 pixel_coord, ivec2 image_size, inout vec3 origin, inout vec3 direction, inout uvec4 rng_state) {
    const vec2 pixelCenter = vec2(pixel_coord) + getPixelCenterOffset(rng_state);
    const vec2 inUV = pixelCenter / vec2(image_size);
    vec2 d = inUV * 2.0 - 1.0;

    origin = (sceneData.inv_view * vec4(0,0,0,1)).xyz;
    vec4 target = sceneData.inv_proj * vec4(d.x, d.y, 1, 1);
    direction = (sceneData.inv_view * vec4(normalize(target.xyz), 0)).xyz;
}

#endif // PRIMARY_RAYS_GLSL