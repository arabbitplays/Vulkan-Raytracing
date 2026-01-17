struct LightSample {
    vec3 P;
    vec3 light;
    float pdf;
};

LightSample sampleEmittingPrimitive(vec3 P, uint emitter_count) {
    float u = stepAndOutputRNGFloat(payload.rng_state);
    uint emitting_instance_idx = min(uint(u * emitter_count), uint(emitter_count - 1u));
    float pmf_light = 1.0 / emitter_count;

    EmittingInstance emitting_instance = emitting_instance_buffer.instances[emitting_instance_idx];

    u = stepAndOutputRNGFloat(payload.rng_state);
    uint primitive_idx = uint(min(u * emitting_instance.primitive_count, emitting_instance.primitive_count - 1));
    float pmf_primitive = 1.0 / emitting_instance.primitive_count;

    Triangle triangle = getTriangle(emitting_instance.instance_idx, primitive_idx);

    u = stepAndOutputRNGFloat(payload.rng_state);
    float v = stepAndOutputRNGFloat(payload.rng_state);
    if (u + v > 1.0) {
        u = 1 - u;
        v = 1 - v;
    }

    vec3 A_pos = vec3(emitting_instance.transform * vec4(triangle.A.position, 1.0));
    vec3 B_pos = vec3(emitting_instance.transform * vec4(triangle.B.position, 1.0));
    vec3 C_pos = vec3(emitting_instance.transform * vec4(triangle.C.position, 1.0));
    vec3 sampled_P = (1 - u - v) * A_pos + u * B_pos + v * C_pos;

    float area = 0.5 * length(cross(B_pos - A_pos, C_pos - A_pos));
    float pdf = 1.0 / area;

    mat3 normal_matrix = transpose(inverse(mat3(emitting_instance.transform)));
    vec3 N = normalize((1 - u - v) * triangle.A.normal + u * triangle.B.normal + v * triangle.C.normal);
    N = normalize(vec3(normal_matrix * N));

    vec3 L = P - sampled_P;

    Material material = getMaterial(triangle.material_idx);
    vec3 li = vec3(0.0);
    float NdotL = dot(normalize(L), N);
    if (NdotL > 0.001) {
        li = material.emission_color * material.emission_power;
        // convert probability to solid angle??
        pdf = pdf * dot(L, L) / abs(NdotL);
    }

    LightSample result;
    result.P = sampled_P;
    result.light = li;
    result.pdf = pmf_light * pmf_primitive * pdf;

    return result;
}

bool unoccluded(vec3 P, vec3 L, float distance_to_light) {
    float tmin = EPSILON;
    float tmax = distance_to_light - EPSILON;
    vec3 direction = L;
    vec3 origin = P;
    uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    isShadowed = true;

    traceRayEXT(topLevelAS, flags, 0xff, 0, 0, 1, origin.xyz, tmin, direction.xyz, tmax, 1);

    return !isShadowed;
}