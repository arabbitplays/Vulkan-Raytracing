#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#include "../common/payload.glsl"
#include "../common/scene_data.glsl"
#include "../denoising/svgf_primary_hit.glsl"
#include "push_constants.glsl"

layout(set = 0, binding = 7) uniform sampler2D textures[6];

layout(location = 0) rayPayloadInEXT Payload payload;

void main() {
    if (!options.sample_light || payload.specular_bounce || (payload.depth == 0 && sceneData.sunlightColor.w > 0)) {
        //payload.light += payload.beta * uniformLe();
    }

    if (options.svgf_denoising && payload.depth == 0) {
        writePrimaryHitData(vec3(0), vec3(0), -1.0, -1);
    }
}