#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#include "../common/payload.glsl"
#include "../common/scene_data.glsl"
#include "../common/random.glsl"

#include "./light_sampler.glsl"
#include "./infinite_area_light.glsl"

#include "../volume/layout.glsl"
#include "../volume/distance_sampler.glsl"
#include "../volume/transmittance_estimator.glsl"
#include "../volume/phase_function.glsl"

#include "options.glsl"

layout(location = 0) rayPayloadInEXT Payload payload;

PathVertex createVolumeVertex(PathVertex last_vertex, SampledSegment last_segment) {
    PathVertex vertex;
    vertex.valid = true;
    vertex.volume_idx = last_vertex.volume_idx;
    vertex.volume_world_to_object = last_vertex.volume_world_to_object;

    float traveled_distance = last_segment.dist;

    vertex.P = last_vertex.P + traveled_distance * normalize(last_segment.dir);
    vertex.V = -normalize(last_segment.dir);
    return vertex;
}

void main() {
    PathVertex last_vertex = payload.next_vertex;

    if (last_vertex.volume_idx >= 0) {
        PathVertex vertex = createVolumeVertex(last_vertex, payload.next_segment);

        VolumeInstance volume = getVolume(vertex.volume_idx);

        vec3 vol_uv = posToVolumeUV(volume, vertex.P, vertex.volume_world_to_object);
        vec2 coefficients = getCoefficients(volume, vol_uv);

        float absorption = coefficients.x;
        float scattering = coefficients.y;
        float null_scattering = volume.majorant - absorption - scattering;

        float rand = stepAndOutputRNGFloat(payload.rng_state);
        float p_continue = null_scattering / volume.majorant;
        if (rand > p_continue) {
            if (options.sample_light) {
                uint emitter_count = max(1, sceneData.emitter_count);
                LightSample light_sample = sampleEmittingPrimitive(vertex.P, emitter_count, payload.rng_state);
                vec3 L = light_sample.P - vertex.P;
                float distance_to_light = length(L);
                L = normalize(L);

                float sampled_dist = sampleDistance(volume.majorant, payload.rng_state);
                vec3 transmittance = estimateTransmittance(vertex.P, L, distance_to_light, vertex.volume_idx, sampled_dist, payload.rng_state);
                float phase = henyeyGreenstein(vertex.V, L, volume.g);
                if (light_sample.light != vec3(0) && phase > 0.0 && length(transmittance) > 0) {
                    payload.light += scattering * payload.beta * transmittance * phase * light_sample.light / light_sample.pdf;
                }
            }

            PhaseFunctionSample p_sample = sampleHGPhaseFunction(vertex.V, volume.g, payload.rng_state);
            //PhaseFunctionSample p_sample = sampleIsoPhaseFunction(wo, payload.rng_state);

            payload.beta *= scattering / (absorption + scattering); // TODO try to remove this when stopping for absorption
            payload.next_segment.dir = p_sample.wi;
        }


        // This is the full version, without terms cut out for the specific phase function used
        //payload.beta *= scattering * transmittance(traveled_distance, extinction) * p_sample.p / distanceSamplingPdf(traveled_distance, extinction) / p_sample.pdf;
        // This is the version working for homogenous volumes (transmittance and distancePDF can also be cut)
        //payload.beta *= scattering * transmittance(traveled_distance, volume.majorant) / distanceSamplingPdf(traveled_distance, volume.majorant);

        payload.next_vertex = vertex;
        payload.next_segment.dist = sampleDistance(volume.majorant, payload.rng_state);
    } else {
        //float alignment = dot(normalize(payload.next_direction), -normalize(sceneData.sunlightDirection.xyz));
        float alignment = 1;
        if (!options.sample_light || payload.specular_bounce || (payload.depth == 0 && sceneData.sunlightColor.w > 0)) {
            payload.light += payload.beta * alignment * uniformLe();
        }
        payload.beta = vec3(0);
    }

}