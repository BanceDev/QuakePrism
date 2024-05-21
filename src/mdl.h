/*
Copyright (C) 2024 Lance Borden

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3.0
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.

*/

#pragma once

#include <GL/glew.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <filesystem>

typedef float vec3_t[3];

struct mdl_header_t;

struct mdl_skin_t;

struct mdl_texcoord_t;

struct mdl_triangle_t;

struct mdl_vertex_t;

struct mdl_simpleframe_t;

struct mdl_frame_t;

struct mdl_model_t;

namespace QuakePrism::MDL {

extern float interpAmt;
extern int currentFrame;
extern int totalFrames;
extern vec3_t modelAngles;
extern vec3_t modelPosition;
extern GLfloat modelScale;

GLuint MakeTextureFromSkin(int n, const struct mdl_model_t *mdl);

int ReadMDLModel(const char *filename, struct mdl_model_t *mdl);

void FreeModel(struct mdl_model_t *mdl);

void RenderFrame(int n, const struct mdl_model_t *mdl);

void RenderFrameItp(int n, float interp, const struct mdl_model_t *mdl);

void Animate(int start, int end, int *frame, float *interp);

void cleanup();

void reshape(int w, int h);

void render(const std::filesystem::path modelPath, const bool paused);
} // namespace QuakePrism::MDL
