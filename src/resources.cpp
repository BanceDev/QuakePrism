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

#include "resources.h"
#include "util.h"
#include <cstdio>
#include <fstream>
#include <string>

namespace QuakePrism {

// Fonts
ImFont *ubuntuFont;
ImFont *ubuntuMonoFont;

// Colormap
unsigned char colormap[256][3] = {
#include "colormap.h"
};

// Images
GLuint fileIcon;
GLuint directoryIcon;
GLuint modelIcon;
GLuint imageIcon;
GLuint playButton;
GLuint forwardButton;
GLuint backButton;
GLuint appIcon;

GLuint newCard;
GLuint importCard;
GLuint libreCard;

std::vector<TextEditor> editorList;

// Config Files
std::filesystem::path configFile =
	std::filesystem::current_path() / ".quakeprism.cfg";
std::vector<std::filesystem::path> projectsList;

// Essential Paths
std::vector<std::filesystem::path> currentQCFileNames;
std::filesystem::path currentModelName;
std::filesystem::path currentTextureName;
std::filesystem::path baseDirectory;
std::filesystem::path executingDirectory = std::filesystem::current_path();

void loadFonts() {
	ImGuiIO &io = ImGui::GetIO();
	(void)io;

	ubuntuFont = io.Fonts->AddFontFromFileTTF("res/Ubuntu-M.ttf", 21.0f);
	IM_ASSERT(ubuntuFont != nullptr);
	ubuntuMonoFont = io.Fonts->AddFontFromFileTTF(
		"res/UbuntuMonoNerdFontMono-Regular.ttf", 21.0f);
	IM_ASSERT(ubuntuMonoFont != nullptr);
}

void loadIcons() {
	LoadTextureFromFile("res/FileIcon.png", &fileIcon, nullptr, nullptr);
	LoadTextureFromFile("res/DirectoryIcon.png", &directoryIcon, nullptr,
						nullptr);
	LoadTextureFromFile("res/ModelIcon.png", &modelIcon, nullptr, nullptr);
	LoadTextureFromFile("res/ImageIcon.png", &imageIcon, nullptr, nullptr);
	LoadTextureFromFile("res/play.png", &playButton, nullptr, nullptr);
	LoadTextureFromFile("res/forward.png", &forwardButton, nullptr, nullptr);
	LoadTextureFromFile("res/back.png", &backButton, nullptr, nullptr);
	LoadTextureFromFile("res/prism_small.png", &appIcon, nullptr, nullptr);
	LoadTextureFromFile("res/NewCard.png", &newCard, nullptr, nullptr);
	LoadTextureFromFile("res/ImportCard.png", &importCard, nullptr, nullptr);
	LoadTextureFromFile("res/LibreCard.png", &libreCard, nullptr, nullptr);
}

void loadColormap() {
	FILE *fp;

	fp = fopen((baseDirectory / "gfx/palette.lmp").string().c_str(), "rb");
	if (!fp) {
		return;
	}

	for (int i = 0; i < 256; ++i) {
		for (int j = 0; j < 3; ++j) {
			fread(&colormap[i][j], 1, sizeof(unsigned char), fp);
		}
	}

	fclose(fp);
}
} // namespace QuakePrism
