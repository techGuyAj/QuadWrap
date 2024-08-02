#include<GL/glew.h>
#include <GLFW/glfw3.h>
#include<cglm/cglm.h>
#include<stdio.h>
#include<time.h>
#include<string.h>
#include"Utils.h"

void init();
void drawFrame();
void render();
void clean();

enum renderMode sceneRendermode;
GLuint program_basic=-1, posLoc_basic, mvpLocBasic, colorLocBasic;
GLuint program_Instanced = -1, posLoc_Instanced, mvpLocInsanced, colorLocInstanced, offsetsLocInstanced;
int screenWidth, screenHeight;
mat4 projMat, viewMat;
float *VertexArray;
float* offsets_Arr; //offsets for Instanced Rendering
float *vertexArr_rect;  // General Quad Vertices
float* vertexArr_BatchRender;  //Vertices to hold the Vertices for all Quad's
int numFrames;
time_t curTime, prevTime;
double elapsed = 0;
float angle = 0;
double angleIncrement = 0;
bool chageRendermode= false;
int maxQuads = 0; // Max Quad's that can fit in the screen.
GLuint vbo; //vbo to handle the vertices for batch rendering
struct QuadProps quad;

/********************************
* Vertex Shader for Batch & Noraml rendering .
**********************************/
 const char* vShader_Basic="#version 330 core\n"
"layout(location = 0) in vec3 position;\n"
"uniform mat4 mvp;\n"
"void main()\n"
"{\n"
"gl_Position = mvp *vec4(position, 1);\n"
"}" ;
 /********************************
* Fragment Shader for All types of rendering.
**********************************/
 const char* fShader_Basic = "#version 330 core\n"
     "layout(location =0) out vec4 color\n;"
     "uniform vec4 u_color;\n"
     "void main(){\n"
     " color=vec4(u_color);\n"
     "}\n";
 /********************************
 * Vertex Shader for Instance rendering , We can re-use the fragShader , No need to create a new one.
 **********************************/
 const char* vShader_Instanced = "#version 330 core\n"
     "layout(location = 0) in vec3 position;\n"
     "uniform mat4 mvp;\n"
     "uniform vec2 offsets[1000];\n"
     "void main()\n"
     "{\n"
     "vec2 offset_ = offsets[gl_InstanceID];\n"
     "gl_Position = mvp * vec4(position + vec3(offset_, 1.0), 1.0);\n"
     "}\n";
 
 /*************************************
 Method to prepare the offsets and the Vertices in the case of Batch rendering.
 And we are using the offsets for both Batch and Normal rendering.
 **************************************/
 void prepateOffsets()
 {
     // Has to prepare re
     //Find max num of rects for given Box dimensions and screen dimemsions.
     //leaving right & left paddiing of 10 pixels

     int numRectsX = (screenWidth - 10) / (quad.dimensions[0] + 10);
     int numRectsY = ceil ((float)quad.numRects/ numRectsX);

     //generate Offsets
     offsets_Arr = (float*)malloc(quad.numRects * 2 * sizeof(float));

     //Allocate Memory for All Quad's For Batch Rendering
     //Need 6Verts for each Quad, So need 6*3 =18 floats to represent Single Quad
     //To-Do remove the Z coordinate as we are doing 2D, We can save some space
     vertexArr_BatchRender = (float*)malloc(quad.numRects * 18 * sizeof(float)); 
     float* tmpBatchPtr = vertexArr_BatchRender;
     bool maxReached = false;

     float* tmp = offsets_Arr;
     int cnt = 0;
     for (int i = 1; i <= numRectsY; i++)
     {
         int y = (screenHeight / 2) - (i * quad.dimensions[1]) - (i * 10) + quad.dimensions[1] / 2;;
         for (int j = 1; j <= numRectsX; j++)
         {
             if (cnt >= quad.numRects)
             {
                 maxReached = true;
                 break;
             }
             int x = (-screenWidth / 2 + (j * quad.dimensions[0])) + j * 10 - quad.dimensions[0] / 2;
             if (tmp != NULL)
             {
                 *tmp++ = x;
                 *tmp++ = y;
             }
             if (tmpBatchPtr != NULL)
             {
                 //Triangle 1
                 *tmpBatchPtr++ =(x- quad.dimensions[0]/2); *tmpBatchPtr++ =(y- quad.dimensions[1] / 2); *tmpBatchPtr++ = 0;
                 *tmpBatchPtr++ =(x+ quad.dimensions[0] / 2); *tmpBatchPtr++ =(y- quad.dimensions[1] / 2) ; *tmpBatchPtr++ =0;
                 *tmpBatchPtr++ =(x+ quad.dimensions[0] / 2); *tmpBatchPtr++ =(y+ quad.dimensions[1] / 2) ; *tmpBatchPtr++ =0;

                 //Triangle 2
                 *tmpBatchPtr++ = (x - quad.dimensions[0] / 2); *tmpBatchPtr++ = (y - quad.dimensions[1] / 2); *tmpBatchPtr++ = 0;
                 *tmpBatchPtr++ = (x + quad.dimensions[0] / 2); *tmpBatchPtr++ = (y + quad.dimensions[1] / 2); *tmpBatchPtr++ = 0;
                 *tmpBatchPtr++ = (x - quad.dimensions[0] / 2); *tmpBatchPtr++ = (y + quad.dimensions[1] / 2); *tmpBatchPtr++ = 0;



             }
             //cout << "x: " << x << " y : " << y <<endl;
             cnt++;
         }
         if (maxReached)
             break;
     }
     maxQuads = cnt;
 //    printf("Prepared the Offsets = %d \n",cnt);

 }
 void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
 {
     if (key == GLFW_KEY_C && action == GLFW_PRESS)
     {
         printf("Pressed C Button \n");
          chageRendermode = true;
     }
 }
int main( int argc , char * argv[])
{
    if (argc > 1)
    {

        //start Parsing the Args

        for (int i = 1; i < argc; i++)
        {
            char arr[100];
            strcpy(arr, argv[i]);
            char* spilitTokens = strtok(arr, "=");
            if (strcmp(spilitTokens, "-num_boxes") == 0)
            {
                while (spilitTokens != NULL)
                {
                   // printf("%s \n", spilitTokens);
                    spilitTokens = strtok(NULL, "=");
                    if (spilitTokens != NULL)
                    {
                        quad.numRects = atoi(spilitTokens);;
                     //   printf("numRects = %d  \n", quad.numRects);
                        
                    }
                }
            }
            else if (strcmp(spilitTokens, "-box_size") == 0)
            {
                int i = 0;
                while (spilitTokens != NULL)
                {
                  //  printf("%s \n", spilitTokens);
                    spilitTokens = strtok(NULL, "x");
                    if (spilitTokens != NULL)
                    {
                        quad.dimensions[i] = atof(spilitTokens);
                  //      printf("rectDimensions[i]= %f  \n", quad.dimensions[i]);
                        i++;
                    }
                }
            }
            else if (strcmp(spilitTokens, "-box_color") == 0)
            {

                int i = 0;
                while (spilitTokens != NULL)
                {
                  //  printf("%s \n", spilitTokens);
                    spilitTokens = strtok(NULL, ",");
                    if (spilitTokens != NULL)
                    {
                        quad.color[i] = atof(spilitTokens);
                  //      printf("boxColor[i]= %f  \n", quad.color[i]);
                        i++;
                    }
                }

            }
            else if (strcmp(spilitTokens, "-box_alpha") == 0)
            {
                while (spilitTokens != NULL)
                {
                //    printf("%s \n", spilitTokens);
                    spilitTokens = strtok(NULL, "=");
                    if (spilitTokens != NULL)
                    {
                        quad.alpha = atoi(spilitTokens);
                    //    printf("numRects = %d  \n", quad.alpha);
                    }
                }

            }

        }

    }
    else {

        quad.numRects = 1000;
        quad.color[0] = 255;
        quad.color[1] = 255;
        quad.color[2] = 0;
        quad.dimensions[0] = 1000;
        quad.dimensions[1] = 10;
        quad.alpha = 255;
    }
   

    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;
    int min, max;
    glfwGetVersion(&max, &min, NULL);
    const GLFWvidmode* vidMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
   
    screenWidth = vidMode->width;
    screenHeight = vidMode->height;
    //rectDimensions[0] = 200;
    //rectDimensions[1] = 100;
    //numRects = 100;
    window = glfwCreateWindow(screenWidth, screenHeight, "OpenGL", NULL, NULL);
   
   
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
   
    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    GLenum err = glewInit();
    if (err != GLEW_OK)
        printf( "error with GLEW initialization \n");
    //std::cout << "opengl version " << glGetString(GL_VERSION) << std::endl;
    printf("opengl version  = %s \n" , glGetString(GL_VERSION));
    GLuint maxVertUniformsVect;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxVertUniformsVect);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwSetKeyCallback(window, key_callback);
    init();
    time(&curTime);
    prevTime = curTime;
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);
        time(&curTime);
        elapsed += difftime(curTime, prevTime); // Will return the elpased time in Seconds
        if (elapsed >= 1)
        {

          //  printf("FPS =  %d \n", numFrames );
            fprintf(stderr, "FPS =  %d \n", numFrames);
            angleIncrement = (double)30.0/numFrames;  //To move 30degree's ,Increment this much amount of angle.
            numFrames = 0;
            elapsed = 0;
            
        }
        prevTime = curTime;
        drawFrame();
        glfwSwapBuffers(window);
        numFrames++;
        /* Poll for and process events */
        glfwPollEvents();
   
    }
   // glDeleteProgram(shader);
    glfwTerminate();
    clean();
	printf("Program is terminated and clean-up is done. \n");
    return 0;
}
/*******************************************
Preparing VertexData Required For Batch rendering.
***************************************/
void prepareVerticesForBatchRendering()
{

}
/*******************************************
Method to prepare VertexData,programs,View & Projections matrices and all the other required things
***************************************/
void init()
{
    sceneRendermode = BatchRendering;  //setting  Instance rendering

    //Idle way is to prepare the Quad around origin(0,0,0)
    program_basic =createShaderProgram(vShader_Basic, fShader_Basic);
    posLoc_basic = glGetAttribLocation(program_basic, "position");
    mvpLocBasic =   glGetUniformLocation(program_basic, "mvp");
    colorLocBasic = glGetUniformLocation(program_basic, "u_color");

    program_Instanced = createShaderProgram(vShader_Instanced, fShader_Basic);
    posLoc_Instanced = glGetAttribLocation(program_Instanced, "position");
    mvpLocInsanced = glGetUniformLocation(program_Instanced, "mvp");
    colorLocInstanced = glGetUniformLocation(program_Instanced, "u_color");
    offsetsLocInstanced = glGetUniformLocation(program_Instanced, "offsets");

    vec3 eye1 = { 0, 0, 10 };
    vec3 center1 = { 0, 0, 0 };
    vec3 up1 = { 0, 1, 0 };
    glm_lookat(eye1, center1, up1, viewMat);
    glm_ortho(-screenWidth / 2, screenWidth / 2, -screenHeight / 2, screenHeight / 2, 0.1f, 100.0f, projMat);

    VertexArray = (float *)malloc(12 * sizeof(float));
    if(VertexArray!=NULL)
    { 
    VertexArray[0] = -screenWidth/2; VertexArray[1] = 0.0f;VertexArray[2] = 0.0f;
    VertexArray[3] = screenWidth/2;VertexArray[4] = 0.0f;VertexArray[5] = 0.0f;
    
    VertexArray[6] = 0.0f;VertexArray[7] = screenHeight/2;VertexArray[8] = 0;
    VertexArray[9] = 0.0f;VertexArray[10] = -screenHeight / 2;VertexArray[11] = 0;
    }

    //Preparation  of Quad , Gnerating the Quad around the Origin
    int quadWidth = quad.dimensions[0];
    int quadHeight = quad.dimensions[1];
    vertexArr_rect = (float *)malloc(18 * sizeof(float));
   
    
    if (vertexArr_rect != NULL)
    {
    vertexArr_rect[0] = -quadWidth / 2; vertexArr_rect[1] = -quadHeight / 2; vertexArr_rect[2] = 0;
    vertexArr_rect[3] = quadWidth / 2; vertexArr_rect[4] = -quadHeight / 2; vertexArr_rect[5] = 0;
    vertexArr_rect[6] = quadWidth / 2; vertexArr_rect[7] = quadHeight / 2; vertexArr_rect[8] = 0;

    vertexArr_rect[9] = -quadWidth / 2; vertexArr_rect[10] = -quadHeight / 2; vertexArr_rect[11] = 0;
    vertexArr_rect[12] = quadWidth / 2; vertexArr_rect[13] = quadHeight / 2; vertexArr_rect[14] = 0;
    vertexArr_rect[15] = -quadWidth / 2; vertexArr_rect[16] = quadHeight / 2; vertexArr_rect[17] = 0;

    }
    prepateOffsets();
   vbo=  createVBO(vertexArr_BatchRender,maxQuads*18*sizeof(float) );
   free(vertexArr_BatchRender);
}
void drawFrame()
{
    if (chageRendermode)
    {
        chageRendermode = false;
        int rMode = sceneRendermode;
        rMode++;
        if (rMode > NoramlRendering)
            rMode = 0;
        switch (rMode)
        {
        case 0:
            sceneRendermode = InstanceRendering;
            printf("Current Render mode is: InstanceRendering  \n");
            break;
        case 1:
            sceneRendermode = BatchRendering;
            printf("Current Render mode is: BatchRendering  \n");
            break;
        case 2:
            sceneRendermode = NoramlRendering;
            printf("Current Render mode is: NoramlRendering  \n");
            break;
        default:
            break;
        }
    }
    render();
}
/*******************************************
render method to render the given things.
***************************************/
void render()
{
    mat4 mv1, mvp1;
    mat4 modelMat1 = { {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };


    glm_mat4_mul(modelMat1, viewMat, mv1);
    glm_mat4_mul(projMat, mv1, mvp1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Draw the Quad's
    if (sceneRendermode == InstanceRendering)
    {
        glUseProgram(program_Instanced);
        glEnableVertexAttribArray(posLoc_Instanced);
        glUniformMatrix4fv(mvpLocInsanced, 1, GL_FALSE, mvp1);
        glUniform2fv(offsetsLocInstanced, quad.numRects, offsets_Arr);
        glUniform4f(colorLocInstanced, quad.color[0] / 255.0f, quad.color[1] / 255.0f, quad.color[2] / 255.0f, (float)(quad.alpha) / 255.0f);
        glVertexAttribPointer(posLoc_Instanced, 3, GL_FLOAT, GL_FALSE, 0, vertexArr_rect);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, quad.numRects);
        glDisableVertexAttribArray(posLoc_Instanced);
    }
    //Draw the center box:
    float axis[3] = {0,0,1};
    mat4 rotaMat= { {1, 0, 0, 0}, { 0,1,0,0 }, { 0,0,1,0 }, { 0,0,0,1 } },mvp_rot= { {1, 0, 0, 0}, { 0,1,0,0 }, { 0,0,1,0 }, { 0,0,0,1 } };
   
    

    

    glUseProgram(program_basic);

    if (sceneRendermode == BatchRendering)
    {
        //Do the Batch Render
        glEnableVertexAttribArray(posLoc_basic);
        glUniformMatrix4fv(mvpLocBasic, 1, GL_FALSE, mvp1);
        glUniform4f(colorLocBasic, quad.color[0] / 255.0f, quad.color[1] / 255.0f, quad.color[2] / 255.0f, (float)(quad.alpha) / 255.0f);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glVertexAttribPointer(posLoc_basic, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glDrawArrays(GL_TRIANGLES, 0, quad.numRects * 6);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if (sceneRendermode == NoramlRendering)
    {

        //Do the Normal Renering , 1 Draw for 1 Quad
        int j = 0;
        for (int i = 0; i < maxQuads; i++)
        {
            mat4 mvp_translate;
mat4 mat_translation = { {1, 0, 0, 0}, { 0,1,0,0 }, { 0,0,1,0 }, { 0,0,0,1 } };
            glm_mat4_mul(modelMat1, viewMat, mv1);
            vec3 tPos = { offsets_Arr[j] ,offsets_Arr[j+1],0 };
            glm_translate(mat_translation, tPos);
            glm_mat4_mul(mv1, mat_translation, mv1);
            glm_mat4_mul(projMat, mv1, mvp_translate);

            glEnableVertexAttribArray(posLoc_basic);
            glUniformMatrix4fv(mvpLocBasic, 1, GL_FALSE, mvp_translate);
            glUniform4f(colorLocBasic, quad.color[0] / 255.0f, quad.color[1] / 255.0f, quad.color[2] / 255.0f, (float)(quad.alpha) / 255.0f);
            glVertexAttribPointer(posLoc_basic, 3, GL_FLOAT, GL_FALSE, 0, vertexArr_rect);
            glDrawArrays(GL_TRIANGLES, 0,   6);

            j = j + 2;
        }
    }

    mat4 modelMat2 = { {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };

    glm_mat4_mul(modelMat2, viewMat, mv1);
    glm_rotate(rotaMat, glm_rad(angle), axis);
    glm_mat4_mul(mv1, rotaMat, mv1);
    glm_mat4_mul(projMat, mv1, mvp_rot);

    glEnableVertexAttribArray(posLoc_basic);
    glUniformMatrix4fv(mvpLocBasic, 1, GL_FALSE, mvp_rot);
    glUniform4f(colorLocBasic, 1.0f, 0.0f, 0.0f, (float)(quad.alpha) / 255.0f);
    glVertexAttribPointer(posLoc_basic, 3, GL_FLOAT, GL_FALSE, 0, vertexArr_rect);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    angle += angleIncrement;
    
    
    //DrawAxis
    glEnableVertexAttribArray(posLoc_basic);
    glUniformMatrix4fv(mvpLocBasic, 1, GL_FALSE, mvp1);
    glUniform4f(colorLocBasic, 1.0f, 0.0f, 0.0f, 1.0f);
    glVertexAttribPointer(posLoc_basic, 3, GL_FLOAT, GL_FALSE, 0, VertexArray);
   // glDrawArrays(GL_LINES, 0, 4);
    glLineWidth(5.0f);

}
/*******************************************
Clean/Free the dynamic Memory allocations done.
***************************************/
void clean()
{
    free(offsets_Arr);
    free(vertexArr_rect);
    free(VertexArray);
    free(vertexArr_BatchRender);
    glDeleteBuffers(1, vbo);
}