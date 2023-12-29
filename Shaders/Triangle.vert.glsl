#version 460
#include <Intrinsics.glsl>
#include <Bindings.glsl>

layout (location = 0) out vec4 o_color;

layout (push_constant) uniform UPushConstants {
    uint u_cameraId;
};

struct SCamera {
    mat4 Projection;
    mat4 View;
    mat4 ProjView;
    vec4 Position;
};

RetinaDeclareStorageBuffer(restrict readonly, BCameraBuffer, {
    SCamera Data;
});
#define cameraBufferPtr RetinaGetStorageBuffer(BCameraBuffer, u_cameraId)

void main() {
    const vec3[] trianglePositions = vec3[](
        vec3(-0.5,  0.5, 0.0),
        vec3( 0.5,  0.5, 0.0),
        vec3( 0.0, -0.5, 0.0)
    );
    const vec3[] triangleColors = vec3[](
        vec3(1.0, 0.0, 0.0),
        vec3(0.0, 1.0, 0.0),
        vec3(0.0, 0.0, 1.0)
    );
    o_color = vec4(triangleColors[gl_VertexIndex], 1.0);
    gl_Position = cameraBufferPtr.Data.ProjView * vec4(trianglePositions[gl_VertexIndex], 1.0);
}