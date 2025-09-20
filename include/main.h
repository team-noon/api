#pragma once

#include "match.h"
#include "types.h"
#include "render.h"

float getTime();

float vec_length(vec2_t v);
vec2_t vec_norm(vec2_t v);

// Rendering
void drawField(struct Render render);
void drawMarker(struct Render render, body_t body, float r, float g, float b);

int lua_api(lua_State* L);