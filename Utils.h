#pragma once
#include<GL/glew.h>
#include<stdio.h>
enum renderMode {
    InstanceRendering, //Single Draw Call with Instance Count && Offsets
    BatchRendering,   // SingleDraw call with Vetex Data prepared
    NoramlRendering   // DrawCall for each Quad
};
struct QuadProps {
    float dimensions[2];
    float color[3];
    float alpha;
    int numRects;

};
GLuint createShaderProgram(char* vShader, char* fShader);
GLuint  compileShader(char* ShaderSource, GLuint type);
GLuint createVBO(float* vertices , int szeInBytes);