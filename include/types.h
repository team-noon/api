#pragma once

#include <lua/lua.h>

typedef struct Vec2 {
    float x, y;
} vec2_t;

typedef struct Body {
    vec2_t center, speed;
} body_t;

struct MatchData {
    struct TeamData {
        struct RobotData {
            body_t cs;
            float orientation;
            lua_State *L;
        } robot[4];
    } team[2];

    body_t ball;
};
