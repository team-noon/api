#ifdef __APPLE__
    #include <lua/lua.h>
    #include <lua/lualib.h>
    #include <lua/lauxlib.h>
#else
    #define _POSIX_C_SOURCE 199309L
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
#endif

#include <match.h>
#include <types.h>
#include <render.h>
#include <main.h>

#include <math.h>
#include <time.h>

// Playing Field Positions
const struct {
    struct {
        vec2_t corner, size;
    } outline;

    struct {
        vec2_t l, r;
        vec2_t size;
    } goal;

    struct {
        vec2_t l, r;
        vec2_t size;
    } goal_area;

    struct {
        vec2_t l, r;
        vec2_t size;
    } penalty_area;
} field = {
    {{BORDER_STRIP_WIDTH, BORDER_STRIP_WIDTH}, {FIELD_LENGTH, FIELD_WIDTH}},
    {{BORDER_STRIP_WIDTH - GOAL_DEPTH, CENTER_Y - GOAL_WIDTH / 2.0f}, {BORDER_STRIP_WIDTH + FIELD_LENGTH, CENTER_Y - GOAL_WIDTH / 2.0f}, {GOAL_DEPTH, GOAL_WIDTH}},
    {{BORDER_STRIP_WIDTH, CENTER_Y - GOAL_AREA_WIDTH / 2.0f}, {BORDER_STRIP_WIDTH + FIELD_LENGTH - GOAL_AREA_LENGTH, CENTER_Y - GOAL_AREA_WIDTH / 2.0f}, {GOAL_AREA_LENGTH, GOAL_AREA_WIDTH}},
    {{BORDER_STRIP_WIDTH, CENTER_Y - PENALTY_AREA_WIDTH / 2.0f}, {BORDER_STRIP_WIDTH + FIELD_LENGTH - PENALTY_AREA_LENGTH, CENTER_Y - PENALTY_AREA_WIDTH / 2.0f}, {PENALTY_AREA_LENGTH, PENALTY_AREA_WIDTH}}};

// Physics Colliders
struct {
    vec2_t *corners, *sizes;
    int a;
    float *radiuses;
    unsigned int count;
} colliders;

struct MatchData match;

float getTime() {
    struct timespec res = {};
    clock_gettime(CLOCK_MONOTONIC, &res);
    return (float)(1000.0f * (float)res.tv_sec + (float)res.tv_nsec / 1e6) / 1000.0f;
}

float vec_length(vec2_t v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

vec2_t vec_norm(vec2_t v) {
    float len = vec_length(v);
    if (len == 0.0f) return (vec2_t){0.0f, 0.0f}; // avoid division by zero
    return (vec2_t){v.x / len, v.y / len};
}

void drawField(struct Render render) {
    Render_Color(render, 1, 1, 1);

    // Center Line
    Render_Line(render, (vec2_t){CENTER_X, BORDER_STRIP_WIDTH}, (vec2_t){BORDER_STRIP_WIDTH + FIELD_LENGTH / 2.0f, BORDER_STRIP_WIDTH + FIELD_WIDTH});

    // Center Circle
    Render_PolyLine(render, 8, (vec2_t){CENTER_X, CENTER_Y - CENTER_CIRCLE_DMTR / 2.0f}, (vec2_t){CENTER_X + CENTER_CIRCLE_DMTR / 2.0f, CENTER_Y}, (vec2_t){CENTER_X + CENTER_CIRCLE_DMTR / 2.0f, CENTER_Y}, (vec2_t){CENTER_X, CENTER_Y + CENTER_CIRCLE_DMTR / 2.0f}, (vec2_t){CENTER_X, CENTER_Y + CENTER_CIRCLE_DMTR / 2.0f}, (vec2_t){CENTER_X - CENTER_CIRCLE_DMTR / 2.0f, CENTER_Y}, (vec2_t){CENTER_X - CENTER_CIRCLE_DMTR / 2.0f, CENTER_Y}, (vec2_t){CENTER_X, CENTER_Y - CENTER_CIRCLE_DMTR / 2.0f});
    
    // center cross
    Render_Line(render, (vec2_t){CENTER_X - 10, CENTER_Y}, (vec2_t){CENTER_X + 10, CENTER_Y});

    // crosses
    Render_Line(render, (vec2_t){BORDER_STRIP_WIDTH + PENALTY_MARK_DIST - CROSS_SIZE / 2.0f, CENTER_Y}, (vec2_t){BORDER_STRIP_WIDTH + PENALTY_MARK_DIST + CROSS_SIZE / 2.0f, CENTER_Y});
    Render_Line(render, (vec2_t){BORDER_STRIP_WIDTH + PENALTY_MARK_DIST, CENTER_Y - CROSS_SIZE / 2.0f}, (vec2_t){BORDER_STRIP_WIDTH + PENALTY_MARK_DIST, CENTER_Y + CROSS_SIZE / 2.0f});
    Render_Line(render, (vec2_t){BORDER_STRIP_WIDTH + FIELD_LENGTH - PENALTY_MARK_DIST - CROSS_SIZE / 2.0f, CENTER_Y}, (vec2_t){BORDER_STRIP_WIDTH + FIELD_LENGTH - PENALTY_MARK_DIST + CROSS_SIZE / 2.0f, CENTER_Y});
    Render_Line(render, (vec2_t){BORDER_STRIP_WIDTH + FIELD_LENGTH - PENALTY_MARK_DIST, CENTER_Y - CROSS_SIZE / 2.0f}, (vec2_t){BORDER_STRIP_WIDTH + FIELD_LENGTH - PENALTY_MARK_DIST, CENTER_Y + CROSS_SIZE / 2.0f});

    Render_Rect(render, field.outline.corner, field.outline.size);      // outline
    Render_Rect(render, field.goal.l, field.goal.size);                 // goals
    Render_Rect(render, field.goal.r, field.goal.size);
    Render_Rect(render, field.goal_area.l, field.goal_area.size);       // goal areas
    Render_Rect(render, field.goal_area.r, field.goal_area.size);
    Render_Rect(render, field.penalty_area.l, field.penalty_area.size); // penalty areas
    Render_Rect(render, field.penalty_area.r, field.penalty_area.size);
}

void drawMarker(struct Render render, body_t body, float r, float g, float b) {
    Render_Color(render, r, g, b);
    Render_Polygon(render, 4, (vec2_t){body.center.x, body.center.y + MARKER_RADIUS}, (vec2_t){body.center.x + MARKER_RADIUS, body.center.y}, (vec2_t){body.center.x, body.center.y - MARKER_RADIUS}, (vec2_t){body.center.x - MARKER_RADIUS, body.center.y});
    Render_Color(render, 1, 0, 1);
    Render_Line(render, body.center,  (vec2_t){body.center.x + body.speed.x, body.center.y + body.speed.y});
}

int lua_api(lua_State* L) {
    // find calling robot
    // (in case coroutines are used; could be a context switch otherwise)
    int t = 0, r = 0;
    for (int i = 0; i < 2 * 4; i++) {
        t = i < 4, r = i % 4;
        if (match.team[t].robot[r].L == L) break;
    }

    int args = lua_gettop(L);
    if (!args) return 0; // invalid call

    switch(lua_tointeger(L, 1)) {
        case 00: { // game/state
            lua_pop(L, args);

            lua_pushinteger(L, 0);

            return 1;
        } break;

        case 10: { // locate/teammate
            if (args < 2) return 0;

            int id = lua_tointeger(L, 2);

            lua_pop(L, args);

            lua_pushnumber(L, match.team[t].robot[id].cs.center.x);
            lua_pushnumber(L, match.team[t].robot[id].cs.center.y);

            return 2;
        } break;

        case 11: { // locate/opponent
            if (args < 2) return 0;

            int id = lua_tointeger(L, 2);

            lua_pop(L, args);

            lua_pushnumber(L, match.team[t ^ 1].robot[id].cs.center.x);
            lua_pushnumber(L, match.team[t ^ 1].robot[id].cs.center.y);

            return 2;
        } break;

        case 12: { // locate/ball
            lua_pop(L, args);

            lua_pushnumber(L, match.ball.center.x);
            lua_pushnumber(L, match.ball.center.y);

            return 2;
        } break;

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

int main(void) {
    // Setup Field
    match.ball.center.x = CENTER_X;
    match.ball.center.y = CENTER_Y;

    // Boot up robots
    for (int t = 0; t < 2; t++) {
        for (int r = 0; r < 4; r++) {
            struct RobotData *rd = &match.team[t].robot[r];
            rd->cs.center.x = t == 0 ? BORDER_STRIP_WIDTH + (r + 1) * 2 * 2 * MARKER_RADIUS : FIELD_LENGTH + BORDER_STRIP_WIDTH - (r + 1) * 2 * 2 * MARKER_RADIUS;
            rd->cs.center.y = BORDER_STRIP_WIDTH / 2.0f;
            rd->orientation = 0; // TODO

            rd->L = luaL_newstate();
            luaL_openlibs(rd->L);
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

    struct Render render;
    if (Render_Create(&render, 2 * 100 + 900, 2 * 100 + 600, 0.8)) return 1;

    float past, now;
    past = now = getTime();
    while (Render_Update(render)) {
        // Calculate DeltaTime
        float deltaTime = now - past;
        past = now, now = getTime();
        
        // Update Robot Scripts
        for (int t = 0; t < 2; t++) {
            for (int r = 0; r < 4; r++) {
                struct RobotData *rd = &match.team[t].robot[r];

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
        for (int a = 0; a < 2 * 4 + 1; a++) { // 2*4 robots + ball
            body_t *ba = a != 2*4 ? &match.team[a < 4].robot[a % 4].cs : &match.ball;
            vec2_t maxStep = {ba->speed.x * deltaTime, ba->speed.y * deltaTime};
            for (int b = 0; b < 2 * 4; b++) {
                if (a == b) continue;

                body_t *bb = &match.team[b < 4].robot[b % 4].cs;

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

        // TOOD

        // Render Stuff
        drawField(render);

        for (int t = 0; t < 2; t++)
            for (int r = 0; r < 4; r++)
                drawMarker(render, match.team[t].robot[r].cs, 0, 0, 1);

        drawMarker(render, match.ball, 1, 0, 0);
    }

    Render_Destroy(&render);

    // Cleanup
    for (int t = 0; t < 2; t++) {
        for (int r = 0; r < 4; r++) {
            struct RobotData *rd = &match.team[t].robot[r];
            lua_close(rd->L);
        }
    }

    // TODO
    return 0;
}