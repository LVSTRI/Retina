#version 460
#include <RayTracing/Common.glsl>
#include <RayTracing/Random.glsl>
#include <Utilities.glsl>

struct SViewInfo {
    mat4 InvProjection;
    mat4 InvView;
    mat4 InvProjView;
    vec4 Position;
};

struct SRayPayload {
    bool Miss;
    float RayT;
    vec3 Normal;
    vec3 Color;
};

layout (location = 0) rayPayloadEXT SRayPayload p_rayPayload;

layout (push_constant) uniform UPushConstant {
    uint u_frameIndex;
    uint u_imageId;
    uint u_viewInfoBufferId;
    uint u_vertexBufferId;
    uint u_indexBufferId;
    uint u_objectInfoBufferId;
    uint u_materialBufferId;
    uint u_samplerId;
    uint u_accelerationStructureId;
};

RetinaDeclareStorageImage(restrict image2D, SImage2DBlock);
RetinaDeclareStorageBuffer(restrict readonly, BViewInfo, {
    SViewInfo ViewInfo;
});

#define g_inputImage RetinaGetStorageImage(SImage2DBlock, u_imageId)
#define g_accelerationStructure RetinaGetAccelerationStructure(u_accelerationStructureId)

void main() {
    uint pcgState = InitializeRandomState(u_frameIndex, gl_LaunchIDEXT.xy, gl_LaunchSizeEXT.xy);
    const SViewInfo camera = RetinaGetStorageBufferMember(BViewInfo, ViewInfo, u_viewInfoBufferId);

    const vec2 jitter = RandomUniformVec2(pcgState) - 0.5;
    const vec2 uv = Saturate((vec2(gl_LaunchIDEXT.xy) + jitter) / vec2(gl_LaunchSizeEXT.xy));
    const vec2 ndc = uv * 2.0 - 1.0;
    const vec4 ndc_near = vec4(ndc, 0.0, 1.0);
    const vec4 ndc_far = vec4(ndc, 1.0, 1.0);
    vec4 world_near = camera.InvProjView * ndc_near;
    vec4 world_far = camera.InvProjView * ndc_far;
    world_near /= world_near.w;
    world_far /= world_far.w;

    const uint maxBounces = 8;
    const float tMin = 0.001;
    const float tMax = 10000.0;

    vec3 origin = vec3(world_near);
    vec3 direction = vec3(normalize(world_far - world_near));
    vec3 color = vec3(1.0);
    vec3 incomingLight = vec3(0.0);
    p_rayPayload.Miss = true;
    p_rayPayload.RayT = 0.0;
    p_rayPayload.Color = vec3(0.0);
    p_rayPayload.Normal = vec3(0.0);
    for (uint bounce = 0; bounce < maxBounces; ++bounce) {
        traceRayEXT(g_accelerationStructure, gl_RayFlagsCullBackFacingTrianglesEXT, 0xff, 0, 0, 0, origin, tMin, direction, tMax, 0);
        if (p_rayPayload.Miss) {
            if (bounce == 0) {
                incomingLight = vec3(0.0);
                break;
            }
            p_rayPayload.Miss = false;
            const float sunElevation = radians(50.0);
            const float sunAzimuth = radians(-20.0);
            const vec3 sunDirection = normalize(
                vec3(
                    cos(sunElevation) * sin(sunAzimuth),
                    sin(sunElevation),
                    cos(sunElevation) * cos(sunAzimuth)
                )
            );
            const float sunAngularRadius = 0.00872665;
            const uint shadowTraceRayFlags =
                gl_RayFlagsSkipClosestHitShaderEXT |
                gl_RayFlagsTerminateOnFirstHitEXT;
            direction = RandomDirectionInCone(pcgState, sunDirection, sunAngularRadius);
            traceRayEXT(g_accelerationStructure,  shadowTraceRayFlags, 0xff, 0, 0, 0, origin, tMin, direction, tMax, 0);
            if (p_rayPayload.Miss) {
                incomingLight += 8.0 * color;
            }
            break;
        }
        const vec3 normal = p_rayPayload.Normal;
        const vec3 diffuseDirection = normalize(normal + RandomNormalDirection(pcgState));
        color *= p_rayPayload.Color;
        origin = origin + p_rayPayload.RayT * direction;
        direction = diffuseDirection;
    }
    // average
    if (u_frameIndex > 0) {
        const vec3 previousLighting = vec3(imageLoad(g_inputImage, ivec2(gl_LaunchIDEXT.xy)));
        incomingLight = mix(previousLighting, incomingLight, 1.0 / float(u_frameIndex + 1));
    }
    imageStore(g_inputImage, ivec2(gl_LaunchIDEXT.xy), vec4(incomingLight, 1.0));
}