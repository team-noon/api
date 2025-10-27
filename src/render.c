#define GL_SILENCE_DEPRECATION

#include <GLFW/glfw3.h>
#ifdef __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

#include <types.h>
#include <render.h>
#include <field.h>
#include <stdio.h>
#include <stdarg.h>

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
    {
        {BORDER_STRIP_WIDTH, BORDER_STRIP_WIDTH},
        {FIELD_LENGTH, FIELD_WIDTH}
    },
    {
        {BORDER_STRIP_WIDTH - GOAL_DEPTH, CENTER_Y - GOAL_WIDTH / 2.0f},
        {BORDER_STRIP_WIDTH + FIELD_LENGTH, CENTER_Y - GOAL_WIDTH / 2.0f},
        {GOAL_DEPTH, GOAL_WIDTH}
    },
    {
        {BORDER_STRIP_WIDTH, CENTER_Y - GOAL_AREA_WIDTH / 2.0f},
        {BORDER_STRIP_WIDTH + FIELD_LENGTH - GOAL_AREA_LENGTH, CENTER_Y - GOAL_AREA_WIDTH / 2.0f},
        {GOAL_AREA_LENGTH, GOAL_AREA_WIDTH}
    },
    {
        {BORDER_STRIP_WIDTH, CENTER_Y - PENALTY_AREA_WIDTH / 2.0f},
        {BORDER_STRIP_WIDTH + FIELD_LENGTH - PENALTY_AREA_LENGTH, CENTER_Y - PENALTY_AREA_WIDTH / 2.0f},
        {PENALTY_AREA_LENGTH, PENALTY_AREA_WIDTH}
    }
};

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