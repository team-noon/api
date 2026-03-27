/*
 * types.h — központi adattípusok a szimulációhoz (meccs, csapatok, robotok, labda).
 * A C oldal itt határozza meg a Lua motorral közös memóriaszerkezetek „vázát”.
 */

#pragma once

#ifdef __APPLE__
    #include <lua/lua.h>
#else
    #include <lua.h>
#endif

/* Csapatok és játékosok száma: jelenleg 2×4 robot + 1 labda a fizikában. */
#define TEAMS 2
#define PLAYERS 4

/* Meccsállapot: játékvezetői logika / szünetek (a jelenlegi kód főként SET, IN_PLAY, SWITCH_SIDES). */
enum MatchState {
    SET,
    READY,
    IN_PLAY,
    DROPPED,
    STOP,
    PENALTY,
    FREE_KICK,
    PENALTY_KICK,
    SWITCH_SIDES
};

/* Kétdimenziós vektor: pozíció vagy sebesség komponensei (x, y). */
typedef struct Vec2 {
    float x, y;
} vec2_t;

/*
 * Fizikai test: középpont (center) + pillanatnyi sebesség (speed).
 * Robotok és labda ugyanezt a struktúrát használják a mozgatáshoz.
 */
typedef struct Body {
    vec2_t center, speed;
} body_t;

/*
 * Robot: csapatazonosító, egyedi id, test (Body), tájolás, utolsó ütközés ideje,
 * és saját Lua állapot (taktikai szkript futtatása).
 */
typedef struct RobotData {
    int team;
    int id;
    body_t cs;
    float orientation;
    float timeOfLastCollision;
    lua_State *L;
} robotdata_t;

/* Csapat: robotok tömbje, eredmény, és melyik kapuhoz van közelebb a „saját” kapu (goal). */
typedef struct TeamData {
    robotdata_t robot[PLAYERS];
    int score;
    uint8_t goal;
} teamdata_t;

/*
 * Meccs: mindkét csapat, a labda teste, bedobás, félidő, utolsó rúgás időbélyege,
 * meccsállapot. A lastTouchedBall későbbi szabályokhoz (pl. touch) fenntartott mező.
 */
typedef struct MatchData {
    teamdata_t team[TEAMS];
    body_t ball;
    uint8_t kickoff;
    uint8_t whichHalf;
    float startofHalf;
    float timeOfLastKick;
    robotdata_t* lastTouchedBall;
    enum MatchState state;
} matchdata_t;

typedef struct Collider {
    vec2_t *corners, *sizes;
    int a;
    float *radiuses;
    unsigned int count;
} collider;

/* Vektorhossz és egységvektor (normalizálás) — implementáció: src/types.c */
float vec_length(vec2_t v);
vec2_t vec_norm(vec2_t v);