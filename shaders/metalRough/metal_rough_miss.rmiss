#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#include "../common/payload.glsl"
#include "../common/scene_data.glsl"
#include "options.glsl"


layout(location = 0) rayPayloadInEXT Payload payload;

void main() {
    if (payload.current_volume_idx >= 0) {
        payload.light = vec3(1, 0, 0);
        payload.next_direction = vec3(0);
    } else {
        payload.next_direction = vec3(0);
        if (!options.sample_light || payload.specular_bounce || (payload.depth == 0 && sceneData.sunlightColor.w > 0)) {
            //payload.light += payload.beta * uniformLe();
        }
    }

}