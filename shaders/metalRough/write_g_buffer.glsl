#ifndef WRITE_G_BUFFER_GLSL
#define WRITE_G_BUFFER_GLSL

#include "g_buffer.glsl"

vec2 calculateMotionVector(vec3 position) {
    vec4 prevClip = sceneData.last_view_proj * vec4(position, 1.0);
    if (prevClip.w <= 0.0) {
        return vec2(10000, 10000);
    }
    vec3 prevNDC = prevClip.xyz / prevClip.w;
    vec2 prev_uv = prevNDC.xy * 0.5 + 0.5;
    vec2 prev_screen_coord = prev_uv * vec2(gl_LaunchSizeEXT.xy);

    return prev_screen_coord - vec2(gl_LaunchIDEXT.xy);
}


void writePrimaryHitData(uint idx, vec3 P, vec3 N, float depth, int instance_id) {
    GBufferData data = getGBufferData(idx);
    data.position = P;
    data.normal = N;
    vec3 cam_origin = (sceneData.inv_view * vec4(0,0,0,1)).xyz;
    data.depth = length(P - cam_origin);
    data.instance_id = instance_id;
    data.motion = calculateMotionVector(data.position);
    setGBufferData(idx, data);
}

#endif // WRITE_G_BUFFER_GLSL