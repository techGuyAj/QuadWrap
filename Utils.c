#include"Utils.h"
GLuint createShaderProgram(char* vShader, char* fShader)
{
    unsigned int program;
    program = glCreateProgram();

    //  string vShader = readFromFile(vertexShader);
     // string fShader = readFromFile(fragmentShader);
    unsigned int vs = compileShader(vShader, GL_VERTEX_SHADER);
    unsigned int fs = compileShader(fShader, GL_FRAGMENT_SHADER);
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

GLuint  compileShader(char* ShaderSource, GLuint type)
{
    unsigned int vs = glCreateShader(type);
    glShaderSource(vs, 1, &ShaderSource, NULL);
    glCompileShader(vs);
    //Add error handling code
    int result;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &result);

    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)malloc(length * sizeof(char));

        glGetShaderInfoLog(vs, length, &length, message);

        printf("FAILED  TO COMPIILE shader = %s ", type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");

        printf("Error log : =%s ", message);
        glDeleteShader(vs);
        free(message);

        return 0;
    }

    return vs;
}
GLuint createVBO(float* vertices,int szeInBytes)
{
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,szeInBytes,vertices,GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return vbo;
}