#pragma once

const struct constants {
    unsigned int pcm; // pixel/cm conversion

    // official plaing field dimensions
    unsigned int A; // field length
    unsigned int B; // field width
    unsigned int C; // goal depth
    unsigned int D; // goal width
    unsigned int E; // goal area length
    unsigned int F; // goal area width
    unsigned int G; // penalty mark distance
    unsigned int H; // center circle diameter
    unsigned int I; // border strip width
    unsigned int J; // penalty area length
    unsigned int K; // penalty area width

    unsigned int cross; // cross size (unofficial)

    unsigned int marker_r; // ball & robot marker "radius"
} consts = {1, 900, 600, 60, 260, 100, 300, 150, 150, 100, 200, 500, 20, 15};
