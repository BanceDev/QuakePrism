#pragma once

#include <GL/glew.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

// Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
// global defined indices for OpenGL
GLuint VAO; // vertex array object
GLuint VBO; // vertex buffer object
GLuint FBO; // frame buffer object
GLuint RBO; // rendering buffer object
GLuint texture_id; // the texture id we'll need later to create a texture 

// Starts up SDL, creates window, and initializes OpenGL
bool init();

// Initializes matrices and clear color
bool initGL();

// The window we'll be rendering to
SDL_Window *gWindow = NULL;

// OpenGL context
SDL_GLContext gContext;

// The surface contained by the window
SDL_Surface *gScreenSurface = NULL;

// The image we will load and show on the screen
SDL_Surface *gHelloWorld = NULL;


