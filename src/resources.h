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
#include "imgui.h"
#include <SDL.h>
#include <SDL_opengl.h>

namespace QuakePrism::UI {

extern ImFont *jetBrainsFont;
extern ImFont *ubuntuFont;

void loadIcons();

void loadFonts();

const GLuint getFileIcon();

const GLuint getDirectoryIcon();
} // namespace QuakePrism::UI
