#include <Retina/Retina.glsl>
#include <Retina/Utility.glsl>

layout (location = 0) in vec4 i_Color;
layout (location = 1) in vec2 i_Uv;

layout (location = 0) out vec4 o_Pixel;

RetinaDeclarePushConstant() {
  uint u_VertexBufferId;
  uint u_SamplerId;
  uint u_TextureId;
  vec2 u_Scale;
  vec2 u_Translate;
};

#define g_Texture RetinaGetSampledImage(Texture2D, u_TextureId)
#define g_Sampler RetinaGetSampler(u_SamplerId)

void main() {
  vec4 color = RetinaAsLinearColor(i_Color);
  if (RetinaIsHandleValid(u_TextureId)) {
    color *= texture(sampler2D(g_Texture, g_Sampler), i_Uv);
  }
  color.rgb *= color.a;
  color.a = 1.0 - RetinaAsLinearComponent(1.0 - color.a);
  o_Pixel = color;
}
