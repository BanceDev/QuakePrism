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
#include "imgui.h"
#include "spr.h"
#include "wad.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <filesystem>
#include <vector>

namespace QuakePrism {
// Font Assets
extern ImFont *inconsolataFont;
extern ImFont *notoSansFont;

// Colormap
extern unsigned char colormap[256][3];

// Sprite Panel Assets
extern SPR::sprite_t currentSprite;
extern int activeSpriteFrame;
extern std::vector<SPR::spriteframe_t> currentSpriteFrames;
extern std::vector<unsigned int> currentSpriteTexs;
extern std::filesystem::path currentSpritePath;

// WAD Panel Assets
extern WAD::wad_t currentWad;
extern std::vector<WAD::wadentry_t> currentWadEntries;
extern std::vector<unsigned int> currentWadTexs;
extern std::vector<WAD::waddata_t> currentWadData;
extern std::filesystem::path currentWadPath;

// Image Assets
extern GLuint fileIcon;
extern GLuint directoryIcon;
extern GLuint modelIcon;
extern GLuint imageIcon;
extern GLuint audioIcon;
extern GLuint bspIcon;
extern GLuint demoIcon;
extern GLuint lumpIcon;
extern GLuint wadIcon;
extern GLuint qcIcon;
extern GLuint exeIcon;
extern GLuint cfgIcon;
extern GLuint mapIcon;
extern GLuint playButton;
extern GLuint forwardButton;
extern GLuint backButton;
extern GLuint appIcon;

extern GLuint newCard;
extern GLuint importCard;
extern GLuint libreCard;

// Text Editor Info
extern std::vector<TextEditor> editorList;
extern bool isFindOpen;
extern std::string editorTheme;

// Config Assets
extern std::filesystem::path configFile;
extern std::vector<std::filesystem::path> projectsList;
extern std::filesystem::path projectSourcePort;

// Essential Paths
extern std::vector<std::filesystem::path> currentQCFileNames;
extern TextEditor *currentTextEditor;
extern std::filesystem::path currentModelName;
extern std::filesystem::path currentTextureName;
extern std::filesystem::path baseDirectory;
extern std::filesystem::path executingDirectory;

void loadIcons();

void loadFonts();

void loadColormap();

void CreateQProjectFile();

void UpdateQProjectFile();

void ReadQProjectFile();

} // namespace QuakePrism
