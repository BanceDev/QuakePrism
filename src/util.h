#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

namespace QuakePrism {

bool LoadTextureFromFile(const char *filename, GLuint *out_texture,
                         int *out_width, int *out_height);

bool ImageTreeNode(const char *label, const GLuint icon);

} // namespace QuakePrism
