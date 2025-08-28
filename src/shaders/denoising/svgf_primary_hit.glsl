#include "../denoising/svgf_layout.glsl"

void writePrimaryHitData(vec3 P, vec3 N, float depth) {
    uint idx = gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x;
    SvgfData data = getSvgfData(idx);
    data.position = P;
    data.normal = N;
    data.depth = depth;
    setSvgfData(idx, data);
}