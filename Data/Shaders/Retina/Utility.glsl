#ifndef RETINA_SHADERS_UTILITY_HEADER
#define RETINA_SHADERS_UTILITY_HEADER

#define PI 3.1415926535897932384626433832795
#define TWO_PI 6.283185307179586476925286766559
#define FLT_MAX 3.402823466e+38
#define FLT_MIN 1.175494351e-38
#define FLT_LOWEST -3.402823466e+38
#define FLT_EPS 1.192092896e-07

precise float RetinaAsNonLinearComponent(in float component) {
  const float lower = component * 12.92;
  const float higher = 1.055 * pow(component, 1.0 / 2.4) - 0.055;
  return mix(higher, lower, float(component <= 0.0031308));
}

precise float RetinaAsLinearComponent(in float component) {
  const float lower = component / 12.92;
  const float higher = pow((component + 0.055) / 1.055, 2.4);
  return mix(higher, lower, float(component <= 0.04045));
}

precise vec2 RetinaAsNonLinearColor(in vec2 color) {
  return vec2(
    RetinaAsNonLinearComponent(color.r),
    RetinaAsNonLinearComponent(color.g)
  );
}

precise vec2 RetinaAsLinearColor(in vec2 color) {
  return vec2(
    RetinaAsLinearComponent(color.r),
    RetinaAsLinearComponent(color.g)
  );
}

precise vec3 RetinaAsNonLinearColor(in vec3 color) {
  return vec3(
    RetinaAsNonLinearComponent(color.r),
    RetinaAsNonLinearComponent(color.g),
    RetinaAsNonLinearComponent(color.b)
  );
}

precise vec3 RetinaAsLinearColor(in vec3 color) {
  return vec3(
    RetinaAsLinearComponent(color.r),
    RetinaAsLinearComponent(color.g),
    RetinaAsLinearComponent(color.b)
  );
}

precise vec4 RetinaAsNonLinearColor(in vec4 color) {
  return vec4(
    RetinaAsNonLinearComponent(color.r),
    RetinaAsNonLinearComponent(color.g),
    RetinaAsNonLinearComponent(color.b),
    color.a
  );
}

precise vec4 RetinaAsLinearColor(in vec4 color) {
  return vec4(
    RetinaAsLinearComponent(color.r),
    RetinaAsLinearComponent(color.g),
    RetinaAsLinearComponent(color.b),
    color.a
  );
}

precise float RetinaSaturate(in float value) {
  return clamp(value, 0.0, 1.0);
}

precise vec2 RetinaSaturate(in vec2 value) {
  return clamp(value, 0.0, 1.0);
}

precise vec3 RetinaSaturate(in vec3 value) {
  return clamp(value, 0.0, 1.0);
}

precise vec4 RetinaSaturate(in vec4 value) {
  return clamp(value, 0.0, 1.0);
}

precise bool RetinaIsWithin(in float lower, in float value, in float upper) {
  return lower <= value && value <= upper;
}

precise vec3 RetinaHsvToRgb(in float hue, in float saturation, in float value) {
  const vec3 rgb = clamp(abs(mod(hue * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
  return value * mix(vec3(1.0), rgb, saturation);
}

precise vec3 RetinaUnprojectWorldPosition(in vec2 uv, in float depth, in mat4 invProjView) {
  const vec4 ndc = vec4(uv * 2.0 - 1.0, depth, 1.0);
  const vec4 worldPosition = invProjView * ndc;
  return worldPosition.xyz / worldPosition.w;
}

precise vec2 RetinaOctahedralWrap(in vec2 v) {
  vec2 oneMinusAbs = 1.0 - abs(v.yx);
  oneMinusAbs.x = oneMinusAbs.x * (v.x >= 0.0 ? 1.0 : -1.0);
  oneMinusAbs.y = oneMinusAbs.y * (v.y >= 0.0 ? 1.0 : -1.0);
  return oneMinusAbs;
}

#endif
