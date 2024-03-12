const vec3[] trianglePositions = vec3[](
  vec3( 0.0, -0.5, 0.0),
  vec3(-0.5,  0.5, 0.0),
  vec3( 0.5,  0.5, 0.0)
);

const vec3[] triangleColors = vec3[](
  vec3(1.0, 0.0, 0.0),
  vec3(0.0, 1.0, 0.0),
  vec3(0.0, 0.0, 1.0)
);

layout (location = 0) out vec3 o_color;

void main() {
  o_color = triangleColors[gl_VertexIndex];
  gl_Position = vec4(trianglePositions[gl_VertexIndex], 1.0);
}
