#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#include "../common/payload.glsl"
#include "../common/scene_data.glsl"
#include "../common/layout.glsl"
#include "../common/random.glsl"

#include "options.glsl"
#include "vertex_evaluator.glsl"

#include "../volume/layout.glsl"
#include "../volume/distance_sampler.glsl"
#include "../volume/transmittance_estimator.glsl"
#include "../volume/phase_function.glsl"
#include "../similarity/altered_phase_function.glsl"

layout(location = 0) rayPayloadInEXT Payload payload;

hitAttributeEXT vec3 attribs;

#include "bsdf_sampler.glsl"
#include "light_sampler.glsl"
#include "../common/path_vertex.glsl"

PathVertex createNewPathVertex() {
    PathVertex vertex;
    vertex.volume_idx = -1;
    vertex.is_valid = true;
    return vertex;
}

SampledSegment createNewSegment() {
    SampledSegment segment;
    segment.pre_eval_beta = vec3(1);
    segment.post_eval_beta = vec3(1);
    return segment;
}

PathVertex createSurfaceVertex() {
    PathVertex vertex = createNewPathVertex();

    Triangle triangle = getTriangle(gl_InstanceCustomIndexEXT, gl_PrimitiveID);
    Vertex A = triangle.A;
    Vertex B = triangle.B;
    Vertex C = triangle.C;

    vertex.material_idx = triangle.material_idx;

    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    float alpha = barycentricCoords.x;
    float beta = barycentricCoords.y;
    float gamma = barycentricCoords.z;

    vec3 local_position = alpha * A.position + beta * B.position + gamma * C.position;
    vec3 local_normal = normalize(alpha * A.normal + beta * B.normal + gamma * C.normal);
    vertex.uv = alpha * A.uv + beta * B.uv + gamma * C.uv;

    vertex.P = vec3(gl_ObjectToWorldEXT * vec4(local_position, 1.0)); // transform position to world space

    vec3 geometric_normal = normalize(vec3(local_normal * gl_WorldToObjectEXT)); // transform normal to world space
    vertex.N = geometric_normal;
    vertex.geom_N = geometric_normal;

    vec3 tangent = normalize(alpha * A.tangent + beta * B.tangent + gamma * C.tangent);
    vertex.T = normalize(vec3(tangent * gl_WorldToObjectEXT)); // transform tangent to world space

    vertex.V = -normalize(gl_WorldRayDirectionEXT);

    if (options.normal_mapping) {
        Material material = getMaterial(triangle.material_idx);
        vec3 texNormal = texture(material_textures[material.normal_tex_idx], vertex.uv).xyz;
        texNormal = texNormal * 2.0 - 1.0;

        mat3 TBN = getTBN(vertex.geom_N, vertex.T);
        vertex.N = normalize(TBN * texNormal);
    }

    return vertex;
}


PathVertex createVolumeBorderVertex(bool entering) {
    PathVertex vertex = createNewPathVertex();

    vertex.is_valid = false;

    Triangle triangle = getTriangle(gl_InstanceCustomIndexEXT, gl_PrimitiveID);
    Vertex A = triangle.A;
    Vertex B = triangle.B;
    Vertex C = triangle.C;

    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    float alpha = barycentricCoords.x;
    float beta = barycentricCoords.y;
    float gamma = barycentricCoords.z;

    vec3 local_position = alpha * A.position + beta * B.position + gamma * C.position;
    vertex.P = vec3(gl_ObjectToWorldEXT * vec4(local_position, 1.0));
    vertex.V = -normalize(gl_WorldRayDirectionEXT);

    vertex.volume_idx = entering ? getVolumeIdx(triangle) : -1;

    return vertex;
}

PathVertex createVolumeVertex(vec3 pos, int volume_idx) {
    PathVertex vertex = createNewPathVertex();

    vertex.P = pos;
    vertex.V = -normalize(gl_WorldRayDirectionEXT);

    vertex.volume_idx = volume_idx;
    vertex.local_volume_pos = (gl_WorldToObjectEXT * vec4(pos, 1.0f)).xyz;

    return vertex;
}

SampledSegment sampleNextSegment(PathVertex vertex, bool sample_bsdf, inout uvec4 rng_state) {
    SampledSegment sampled_segment = createNewSegment();

    mat3 TBN = getTBN(vertex.geom_N, vertex.T);
    mat3 transpose_tbn = transpose(TBN);

    Material material = getMaterial(vertex.material_idx);
    vec3 albedo = texture(material_textures[material.albedo_tex_idx], vertex.uv).xyz + material.albedo;
    vec3 metal_rough_ao = texture(material_textures[material.metal_rough_ao_tex_idx], vertex.uv).xyz;
    float metallic = metal_rough_ao.x + material.metallic;
    float roughness = metal_rough_ao.y + material.roughness;
    float ao = metal_rough_ao.z + material.ao;
    float eta = material.eta;

    if (sample_bsdf) {
        vec3 wo = normalize(transpose_tbn * vertex.V);

        BsdfSample brdf_sample = sampleBsfd(wo, albedo, metallic, roughness, eta, rng_state);

        vec3 sampled_dir = TBN * brdf_sample.wi;
        payload.next_dir = sampled_dir;
        sampled_segment.post_eval_beta = brdf_sample.f * abs(dot(sampled_dir, vertex.N)) / brdf_sample.pdf;
        payload.specular_bounce = isSpecular(brdf_sample.flags);
        if (isTransmissive(brdf_sample.flags))
            payload.eta_scale *= sqr(brdf_sample.eta);
    } else {
        //payload.next_direction = TBN * sampleCosHemisphere(payload.rng_state);
        vec3 sampled_dir = sampleUniformSphere(rng_state);
        payload.next_dir = sampled_dir;

        vec3 wo = normalize(transpose_tbn * vertex.V);
        vec3 wi = normalize(transpose_tbn * sampled_dir);
        //payload.beta *= calcBRDF(wo, wi, albedo, metallic, roughness) * PI;
        float inv_pdf = 4.0 * PI;
        sampled_segment.post_eval_beta = computeBsdf(wo, wi, albedo, metallic, roughness, eta) * abs(dot(sampled_dir, vertex.N)) * inv_pdf;
    }

    return sampled_segment;
}

SampledSegment sampleVolumeSegment(vec3 dir, EvaluatedVolume volume, vec3 delta_tracking_pdf, inout uvec4 rng_state) {
    SampledSegment segment = createNewSegment();
    segment.pre_eval_beta *= delta_tracking_pdf;
    payload.specular_bounce = false;

    if (payload.similarity_relation) {
        if (options.sample_bsdf) {
            PhaseFunctionSample alt_sample = sampleAlteredPhaseFunction(-dir, volume.g, rng_state);
            payload.next_dir = alt_sample.wi;
            segment.post_eval_beta *= volume.scattering * alt_sample.p / alt_sample.pdf;
        } else {
            PhaseFunctionSample iso_sample = sampleIsoPhaseFunction(-dir, rng_state);
            payload.next_dir = iso_sample.wi;
            float phase = evaluateAlteredPhaseFunction(-dir, iso_sample.wi, volume.g);
            segment.post_eval_beta *= volume.scattering * phase / iso_sample.pdf;
        }
        return segment;
    }

    if (options.sample_bsdf) {
        PhaseFunctionSample hg_sample = sampleHGPhaseFunction(-dir, volume.g, rng_state);
        payload.next_dir = hg_sample.wi;
        segment.post_eval_beta *= volume.scattering * hg_sample.p / hg_sample.pdf;
    } else {
        PhaseFunctionSample iso_sample = sampleIsoPhaseFunction(-dir, rng_state);
        payload.next_dir = iso_sample.wi;
        float phase = henyeyGreenstein(-dir, iso_sample.wi, volume.g);
        segment.post_eval_beta *= volume.scattering * phase / iso_sample.pdf;
    }


    // This is the full version, without terms cut out for the specific phase function used
    //payload.beta *= scattering * transmittance(traveled_distance, extinction) * p_sample.p / distanceSamplingPdf(traveled_distance, extinction) / p_sample.pdf;
    // This is the version working for homogenous volumes (transmittance and distancePDF can also be cut)
    //payload.beta *= scattering * transmittance(traveled_distance, volume.majorant) / distanceSamplingPdf(traveled_distance, volume.majorant);

    return segment;
}

vec2 sampleChannel(EvaluatedVolume volume, inout uvec4 rng_state) {
    int lambda = min(int(stepAndOutputRNGFloat(rng_state) * 3.0), 2);

    float sampled_scattering;
    float sampled_absorption;

    switch (lambda)
    {
        case 0:
            sampled_scattering = volume.scattering.x;
            sampled_absorption = volume.absorption.x;
            break;

        case 1:
            sampled_scattering = volume.scattering.y;
            sampled_absorption = volume.absorption.y;
            break;

        default:
            sampled_scattering = volume.scattering.z;
            sampled_absorption = volume.absorption.z;
            break;
    }

    return vec2(sampled_scattering, sampled_absorption);
}

PathVertex deltaTracking(vec3 origin, vec3 dir, int volume_idx, inout uvec4 rng_state) {
    float dist_to_boundary = gl_HitTEXT;
    float tracked_dist = 0;

    VolumeInstance volume_instance = getVolume(volume_idx);

    vec3 delta_tracking_pdf = vec3(1);

    while (true) {
        float sampled_dist = sampleDistance(volume_instance.majorant, rng_state);
        tracked_dist += sampled_dist;

        if (tracked_dist >= dist_to_boundary) {
            break;
        }

        vec3 curr_pos = origin + tracked_dist * dir;

        vec3 obj_pos = (gl_WorldToObjectEXT * vec4(curr_pos, 1.0f)).xyz;
        EvaluatedVolume volume = evaluateVolumeAtLocalPos(volume_instance, payload.similarity_relation, obj_pos);

        vec2 sampled_channel = sampleChannel(volume, rng_state);
        float sampled_scattering = sampled_channel.x;
        float sampled_absorption = sampled_channel.y;
        vec3 null_collision = vec3(volume.majorant) - (volume.scattering + volume.absorption);

        float p_real = (sampled_scattering + sampled_absorption) / volume.majorant;
        float rand = stepAndOutputRNGFloat(rng_state);

    if (rand < p_real) {
            PathVertex vertex = createVolumeVertex(curr_pos, volume_idx);

            delta_tracking_pdf *= 1.0 / (sampled_scattering + sampled_absorption);
            SampledSegment segment = sampleVolumeSegment(dir, volume, delta_tracking_pdf, rng_state);

            payload.beta *= segment.pre_eval_beta;
            payload.light += payload.beta * evaluateVolumeVertex(vertex, payload.similarity_relation, rng_state);
            payload.beta *= segment.post_eval_beta;

            payload.next_segment = segment;
            return vertex;
        } else {
            float p_unreal = max(0.0001f, (1 - p_real));
            delta_tracking_pdf *= null_collision / (volume.majorant * p_unreal);
            continue;
        }
    }

    if (tracked_dist >= dist_to_boundary) {
        // exit volume
        payload.beta *= delta_tracking_pdf;
        return createVolumeBorderVertex(false);
    }

    // should not be reachable
     return createVolumeBorderVertex(false);
}

void main() {
    PathVertex vertex;

    SampledSegment last_segment = payload.next_segment;
    PathVertex last_vertex = payload.next_vertex;

    Triangle triangle = getTriangle(gl_InstanceCustomIndexEXT, gl_PrimitiveID);

    if (isVolumeBoundary(triangle)) {
        if (last_vertex.volume_idx >= 0) {
            // exiting volume or scattering inside
            vertex = deltaTracking(last_vertex.P, normalize(gl_WorldRayDirectionEXT), getVolumeIdx(triangle), payload.rng_state);
        } else {
            // entering volume
            vertex = createVolumeBorderVertex(true);
        }
    } else {
        vertex = createSurfaceVertex();

        EvaluatedMaterial material = evaluateVertexMaterial(vertex);

        // no direct light sampling or handle light that goes directly to the camera
        bool consider_emission = !options.sample_light || payload.specular_bounce || (payload.depth == 0 && material.emission_power > 0);
        payload.light += payload.beta * evaluateSurfaceVertex(vertex, options.sample_bsdf, options.sample_light, consider_emission, payload.similarity_relation, payload.rng_state);

        payload.next_segment = sampleNextSegment(vertex, options.sample_bsdf, payload.rng_state);
        payload.beta *= payload.next_segment.post_eval_beta;

        if (options.russian_roulette) {
            vec3 rr_beta = payload.beta * payload.eta_scale;
            float beta_max_component = max(rr_beta.x, max(rr_beta.y, rr_beta.z));
            if (beta_max_component < 1 && payload.depth > 1) {
                float q = max(0, 1 - beta_max_component);
                float u = stepAndOutputRNGFloat(payload.rng_state);
                if (u < q) {
                    payload.beta = vec3(0);
                } else {
                    payload.beta /= 1 - q;
                }
            }
        }
    }

    payload.next_vertex = vertex;
}
