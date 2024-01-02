#version 460

#include <Bindings.glsl>
#include <Utilities.glsl>

layout (location = 0) out vec4 o_output;

layout (push_constant) uniform UPushConstant {
    uint u_inputImageId;
};

RetinaDeclareStorageImage(restrict readonly image2D, SReadonlyImage2DBlock);
#define g_inputImage RetinaGetStorageImage(SReadonlyImage2DBlock, u_inputImageId)

vec3 Tonemap(in vec3 color) {
    const float luminance = GetLuminance(color);
    const vec3 reinhard = color / (color + vec3(1.0));
    return Saturate(mix(color / (luminance + 1.0), reinhard, reinhard));
}

void main() {
    o_output = vec4(ToNonLinearFromLinear(Tonemap(vec3(imageLoad(g_inputImage, ivec2(gl_FragCoord.xy))))), 1.0);
}
