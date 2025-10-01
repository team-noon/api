#include <types.h>

#include <math.h>

float vec_length(vec2_t v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

vec2_t vec_norm(vec2_t v) {
    float len = vec_length(v);
    if (len == 0.0f) return (vec2_t){0.0f, 0.0f}; // avoid division by zero
    return (vec2_t){v.x / len, v.y / len};
}