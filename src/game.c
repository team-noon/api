/*
 * game.c — játék főhurka: Lua taktikai API (c_api), meccs inicializálás, pozícióbeállítás,
 * majd minden képkockán: szkriptek (OnUpdate) + egyszerű fizikai integráció.
 *
 * Ütközés / „szűrés” (fontos a bemutatóhoz):
 * - A belső ciklusban jelenleg csak ROBOT–ROBOT párok szerepelnek (b mindig játékos index).
 * - Ha a külső index a labda (a == TEAMS*PLAYERS), a belső ciklusban minden ágra „continue”
 *   lépünk, tehát a labda NEM kap ilyen robot–robot stílusú lépéskorrekciót ebben a blokkban.
 * - A labda sebessége a rúgásnál (case 21) és a mozgatásnál külön kezelődik; a végén csillapítás.
 * - Robot–labda kemény fizikai ütközésfeloldás itt nincs külön rétegként — a taktika Lua oldalon
 *   (pl. távolság-alapú rúgás) egészíti ki.
 */

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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>

#include <math.h>

#define PI 3.14159265358979323846

/* Monoton óra: másodpercben (deltaTime számításhoz, rúgás cooldown-hoz). */
float getTime() {
    struct timespec res = {};
    clock_gettime(CLOCK_MONOTONIC, &res);
    return (float)res.tv_sec + (float)res.tv_nsec / 1e9;
}

/*
 * lua_api — a Lua tacapi.lua által hívott C központi kapu.
 * Az első egész szám a parancs kódja (0: állapot, 10–16: pozíció/sebesség, 20: mozgás, 21: rúgás, stb.).
 * A hívó robot csapatát (team) és indexét (player) globálisból olvassa, a match pointert userdatából.
 */
int lua_api(lua_State* L) {
    /* Megkeressük a hívó robotot (korutin esetén is érdemes így). */
    
    int args = lua_gettop(L);
    if (!args) return 0; // invalid call
    lua_getglobal(L, "team");
    int t = lua_tointeger(L, args+1);
    lua_getglobal(L, "player");
    int r = lua_tointeger(L, args+2);
    lua_getglobal(L, "match");
    matchdata_t* match = lua_touserdata(L, args+3);
    lua_settop(L, args);
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

            lua_pushnumber(L, match->team[t].robot[id].cs.center.x);
            lua_pushnumber(L, match->team[t].robot[id].cs.center.y);

            return 2;
        } break;

        case 11: { // locate/opponent
            if (args < 2) return 0;

            int id = lua_tointeger(L, 2);

            lua_pop(L, args);

            lua_pushnumber(L, match->team[t ^ 1].robot[id].cs.center.x);
            lua_pushnumber(L, match->team[t ^ 1].robot[id].cs.center.y);

            return 2;
        } break;

        case 12: { // locate/ball
            lua_pop(L, args);

            lua_pushnumber(L, match->ball.center.x);
            lua_pushnumber(L, match->ball.center.y);

            return 2;
        } break;

        case 13: { // locate/opponent-goal
            lua_pop(L, args);
            int x = CENTER_X + (match->team[t ^ 1].goal ? (FIELD_LENGTH / 2) : (-FIELD_LENGTH/2));
            int y = CENTER_Y;

            lua_pushnumber(L, x);
            lua_pushnumber(L, y);

            return 2;
        } break;

        case 14: { // locate/teammate-velocity
            if (args < 2) return 0;

            int id = lua_tointeger(L, 2);

            lua_pop(L, args);

            lua_pushnumber(L, match->team[t].robot[id].cs.speed.x);
            lua_pushnumber(L, match->team[t].robot[id].cs.speed.y);

            return 2;
        } break;

        case 15: { // locate/opponent-velocity
            if (args < 2) return 0;

            int id = lua_tointeger(L, 2);

            lua_pop(L, args);

            lua_pushnumber(L, match->team[t ^ 1].robot[id].cs.speed.x);
            lua_pushnumber(L, match->team[t ^ 1].robot[id].cs.speed.y);

            return 2;
        } break;

        case 16: { // locate/ball-velocity
            lua_pop(L, args);

            lua_pushnumber(L, match->ball.speed.x);
            lua_pushnumber(L, match->ball.speed.y);

            return 2;
        } break;

        case 22: { // robot/collisionAge
            // Proxy for "timeOfLastCollision": seconds since last collision.
            float current = getTime();
            float age = current - match->team[t].robot[r].timeOfLastCollision;
            lua_pop(L, args);
            lua_pushnumber(L, age);
            return 1;
        } break;

        case 20: { // robot/move
            if (args < 3) return 0;

            float dir = lua_tonumber(L, 2);
            int speed = lua_tointeger(L, 3);

            match->team[t].robot[r].cs.speed.x = cos(dir)*speed;
            match->team[t].robot[r].cs.speed.y = sin(dir)*speed;
            
            lua_pop(L, args);
            return 0;
        } break;

        case 21: { // robot/kick
            if (args < 3) return 0;

            float dir = lua_tonumber(L, 2);
            int force = lua_tointeger(L, 3);

            lua_pop(L, args);

            float current = getTime();

            if (current - match->timeOfLastKick < 0.5 || current - match->team[t].robot[t].timeOfLastCollision < 2) return 0;

            match->team[t].robot[r].timeOfLastCollision = current;

            float offset_x = rand() % 2 ? (PI / 9) : (-PI / 9);
            float offset_y = rand() % 2 ? (PI / 9) : (-PI / 9);

            match->ball.speed.x = cos(dir + offset_x)*force;
            match->ball.speed.y = sin(dir + offset_y)*force;

            match->timeOfLastKick = current;
            
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

/*
 * game_whereBody — egy test (tipikusan labda) hol van a pályán:
 * kapuban, tizenhatoson, pályán belül vagy kint. Visszatérési kódok: 0–5 (lásd feltételek).
 */
int game_whereBody(matchdata_t* match, body_t* body) {
    /* Kapuban: bal = 1, jobb = 2 */
    if (
        BORDER_STRIP_WIDTH - GOAL_DEPTH < body->center.x &&
        BORDER_STRIP_WIDTH > body->center.x &&
        CENTER_Y - GOAL_WIDTH / 2.0f < body->center.y &&
        CENTER_Y + GOAL_WIDTH / 2.0f > body->center.y 
    ) {
        return 1;
    }
    if (
        BORDER_STRIP_WIDTH + FIELD_LENGTH < body->center.x &&
        BORDER_STRIP_WIDTH + FIELD_LENGTH + GOAL_DEPTH > body->center.x &&
        CENTER_Y - GOAL_WIDTH / 2.0f < body->center.y &&
        CENTER_Y + GOAL_WIDTH / 2.0f > body->center.y
    ) {
        return 2;
    }
    /* Tizenhatos / kapu előtti terület: bal = 3, jobb = 4 */
    if (
        BORDER_STRIP_WIDTH < body->center.x &&
        BORDER_STRIP_WIDTH + GOAL_AREA_LENGTH > body->center.x &&
        CENTER_Y - GOAL_AREA_WIDTH / 2.0f < body->center.y &&
        CENTER_Y + GOAL_AREA_WIDTH / 2.0f > body->center.y 
    ) {
        return 3;
    }
    if (
        BORDER_STRIP_WIDTH + FIELD_LENGTH - GOAL_AREA_LENGTH < body->center.x &&
        BORDER_STRIP_WIDTH + FIELD_LENGTH > body->center.x &&
        CENTER_Y - GOAL_AREA_WIDTH / 2.0f < body->center.y &&
        CENTER_Y + GOAL_AREA_WIDTH / 2.0f > body->center.y 
    ) {
        return 4;
    }
    /* Pályán kívül */
    if (
        body->center.x < BORDER_STRIP_WIDTH ||
        body->center.x > BORDER_STRIP_WIDTH + FIELD_LENGTH ||
        body->center.y < BORDER_STRIP_WIDTH ||
        body->center.y > BORDER_STRIP_WIDTH + FIELD_WIDTH
    ) {
        return 0;
    }
    /* Rendben a játékterületen */
    return 5;
}

/* Bedobás / SET állapot: játékosok és labda kezdőpozíciója, sebességek nullázása. */
int game_setpos(matchdata_t* match) {
    switch (match->state) {
        case SET:
            match->team[match->kickoff].robot[0].cs.center.x = BORDER_STRIP_WIDTH + (match->team[match->kickoff].goal ? (FIELD_LENGTH - GOAL_AREA_LENGTH / 2) : GOAL_AREA_LENGTH / 2);
            match->team[!match->kickoff].robot[0].cs.center.x = BORDER_STRIP_WIDTH + (match->team[!match->kickoff].goal ? (FIELD_LENGTH - GOAL_AREA_LENGTH / 2) : GOAL_AREA_LENGTH / 2);
            match->team[match->kickoff].robot[0].cs.center.y = match->team[!match->kickoff].robot[0].cs.center.y = CENTER_Y;
            for (int r = 1; r < PLAYERS - 1; ++r) {
                match->team[match->kickoff].robot[r].cs.center.x = BORDER_STRIP_WIDTH + (match->team[match->kickoff].goal ? + FIELD_LENGTH * 12/18 : FIELD_LENGTH * 6/18);
                match->team[!match->kickoff].robot[r].cs.center.x = BORDER_STRIP_WIDTH + (match->team[!match->kickoff].goal ? + FIELD_LENGTH * 12/18 : FIELD_LENGTH * 6/18);
                match->team[match->kickoff].robot[r].cs.center.y = BORDER_STRIP_WIDTH + r * (FIELD_WIDTH / (PLAYERS-1));
                match->team[!match->kickoff].robot[r].cs.center.y = BORDER_STRIP_WIDTH + r * (FIELD_WIDTH / (PLAYERS));
            }
            match->team[match->kickoff].robot[PLAYERS - 1].cs.center.x = BORDER_STRIP_WIDTH + (FIELD_LENGTH / 2) + (match->team[match->kickoff].goal ? 30 : -30);
            match->team[match->kickoff].robot[PLAYERS - 1].cs.center.y = CENTER_Y;
            match->team[!match->kickoff].robot[PLAYERS - 1].cs.center.x = BORDER_STRIP_WIDTH + (match->team[!match->kickoff].goal ? + FIELD_LENGTH * 12/18 : FIELD_LENGTH * 6/18);
            match->team[!match->kickoff].robot[PLAYERS - 1].cs.center.y = BORDER_STRIP_WIDTH + (PLAYERS - 1) * (FIELD_WIDTH / (PLAYERS));
            match->ball.center.x = CENTER_X;
            match->ball.center.y = CENTER_Y;
            float current = getTime();
            for (int a = 0; a < TEAMS*PLAYERS+1; a++) {
                body_t *ba = a != TEAMS*PLAYERS ? &match->team[a < 4].robot[a % 4].cs : &match->ball;
                ba->speed.x = 0;
                ba->speed.y = 0;
                if (a != TEAMS*PLAYERS) {
                    robotdata_t *ra = &match->team[a < 4].robot[a % 4];
                    ra->timeOfLastCollision = current;
                }
            }
            
            match->timeOfLastKick = current;
            break;
        case READY:
        case IN_PLAY:
        case DROPPED:
        case STOP:
        case PENALTY:
        case FREE_KICK:
        case PENALTY_KICK:
        case SWITCH_SIDES:
            break;
    }
    return 0;
}

/* Meccs és Lua VM-ek indítása: minden robothoz külön tactics.lua + tacapi.lua. */
int game_init(matchdata_t* match) {
    match->lastTouchedBall = NULL;
    match->whichHalf = 0;
    match->kickoff = rand() % 2;
    match->startofHalf = match->timeOfLastKick = getTime();
    match->team[!match->kickoff].goal = rand() % 2;
    match->team[match->kickoff].goal = !match->team[!match->kickoff].goal;
    game_setpos(match);

    /* Robotok „felélesztése”: Lua állapot, globális team/player/match */
    for (int t = 0; t < TEAMS; t++) {
        for (int r = 0; r < PLAYERS; r++) {
            robotdata_t* rd = &match->team[t].robot[r];
            rd->team = t;
            rd->id = t*PLAYERS + r;
            rd->timeOfLastCollision = getTime();
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
            lua_pushinteger(rd->L, t);
            lua_setglobal(rd->L, "team");
            lua_pushinteger(rd->L, r);
            lua_setglobal(rd->L, "player");
            lua_pushlightuserdata(rd->L, match);
            lua_setglobal(rd->L, "match");
            lua_getglobal(rd->L, "OnInit");
            if (lua_pcall(rd->L, 0, 0, 0) != LUA_OK) { // L, args, returns
                const char *error = lua_tostring(rd->L, -1);
                printf("Error: %s\n", error);
                lua_pop(rd->L, 1); // Remove error message from the stack
            }
        }
    }
    match->state = IN_PLAY;
    return 0;
}

/*
 * game_update — egy szimulációs lépés:
 * 1) deltaTime, 2) meccsállapot (oldalcsere, SET), 3) minden robot OnUpdate(dt),
 * 4) fizika: pozíció frissítés és egyszerű átfedés-kezelés robotok között.
 */
int game_update(matchdata_t* match, float* time) {
    /* Időlépés: előző képkocka óta eltelt másodperc */
    float current = getTime();
    float deltaTime = current - *time;
    *time = current;
    if (match->state == SWITCH_SIDES) {
        match->team[match->kickoff].goal = !match->team[match->kickoff].goal;
        match->team[!match->kickoff].goal = !match->team[!match->kickoff].goal;
        match->kickoff = !match->kickoff;
        match->whichHalf++;
        match->startofHalf = current;
        match->state = SET;
    }
    if (match->state == SET) {
        game_setpos(match);
        match->state = IN_PLAY;
    }
    /* Taktikai szkriptek — minden robot saját Lua kontextusban */
    for (int t = 0; t < TEAMS; t++) {
        for (int r = 0; r < PLAYERS; r++) {
            robotdata_t* rd = &match->team[t].robot[r];
            lua_getglobal(rd->L, "OnUpdate");
            lua_pushnumber(rd->L, deltaTime);
            if (lua_pcall(rd->L, 1, 0, 0) != LUA_OK) { // L, args, returns
                const char *error = lua_tostring(rd->L, -1);
                printf("Error: %s\n", error);
                lua_pop(rd->L, 1); // Remove error message from the stack
            }
            // TODO maybe pop return value
        }
    }
    /*
     * Fizika — pozíció integrálás:
     * - Külső ciklus: minden „test” (8 robot + 1 labda).
     * - Belső ciklus: csak más robotokkal (b < TEAMS*PLAYERS), labda indexnél (a == TEAMS*PLAYERS)
     *   a feltétel miatt nem történik ütközés-számolás — a labda lépése itt nem robot–robot
     *   korrekciót kap.
     * - Ha két robot előre vetített pozíciója túl közel (2×MARKER_RADIUS), a lépéshossz csökkentése
     *   (maxStep módosítása) — ez nem „szellem” ütközés, hanem egyszerű átfedés elkerülése.
     */
    for (int a = 0; a < TEAMS*PLAYERS+1; a++) { /* 8 robot + labda */
        body_t *ba = a != TEAMS*PLAYERS ? &match->team[a < 4].robot[a % 4].cs : &match->ball;
        vec2_t maxStep = {ba->speed.x * deltaTime, ba->speed.y * deltaTime};
        for (int b = 0; b < TEAMS*PLAYERS; b++) {
            if (a == b || a == TEAMS*PLAYERS) continue;
            body_t *bb = &match->team[b < 4].robot[b % 4].cs;
            vec2_t apos = {ba->center.x + maxStep.x, ba->center.y + maxStep.y};
            vec2_t bpos = {bb->center.x, bb->center.y};
            float dist = sqrt(pow(apos.x - bpos.x, 2) + pow(apos.y - bpos.y, 2));
            vec2_t maxnorm = vec_norm(maxStep);
            if (dist <= MARKER_RADIUS * 2 + 0.01) {
                if (current - match->team[a<4].robot[a % 4].timeOfLastCollision > 3) {
                    match->team[a<4].robot[a % 4].timeOfLastCollision = current;
                }
                maxStep = (vec2_t){maxnorm.x * (dist - 2 * MARKER_RADIUS), maxnorm.y * (dist - 2 * MARKER_RADIUS)};
            }
        }
        /* Pozíció alkalmazása; rövid ideig ütközés után a robot nem mozdul (timeOfLastCollision). */
        if (a != TEAMS*PLAYERS && current - match->team[a<4].robot[a % 4].timeOfLastCollision < 2) continue;
        ba->center.x += maxStep.x, ba->center.y += maxStep.y;
        /* Labda csillapítás (súrlódás-szerű hatás) */
        if (a == TEAMS*PLAYERS) {
            float underone = nextafter(0.999, 0);
            ba->speed.x *= underone;
            ba->speed.y *= underone;
        }
    }
    if (game_whereBody(match, &match->ball) < 3) match->state = SET;
    if (current - match->startofHalf > 600) match->state = SWITCH_SIDES;
    fflush(stdout);
    return 0;
}

/* Leállítás: Lua állapotok felszabadítása. */
int game_destroy(matchdata_t* match) {
    for (int t = 0; t < TEAMS; t++) {
        for (int r = 0; r < PLAYERS; r++) {
            robotdata_t* rd = &match->team[t].robot[r];
            lua_close(rd->L);
        }
    }
    return 0;
}