#ifndef VERTEX_EVALUATOR_GLSL
#define VERTEX_EVALUATOR_GLSL

#include "../common/path_vertex.glsl"
#include "material.glsl"
#include "../volume/layout.glsl"

// expects vertex type SURFACE
EvaluatedMaterial evaluateVertexMaterial(PathVertex vertex) {
    EvaluatedMaterial result;
    Material material = getMaterial(vertex.material_idx);
    result.emission_color = material.emission_color;
    result.emission_power = material.emission_power;
    result.albedo = texture(material_textures[material.albedo_tex_idx], vertex.uv).xyz + material.albedo;
    vec3 metal_rough_ao = texture(material_textures[material.metal_rough_ao_tex_idx], vertex.uv).xyz;
    result.metallic = metal_rough_ao.x + material.metallic;
    result.roughness = metal_rough_ao.y + material.roughness;
    result.ao = metal_rough_ao.z + material.ao;
    result.eta = material.eta;
    return result;
}

EvaluatedVolume evaluateVolumeAtPos(VolumeInstance volume, vec3 pos, mat4x3 volume_world_to_object) {
    EvaluatedVolume result;

    vec3 vol_uv = posToVolumeUV(volume, pos, volume_world_to_object);
    vec2 coefficients = getCoefficients(volume, vol_uv);

    result.absorption = coefficients.x;
    result.scattering = coefficients.y;
    result.null_scattering = volume.majorant - result.absorption - result.scattering;
    result.majorant = volume.majorant;
    result.g = volume.g;

    return result;
}

// expects vertex type VOLUME
EvaluatedVolume evaluateVertexVolume(PathVertex vertex) {
    VolumeInstance volume = getVolume(vertex.volume_idx);
    return evaluateVolumeAtPos(volume, vertex.P, vertex.volume_world_to_object);
}


#endif