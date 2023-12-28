#version 460

layout (set = 0, binding = 0) restrict readonly buffer BCamera {
    mat4 Projection;
    mat4 View;
    mat4 ProjView;
    vec4 Position;
} b_camera;

layout (location = 0) out vec4 o_color;

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
    gl_Position = b_camera.ProjView * vec4(trianglePositions[gl_VertexIndex], 1.0);
}