#pragma once

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

namespace QuakePrism {

void createFramebuffer(GLuint &FBO, GLuint &RBO, GLuint &texture_id);

void bindFramebuffer(GLuint &FBO);

void unbindFramebuffer();

void rescaleFramebuffer(float width, float height, GLuint &RBO,
                        GLuint &texture_id);

} // namespace QuakePrism
