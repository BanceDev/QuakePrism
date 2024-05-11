#pragma once

#include <SDL.h>
#include <SDL_opengl.h>
#include <GL\GLU.h>
#include <stdio.h>


struct mdl_header_t;

struct mdl_skin_t;

struct mdl_texcoord_t;

struct mdl_triangle_t;

struct mdl_vertex_t;

struct mdl_simpleframe_t;

struct mdl_frame_t;

struct mdl_model_t;

GLuint MakeTextureFromSkin(int n, const struct mdl_model_t *mdl);

int ReadMDLModel(const char *filename, struct mdl_model_t *mdl);

void FreeModel(struct mdl_model_t *mdl);

void RenderFrame(int n, const struct mdl_model_t *mdl);

void RenderFrameItp(int n, float interp, const struct mdl_model_t *mdl);

void Animate(int start, int end, int *frame, float *interp);

void cleanup();

void reshape(int w, int h);

void render();