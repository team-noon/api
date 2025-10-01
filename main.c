#include <field.h>
#include <types.h>
#include <render.h>
#include <game.h>

struct MatchData match;

int main(void) {
    // Setup Field

    game_init(&match);

    struct Render render;
    if (Render_Create(&render, 2 * 100 + 900, 2 * 100 + 600, 0.8)) return 1;

    float time = getTime();
    while (Render_Update(render)) {
        game_update(&match, &time);
        // TOOD

        // Render Stuff
        drawField(render);

        for (int t = 0; t < 2; t++)
            for (int r = 0; r < 4; r++)
                drawMarker(render, match.team[t].robot[r].cs, 0, 0, 1);

        drawMarker(render, match.ball, 1, 0, 0);
    }

    Render_Destroy(&render);

    game_destroy(&match);

    // TODO
    return 0;
}