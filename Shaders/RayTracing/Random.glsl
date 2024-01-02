#ifndef RETINA_SHADERS_RAY_TRACING_RANDOM_HEADER
#define RETINA_SHADERS_RAY_TRACING_RANDOM_HEADER

uint InitializeRandomState(in uint frame, in uvec2 location, in uvec2 resolution) {
    return frame * resolution.x * resolution.y + location.x * resolution.y + location.y;
}

uint RandomUniform(inout uint state) {
    state = state * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float RandomUniformFloat(inout uint state) {
    return RandomUniform(state) / 4294967296.0;
}

vec2 RandomUniformVec2(inout uint state) {
    return vec2(RandomUniformFloat(state), RandomUniformFloat(state));
}

vec3 RandomUniformVec3(inout uint state) {
    return vec3(RandomUniformFloat(state), RandomUniformFloat(state), RandomUniformFloat(state));
}

vec4 RandomUniformVec4(inout uint state) {
    return vec4(RandomUniformFloat(state), RandomUniformFloat(state), RandomUniformFloat(state), RandomUniformFloat(state));
}

float RandomNormalFloat(inout uint state) {
    const float theta = RandomUniformFloat(state) * 2.0 * 3.141592653589793284626433;
    const float rho = sqrt(-2.0 * log(RandomUniformFloat(state)));
    return rho * cos(theta);
}

vec2 RandomNormalVec2(inout uint state) {
    return vec2(RandomNormalFloat(state), RandomNormalFloat(state));
}

vec3 RandomNormalVec3(inout uint state) {
    return vec3(RandomNormalFloat(state), RandomNormalFloat(state), RandomNormalFloat(state));
}

vec4 RandomNormalVec4(inout uint state) {
    return vec4(RandomNormalFloat(state), RandomNormalFloat(state), RandomNormalFloat(state), RandomNormalFloat(state));
}

vec3 RandomNormalDirection(inout uint state) {
    return normalize(RandomNormalVec3(state));
}

vec3 RandomDirectionInCone(inout uint state, in vec3 direction, in float angle) {
    const float phi = RandomUniformFloat(state) * 2.0 * 3.141592653589793284626433;
    const float z = RandomUniformFloat(state) * (1.0 - cos(angle)) + cos(angle);
    const float x = sqrt(1.0 - z * z) * cos(phi);
    const float y = sqrt(1.0 - z * z) * sin(phi);
    const vec3 tangent = normalize(cross(vec3(0.0, 1.0, 0.0), direction));
    const vec3 bitangent = normalize(cross(direction, tangent));
    const mat3 rotation = mat3(tangent, bitangent, direction);
    return normalize(rotation * vec3(x, y, z));
}

#endif
