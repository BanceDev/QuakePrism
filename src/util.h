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
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <filesystem>

namespace QuakePrism {

bool LoadTextureFromFile(const char *filename, GLuint *out_texture,
						 int *out_width, int *out_height);
void convertRGBToIndices(unsigned char *pixels, unsigned char *indices,
						 const int size);
void convertRGBAToIndices(unsigned char *pixels, unsigned char *indices,
						  const int size);
void convertRGBToImage(const char *filename, unsigned char *pixels,
					   const int width, const int height);
void convertRGBAToImage(const char *filename, unsigned char *pixels,
						const int width, const int height);

bool ImageTreeNode(const char *label, const GLuint icon);

void HelpMarker(const char *desc);

bool ButtonRight(const char *label, float offset_from_right);
bool ButtonCentered(const char *label);

bool CompileProject();
bool RunProject();

void CreateFile(const char *filename);
void CreateFolder(const char *dirname);

void AddProjectToRecents(const std::filesystem::path &projectPath);
void InitializeRecentProjectsList();

} // namespace QuakePrism
