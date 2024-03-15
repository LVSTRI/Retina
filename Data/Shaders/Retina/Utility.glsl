#ifndef RETINA_SHADERS_UTILITY_HEADER
#define RETINA_SHADERS_UTILITY_HEADER

#define PI 3.1415926535897932384626433832795
#define TWO_PI 6.283185307179586476925286766559
#define FLT_MAX 3.402823466e+38
#define FLT_MIN 1.175494351e-38
#define FLT_LOWEST -3.402823466e+38

float RetinaGetLuminance(in vec3 color) {
  return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 RetinaChangeLuminance(in vec3 color, in float inLum) {
  const float outLum = RetinaGetLuminance(color);
  return color * (inLum / outLum);
}

vec3 RetinaToNonLinearFromLinear(in vec3 color) {
  const bvec3 cutoff = lessThanEqual(color, vec3(0.0031308));
  const vec3 lower = color * 12.92;
  const vec3 higher = 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055;
  return mix(higher, lower, cutoff);
}

bool RetinaIsWithin(in float lower, in float value, in float upper) {
  return lower <= value && value <= upper;
}

vec3 RetinaHsvToRgb(in float hue, in float saturation, in float value) {
  const vec3 rgb = clamp(abs(mod(hue * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
  return value * mix(vec3(1.0), rgb, saturation);
}

vec3 RetinaUnprojectWorldPosition(in vec2 uv, in float depth, in mat4 invProjView) {
  const vec4 ndc = vec4(uv * 2.0 - 1.0, depth, 1.0);
  const vec4 worldPosition = invProjView * ndc;
  return worldPosition.xyz / worldPosition.w;
}

#endif
