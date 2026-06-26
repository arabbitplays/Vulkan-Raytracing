#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#include "../common/payload.glsl"
#include "../common/scene_data.glsl"
#include "../common/random.glsl"
#include "../common/path_vertex.glsl"

#include "./light_sampler.glsl"
#include "./infinite_area_light.glsl"

#include "options.glsl"

layout(location = 0) rayPayloadInEXT Payload payload;

PathVertex createEnvironmentVertex() {
    PathVertex vertex = createNewPathVertex();
    vertex.type = ENVIRONMENT_TYPE;
    return vertex;
}

void main() {
    //float alignment = dot(normalize(payload.next_direction), -normalize(sceneData.sunlightDirection.xyz));
    float alignment = 1;
    if (!options.sample_light || payload.specular_bounce || (payload.depth == 0 && sceneData.sunlightColor.w > 0)) {
        payload.light += payload.beta * alignment * uniformLe();
    }
    payload.beta = vec3(0);
    payload.next_vertex = createEnvironmentVertex();
}
