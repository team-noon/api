#ifdef __linux__
    #define _POSIX_C_SOURCE 199309L
#endif

#ifdef __APPLE__
    #include <lua/lua.h>
    #include <lua/lualib.h>
    #include <lua/lauxlib.h>
#else
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
#endif

#include <game.h>
#include <time.h>
#include <field.h>
#include <types.h>

#include <math.h>

float getTime() {
    struct timespec res = {};
    clock_gettime(CLOCK_MONOTONIC, &res);
    return (float)(1000.0f * (float)res.tv_sec + (float)res.tv_nsec / 1e6) / 1000.0f;
}

int lua_api(lua_State* L) {
    // find calling robot
    // (in case coroutines are used; could be a context switch otherwise)
    int args = lua_gettop(L);
    lua_getglobal(L, "team");
    lua_getglobal(L, "player");
    int t = lua_tointeger(L, args+1), r = lua_tointeger(L, args+2);
    for (int i = 0; i < 2; ++i) {
        lua_pop(L, args+1);
    }
    if (!args) return 0; // invalid call

    switch(lua_tointeger(L, 1)) {
        case 00: { // game/state
            lua_pop(L, args);

            lua_pushinteger(L, 0);

            return 1;
        } break;

        // case 10: { // locate/teammate
        //     if (args < 2) return 0;

        //     int id = lua_tointeger(L, 2);

        //     lua_pop(L, args);

        //     lua_pushnumber(L, match.team[t].robot[id].cs.center.x);
        //     lua_pushnumber(L, match.team[t].robot[id].cs.center.y);

        //     return 2;
        // } break;

        // case 11: { // locate/opponent
        //     if (args < 2) return 0;

        //     int id = lua_tointeger(L, 2);

        //     lua_pop(L, args);

        //     lua_pushnumber(L, match.team[t ^ 1].robot[id].cs.center.x);
        //     lua_pushnumber(L, match.team[t ^ 1].robot[id].cs.center.y);

        //     return 2;
        // } break;

        // case 12: { // locate/ball
        //     lua_pop(L, args);

        //     lua_pushnumber(L, match.ball.center.x);
        //     lua_pushnumber(L, match.ball.center.y);

        //     return 2;
        // } break;

        case 20: { // robot/move
            if (args < 3) return 0;

            // float dir = lua_tonumber(L, )

            lua_pop(L, args);
            return 0;
        } break;

        case 21: { // robot/kick
            if (args < 3) return 0;

            lua_pop(L, args);
            return 0;
        } break;

        case 30: { // comms/send
            if (args < 3) return 0;

            int id = lua_tointeger(L, 2);
            // const char *msg = lua_tostring(L, 3, )

            lua_pop(L, args);
            return 0;
        } break;

        case 32: { // comms/popMessage
            lua_pop(L, args);

            lua_pushinteger(L, 0);
            lua_pushnil(L);

            return 2;
        } break;

        default:
            lua_pop(L, args);
            return 0;
            break;
    }
}

int game_init(matchdata_t* match) {
    match->ball.center.x = CENTER_X;
    match->ball.center.y = CENTER_Y;

    // Boot up robots
    for (int t = 0; t < TEAMS; t++) {
        for (int r = 0; r < PLAYERS; r++) {
            robotdata_t* rd = &match->team[t].robot[r];
            rd->cs.center.x = t == 0 ? BORDER_STRIP_WIDTH + (r + 1) * 2 * 2 * MARKER_RADIUS : FIELD_LENGTH + BORDER_STRIP_WIDTH - (r + 1) * 2 * 2 * MARKER_RADIUS;
            rd->cs.center.y = BORDER_STRIP_WIDTH / 2.0f;
            rd->orientation = 0; // TODO

            rd->L = luaL_newstate();
            luaL_openlibs(rd->L);
            lua_pushinteger(rd->L, t);
            lua_setglobal(rd->L, "team");
            lua_pushinteger(rd->L, r);
            lua_setglobal(rd->L, "player");
            luaL_dofile(rd->L, "tacapi.lua");
            lua_register(rd->L, "c_api", lua_api);
            if (luaL_dofile(rd->L, "tactics.lua") != LUA_OK) {  // Run the Lua file
                // Error handling
                fprintf(stderr, "Error: %s\n", lua_tostring(rd->L, -1));
                lua_pop(rd->L, 1);  // Remove error message from stack
                lua_close(rd->L);
                rd->L = NULL;
                // TODO remove from map
            }
        }
    }
}

int game_update(matchdata_t* match, float* time) {
    // Deltatime
    float current = getTime(); 
    float deltaTime = current - *time;
    *time = current;
    // Update Robot Scripts
    for (int t = 0; t < TEAMS; t++) {
        for (int r = 0; r < PLAYERS; r++) {
            robotdata_t* rd = &match->team[t].robot[r];
            lua_getglobal(rd->L, "OnUpdate");
            if (lua_pcall(rd->L, 0, 0, 0) != LUA_OK) { // L, args, returns
                const char *error = lua_tostring(rd->L, -1);
                printf("Error: %s\n", error);
                lua_pop(rd->L, 1); // Remove error message from the stack
            }
            // TODO maybe pop return value
        }
    }
    // Update Physics
    for (int a = 0; a < TEAMS*PLAYERS+1; a++) { // 2*4 robots + ball
        body_t *ba = a != TEAMS*PLAYERS ? &match->team[a < 4].robot[a % 4].cs : &match->ball;
        vec2_t maxStep = {ba->speed.x * deltaTime, ba->speed.y * deltaTime};
        for (int b = 0; b < TEAMS*PLAYERS; b++) {
            if (a == b) continue;
            body_t *bb = &match->team[b < 4].robot[b % 4].cs;
            vec2_t apos = {ba->center.x + maxStep.x, ba->center.y + maxStep.y};
            vec2_t bpos = {bb->center.x, bb->center.y};
            float dist = sqrt(pow(apos.x - bpos.x, 2) + pow(apos.y - bpos.y, 2));
            vec2_t maxnorm = vec_norm(maxStep);
            if (dist <= MARKER_RADIUS * 2 + 0.01) {
               maxStep = (vec2_t){maxnorm.x * (dist - 2 * MARKER_RADIUS), maxnorm.y * (dist - 2 * MARKER_RADIUS)};
            }
        }
        // apply speed
        ba->center.x += maxStep.x, ba->center.y += maxStep.y;
    }
}

int game_destroy(matchdata_t* match) {
    for (int t = 0; t < TEAMS; t++) {
        for (int r = 0; r < PLAYERS; r++) {
            robotdata_t* rd = &match->team[t].robot[r];
            lua_close(rd->L);
        }
    }
}