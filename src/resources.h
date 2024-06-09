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
// Font Assets
extern ImFont *ubuntuMonoFont;
extern ImFont *ubuntuFont;

// Image Assets
extern GLuint fileIcon;
extern GLuint directoryIcon;
extern GLuint modelIcon;
extern GLuint imageIcon;
extern GLuint playButton;
extern GLuint forwardButton;
extern GLuint backButton;
extern GLuint appIcon;

extern GLuint newCard;
extern GLuint importCard;
extern GLuint sharewareCard;
extern GLuint libreCard;

void loadIcons();

void loadFonts();

bool configFound();

} // namespace QuakePrism::UI
