#include <Retina/Retina.glsl>
#include <Retina/Utility.glsl>

layout (location = 0) in vec4 i_Color;
layout (location = 1) in vec2 i_Uv;

layout (location = 0) out vec4 o_Pixel;

RetinaDeclarePushConstant() {
  uint u_VertexBufferId;
  uint u_SamplerId;
  uint u_TextureId;
  uint u_FontTextureId;
  vec2 u_Scale;
  vec2 u_Translate;
};

#define g_Texture RetinaGetSampledImage(Texture2D, u_TextureId)
#define g_Sampler RetinaGetSampler(u_SamplerId)

void main() {
  if (RetinaIsHandleValid(u_TextureId)) {
    const vec4 sampledColor = texture(sampler2D(g_Texture, g_Sampler), i_Uv);
    if (u_TextureId == u_FontTextureId) {
      o_Pixel = i_Color * sampledColor;
    } else {
      o_Pixel = i_Color * vec4(RetinaAsNonLinear(vec3(sampledColor)), sampledColor.a);
    }
  } else {
    o_Pixel = i_Color;
  }
}
