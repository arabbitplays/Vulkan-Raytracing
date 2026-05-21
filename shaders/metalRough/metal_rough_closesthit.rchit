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

layout(location = 0) rayPayloadInEXT Payload payload;

hitAttributeEXT vec3 attribs;

#include "bsdf_sampler.glsl"
#include "light_sampler.glsl"
#include "../common/path_vertex.glsl"

mat3 getTBN(vec3 geom_N, vec3 T) {
    vec3 bitangent = -normalize(cross(geom_N, T));
    return mat3(T, bitangent, geom_N);
}

PathVertex createNewPathVertex() {
    PathVertex vertex;
    vertex.volume_idx = -1;
    vertex.is_valid = true;
    return vertex;
}

PathVertex createSurfaceVertex() {
    PathVertex vertex = createNewPathVertex();

    Triangle triangle = getTriangle(gl_InstanceCustomIndexEXT, gl_PrimitiveID);
    Vertex A = triangle.A;
    Vertex B = triangle.B;
    Vertex C = triangle.C;

    vertex.material_idx = triangle.material_idx;
    vertex.volume_idx = getVolumeIdx(triangle);
    if (vertex.volume_idx >= 0) {
        vertex.volume_world_to_object = gl_WorldToObjectEXT;
    }

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

SampledSegment sampleNextSegment(PathVertex vertex, bool sample_bsdf, inout uvec4 rng_state) {
    SampledSegment sampled_segment;
    sampled_segment.dist = INFINITY;

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

        sampled_segment.dir = TBN * brdf_sample.wi;
        sampled_segment.brdf = brdf_sample.f * abs(dot(sampled_segment.dir, vertex.N));
        sampled_segment.pdf = brdf_sample.pdf;
        payload.specular_bounce = isSpecular(brdf_sample.flags);
        if (isTransmissive(brdf_sample.flags))
            payload.eta_scale *= sqr(brdf_sample.eta);
    } else {
        //payload.next_direction = TBN * sampleCosHemisphere(payload.rng_state);
        sampled_segment.dir = sampleUniformSphere(rng_state);

        vec3 wo = normalize(transpose_tbn * vertex.V);
        vec3 wi = normalize(transpose_tbn * sampled_segment.dir);
        //payload.beta *= calcBRDF(wo, wi, albedo, metallic, roughness) * PI;
        sampled_segment.brdf = computeBsdf(wo, wi, albedo, metallic, roughness, eta) * abs(dot(sampled_segment.dir, vertex.N));
        sampled_segment.pdf = 1.0f / (4.0f * PI);
    }

    return sampled_segment;
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

    return vertex;
}

struct EvaluationPayload {
    vec3 light;
    vec3 beta;
};

void evaluateVertex(PathVertex vertex, inout EvaluationPayload payload, bool sample_bsdf, bool sample_light, bool consider_emission, inout uvec4 rng_state) {
    EvaluatedMaterial material = evaluateVertexMaterial(vertex);

    // no direct light sampling or handle light that goes directly to the camera
    if (consider_emission && dot(vertex.N, vertex.V) > 0) {
        payload.light += payload.beta * material.emission_color * material.emission_power;
    }

    if (sample_light) {
        mat3 TBN = getTBN(vertex.geom_N, vertex.T);
        mat3 transpose_tbn = transpose(TBN);

        uint emitter_count = max(1, sceneData.emitter_count);
        LightSample light_sample = sampleEmittingPrimitive(vertex.P, emitter_count, rng_state);
        vec3 L = light_sample.P - vertex.P;
        float distance_to_light = length(L);
        L = normalize(L);

        vec3 wo = normalize(transpose_tbn * vertex.V);
        vec3 wi = normalize(transpose_tbn * L);

        vec3 transmittance = estimateTransmittance(vertex.P, L, distance_to_light, rng_state);

        if (sample_bsdf) {
            vec3 f = calcConductorBRDF(wo, wi, material.albedo, material.metallic, material.roughness) * max(dot(vertex.N, L), 0.0);
            if (light_sample.light != vec3(0) && length(f) > 0.0 && length(transmittance) > 0) {
                payload.light += payload.beta * transmittance * f * light_sample.light / light_sample.pdf;
            }
        } else {
            vec3 f = computeBsdf(wo, wi, material.albedo, material.metallic, material.roughness, material.eta) * abs(dot(vertex.N, L));
            if (light_sample.light != vec3(0) && length(f) > 0.0 && length(transmittance) > 0) {
                payload.light += payload.beta * transmittance * f * light_sample.light / light_sample.pdf;
            }
        }
    }
}

void evaluateVolumeVertex(PathVertex vertex) {
    if (options.sample_light) {
        // TODO make this more efficient
        VolumeInstance volume_instance = getVolume(vertex.volume_idx);
        EvaluatedVolume volume = evaluateVolumeAtPos(volume_instance, vertex.P, gl_WorldToObjectEXT);

        uint emitter_count = max(1, sceneData.emitter_count);
        LightSample light_sample = sampleEmittingPrimitive(vertex.P, emitter_count, payload.rng_state);
        vec3 L = light_sample.P - vertex.P;
        float distance_to_light = length(L);
        L = normalize(L);

        vec3 transmittance = estimateTransmittance(vertex.P, L, distance_to_light, vertex.volume_idx, payload.rng_state);
        float phase = henyeyGreenstein(vertex.V, L, volume.g);
        if (light_sample.light != vec3(0) && phase > 0.0 && length(transmittance) > 0) {
            payload.light += volume.scattering * payload.beta * transmittance * phase * light_sample.light / light_sample.pdf;
        }
    }
}

PathVertex deltaTracking(vec3 origin, vec3 dir, int volume_idx, inout uvec4 rng_state) {
    float dist_to_boundary = gl_HitTEXT;
    float tracked_dist = 0;

    VolumeInstance volume_instance = getVolume(volume_idx);

    while (true) {
        float sampled_dist = sampleDistance(volume_instance.majorant, rng_state);
        tracked_dist += sampled_dist;

        if (tracked_dist >= dist_to_boundary) {
            break;
        }

        vec3 curr_pos = origin + tracked_dist * dir;
        EvaluatedVolume volume = evaluateVolumeAtPos(volume_instance, curr_pos, gl_WorldToObjectEXT);

        float p_real = (volume.absorption + volume.scattering) / volume.majorant;
        float rand = stepAndOutputRNGFloat(rng_state);

        if (rand < p_real) {
            // scatter interaction
            PhaseFunctionSample p_sample = sampleHGPhaseFunction(-dir, volume.g, payload.rng_state);
            //PhaseFunctionSample p_sample = sampleIsoPhaseFunction(wo, payload.rng_state);

            // This is the full version, without terms cut out for the specific phase function used
            //payload.beta *= scattering * transmittance(traveled_distance, extinction) * p_sample.p / distanceSamplingPdf(traveled_distance, extinction) / p_sample.pdf;
            // This is the version working for homogenous volumes (transmittance and distancePDF can also be cut)
            //payload.beta *= scattering * transmittance(traveled_distance, volume.majorant) / distanceSamplingPdf(traveled_distance, volume.majorant);

            payload.beta *= 1.0 / (volume.absorption + volume.scattering);
            payload.next_segment.dir = p_sample.wi;

            PathVertex vertex = createVolumeVertex(curr_pos, volume_idx);
            evaluateVolumeVertex(vertex);
            payload.beta *= volume.scattering;
            return vertex;
        } else {
            continue;
        }
    }

    if (tracked_dist >= dist_to_boundary) {
        // exit volume
        return createVolumeBorderVertex(false);
    }
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
        EvaluationPayload eval_payload;
        eval_payload.light = payload.light;
        eval_payload.beta = payload.beta;
        evaluateVertex(vertex, eval_payload, options.sample_bsdf, options.sample_light, consider_emission, payload.rng_state);
        payload.beta = eval_payload.beta;
        payload.light = eval_payload.light;

        payload.next_segment = sampleNextSegment(vertex, options.sample_bsdf, payload.rng_state);
        payload.beta *= payload.next_segment.brdf / payload.next_segment.pdf;

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
