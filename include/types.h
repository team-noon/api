#pragma once

#ifdef __APPLE__
    #include <lua/lua.h>
#else
    #include <lua.h>
#endif

#define TEAMS 2
#define PLAYERS 4

typedef struct Vec2 {
    float x, y;
} vec2_t;

typedef struct Body {
    vec2_t center, speed;
} body_t;

typedef struct RobotData {
    body_t cs;
    float orientation;
    lua_State *L;
} robotdata_t;

typedef struct TeamData {
    robotdata_t robot[PLAYERS];
} teamdata_t;

typedef struct MatchData {
    teamdata_t team[TEAMS];
    body_t ball;
} matchdata_t;

typedef struct Collider {
    vec2_t *corners, *sizes;
    int a;
    float *radiuses;
    unsigned int count;
} collider;

float vec_length(vec2_t v);
vec2_t vec_norm(vec2_t v);