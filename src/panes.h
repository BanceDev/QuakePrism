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
#include "TextEditor.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <filesystem>

namespace QuakePrism {

void DrawMenuBar();

void DrawModelViewer(GLuint &texture_id, GLuint &RBO, GLuint &FBO);

void DrawTextureViewer();

void DrawTextEditor(TextEditor &editor);

void DrawErrorPopup();

void DrawAboutPopup();

void DrawFileTree(const std::filesystem::path &currentPath, TextEditor &editor);

void DrawFileExplorer(TextEditor &editor);

void DrawOpenProjectPopup();

void DrawNewProjectPopup();

void DrawLauncherPopup();

void SaveQuakeCFile(std::string textToSave);

} // namespace QuakePrism
