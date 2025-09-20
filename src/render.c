#include "types.h"
#define GL_SILENCE_DEPRECATION

#include <GLFW/glfw3.h>
#ifdef __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

#include <render.h>

#include <stdio.h>
#include <stdarg.h>

int Render_Create(struct Render *render, unsigned int width, unsigned int height, float scale) {
    if (!render) return 1;

    glfwInit();

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    render->window = glfwCreateWindow(width * scale, height * scale, "Tactics Sim", NULL, NULL);
    if (!render->window) return 2;

    // Make OpenGL Context
    glfwMakeContextCurrent(render->window);

    glfwGetFramebufferSize(render->window, &render->width, &render->height);
    render->pcmx = (float)render->width / width, render->pcmy = (float)render->height / height;
    glViewport(0, 0, render->width, render->height);

    // Load OpenGL (exit if failed)
    // if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return 3;
    return 0;
}

void Render_Destroy(struct Render *render) {
    if (!render) return;

    glfwDestroyWindow(render->window);
    render->window = NULL;

    glfwTerminate();
}

int Render_Update(struct Render render) {
    glfwSwapBuffers(render.window);
    glfwPollEvents();

    glfwGetFramebufferSize(render.window, &render.width, &render.height);
    glViewport(0, 0, render.width, render.height);

    glClearColor(0, 0.5, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    return !glfwWindowShouldClose(render.window);
}

void Render_Color(struct Render render, float r, float g, float b) {
    glColor3f(r, g, b);
}

void _render_vertex(struct Render render, vec2_t p) {
    glVertex2f(p.x / render.width * render.pcmx * 2 - 1, p.y / render.height * render.pcmy * -2 + 1);
}

void Render_Line(struct Render render, vec2_t a, vec2_t b) {
    glBegin(GL_LINES);
    _render_vertex(render, a);
    _render_vertex(render, b);
    glEnd();
}

void Render_Rect(struct Render render, vec2_t corner, vec2_t size) {
    glBegin(GL_LINES);
    _render_vertex(render, corner);
    _render_vertex(render, (vec2_t){corner.x + size.x, corner.y});
    _render_vertex(render, (vec2_t){corner.x + size.x, corner.y});
    _render_vertex(render, (vec2_t){corner.x + size.x, corner.y + size.y});
    _render_vertex(render, (vec2_t){corner.x + size.x, corner.y + size.y});
    _render_vertex(render, (vec2_t){corner.x, corner.y + size.y});
    _render_vertex(render, (vec2_t){corner.x, corner.y + size.y});
    _render_vertex(render, corner);
    glEnd();
}

void Render_FilledRect(struct Render render, vec2_t corner, vec2_t size) {
    glBegin(GL_POLYGON);
    _render_vertex(render, corner);
    _render_vertex(render, (vec2_t){corner.x + size.x, corner.y});
    _render_vertex(render, (vec2_t){corner.x + size.x, corner.y + size.y});
    _render_vertex(render, (vec2_t){corner.x, corner.y + size.y});
    glEnd();
}

void Render_Polygon(struct Render render, unsigned int indices, vec2_t first, ...) {
    va_list args;
    va_start(args, first);

    glBegin(GL_POLYGON);
    for (int i = 0; i < indices; i++)
        _render_vertex(render, i == 0 ? first : va_arg(args, vec2_t));
    glEnd();
}

void Render_PolyLine(struct Render render, unsigned int indices, vec2_t first, ...) {
    va_list args;
    va_start(args, first);

    glBegin(GL_LINES);
    for (int i = 0; i < indices; i++)
        _render_vertex(render, i == 0 ? first : va_arg(args, vec2_t));
    glEnd();
}

void Render_Circle(struct Render render, vec2_t center, float radius) {

}

void Render_FilledCircle(struct Render render, vec2_t center, float radius) {

}