layout (location = 0) in SVertexData {
  vec3 Color;
} i_vertexData;

layout (location = 0) out vec4 o_pixel;

void main() {
  o_pixel = vec4(i_vertexData.Color, 1.0);
}