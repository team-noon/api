#include <field.h>
#include <game.h>
#include <render.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <types.h>

struct MatchData match;

int main(void) {
  // Setup Field

  srand(time(0));

  game_init(&match);

  struct Render render;
  if (Render_Create(&render, 2 * BORDER_STRIP_WIDTH + FIELD_LENGTH,
                    2 * BORDER_STRIP_WIDTH + FIELD_WIDTH, 0.8))
    return 1;

  float time = getTime();
  while (Render_Update(render) && match.whichHalf < 2) {
    game_update(&match, &time);
    // TOOD

    // Render Stuff
    drawField(render);

    for (int t = 0; t < 2; t++)
      for (int r = 0; r < 4; r++)
        drawMarker(render, match.team[t].robot[r].cs, 0, 0, t ? 1 : 0,
                   t ? 0 : 1);

    drawMarker(render, match.ball, 0, 1, 0, 0);
  }

  Render_Destroy(&render);

  game_destroy(&match);

  // TODO
  return 0;
}