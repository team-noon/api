#pragma once

#include <types.h>

struct Render {
    struct GLFWwindow *window;

    int width, height;
    float pcmx, pcmy;
};

/// Create Window & Renderer
/// @param render renderer struct
/// @param width width (in centimeters)
/// @param width height (in centimeters)
/// @param pcm pixel - centimeter conversion
/// @return 0: success, else: fail
int Render_Create(struct Render *render, unsigned int width, unsigned int height, float scale);

/// Destroy Window & Renderer
/// @param render renderer struct
void Render_Destroy(struct Render *render);

/// Update Window & Renderer
/// @param render renderer struct
/// @return window state
int Render_Update(struct Render render);

void Render_Color(struct Render render, float r, float g, float b);

void Render_Line(struct Render render, vec2_t a, vec2_t b);

void Render_Rect(struct Render render, vec2_t corner, vec2_t size);
void Render_FilledRect(struct Render render, vec2_t corner, vec2_t size);

void Render_Polygon(struct Render render, unsigned int indices, vec2_t first, ...);
void Render_PolyLine(struct Render render, unsigned int indices, vec2_t first, ...);

void Render_Circle(struct Render render, vec2_t center, float radius);
void Render_FilledCircle(struct Render render, vec2_t center, float radius);
