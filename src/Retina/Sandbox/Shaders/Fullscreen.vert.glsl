layout (location = 0) out vec2 o_Uv;

void main() {
  const vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  o_Uv = uv;
  gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}
