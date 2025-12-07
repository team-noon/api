#pragma once

#ifdef __APPLE__
    #include <lua/lua.h>
#else
    #include <lua.h>
#endif

#include <types.h>

float getTime();

int lua_api(lua_State* L);

int game_whereBody(matchdata_t* match, body_t* body);

int game_init(matchdata_t* match);

int game_update(matchdata_t* match, float* time);

int game_destroy(matchdata_t* match);