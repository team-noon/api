/*
 * types.c — egyszerű 2D vektorműveletek a fizikai lépéshez (pl. game.c: lépés irányának normalizálása).
 * Nincs itt ütközés-szűrés; csak matematikai segédfüggvények.
 */

#include <types.h>

#include <math.h>

/* Vektor hossza: |v| = sqrt(x² + y²) (Pitagorasz-tétel a síkon). */
float vec_length(vec2_t v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

/*
 * Egységvektor (normalizálás): u = v / |v|.
 * Nullvektor esetén {0,0} — elkerüljük a nullával való osztást.
 */
vec2_t vec_norm(vec2_t v) {
    float len = vec_length(v);
    if (len == 0.0f) return (vec2_t){0.0f, 0.0f}; /* avoid division by zero */
    return (vec2_t){v.x / len, v.y / len};
}