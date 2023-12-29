#ifndef RETINA_SHADERS_UTILITIES_HEADER
#define RETINA_SHADERS_UTILITIES_HEADER

float Saturate(in float value) {
    return clamp(value, 0.0, 1.0);
}

vec2 Saturate(in vec2 value) {
    return clamp(value, vec2(0.0), vec2(1.0));
}

vec3 Saturate(in vec3 value) {
    return clamp(value, vec3(0.0), vec3(1.0));
}

vec4 Saturate(in vec4 value) {
    return clamp(value, vec4(0.0), vec4(1.0));
}

float GetLuminance(in vec3 color) {
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 ToNonLinearFromLinear(in vec3 color) {
    const bvec3 cutoff = lessThanEqual(color, vec3(0.0031308));
    const vec3 lower = color * 12.92;
    const vec3 higher = 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055;
    return mix(higher, lower, cutoff);
}

#endif
