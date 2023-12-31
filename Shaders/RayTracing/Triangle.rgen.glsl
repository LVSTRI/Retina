#version 460
#include <RayTracing/Common.glsl>

#define RETINA_ENABLE_RAY_TRACING
#include <Bindings.glsl>

struct SCamera {
    mat4 InvProjection;
    mat4 InvView;
    mat4 InvProjView;
    vec4 Position;
};

layout (location = 0) rayPayloadEXT vec3 p_color;

layout (push_constant) uniform UPushConstant {
    uint u_imageId;
    uint u_cameraBufferId;
    uint u_accelerationStructureId;
};

RetinaDeclareStorageImage(restrict writeonly image2D, SImage2DBlock);
RetinaDeclareUniformBuffer(SCamera);

#define u_inputImage RetinaGetStorageImage(SImage2DBlock, u_imageId)
#define u_cameraBuffer RetinaGetUniformBufferData(SCamera, u_cameraBufferId)
#define u_accelerationStructure RetinaGetAccelerationStructure(u_accelerationStructureId)

void main() {
    const vec2 uv = (vec2(gl_LaunchIDEXT.xy) + 0.5) / vec2(gl_LaunchSizeEXT.xy);
    const vec2 ndc = uv * 2.0 - 1.0;

    const vec4 origin = u_cameraBuffer.InvView * vec4(0.0, 0.0, 0.0, 1.0);
    const vec4 target = u_cameraBuffer.InvProjection * vec4(ndc, 1.0, 1.0);
    const vec4 direction = u_cameraBuffer.InvView * vec4(normalize(target.xyz), 0.0);

    const uint rayFlags = gl_RayFlagsOpaqueEXT;
    const float tMin = 0.0001;
    const float tMax = 1000000.0;
    traceRayEXT(
        u_accelerationStructure,
        rayFlags,
        0xff,
        0,
        0,
        0,
        origin.xyz,
        tMin,
        direction.xyz,
        tMax,
        0
    );

    imageStore(u_inputImage, ivec2(gl_LaunchIDEXT.xy), vec4(p_color, 1.0));
}