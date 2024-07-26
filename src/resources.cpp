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
#include "spr.h"
#include "util.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

namespace QuakePrism {

// Fonts
ImFont *inconsolataFont;
ImFont *notoSansFont;

// Colormap
unsigned char colormap[256][3] = {
#include "colormap.h"
};

// Sprite Panel Assets
SPR::sprite_t currentSprite;
int activeSpriteFrame = 0;
std::vector<SPR::spriteframe_t> currentSpriteFrames;
std::vector<unsigned int> currentSpriteTexs;
std::filesystem::path currentSpritePath;

// WAD Panel Assets
WAD::wad_t currentWad;
std::vector<WAD::wadentry_t> currentWadEntries;
std::vector<std::vector<char>> currentWadEntryData;
std::vector<unsigned int> currentWadTexs;
std::filesystem::path currentWadPath;

// Images
GLuint fileIcon;
GLuint directoryIcon;
GLuint modelIcon;
GLuint imageIcon;
GLuint audioIcon;
GLuint bspIcon;
GLuint demoIcon;
GLuint lumpIcon;
GLuint wadIcon;
GLuint qcIcon;
GLuint exeIcon;
GLuint cfgIcon;
GLuint mapIcon;
GLuint playButton;
GLuint forwardButton;
GLuint backButton;
GLuint appIcon;

GLuint newCard;
GLuint importCard;
GLuint libreCard;

// Text Editor
std::vector<TextEditor> editorList;
bool isFindOpen = false;
std::string editorTheme = "prism-dark";

// Config Files
std::filesystem::path configFile =
	std::filesystem::current_path() / ".quakeprism.cfg";
std::vector<std::filesystem::path> projectsList;
std::filesystem::path projectSourcePort;

// Essential Paths
std::vector<std::filesystem::path> currentQCFileNames;
TextEditor *currentTextEditor = nullptr;
std::filesystem::path currentModelName;
std::filesystem::path currentTextureName;
std::filesystem::path baseDirectory;
std::filesystem::path executingDirectory = std::filesystem::current_path();

void loadFonts() {
	ImGuiIO &io = ImGui::GetIO();
	(void)io;

	notoSansFont = io.Fonts->AddFontFromFileTTF("res/NotoSans.ttf", 21.0f);
	IM_ASSERT(notoSansFont != nullptr);
	inconsolataFont =
		io.Fonts->AddFontFromFileTTF("res/Inconsolata.ttf", 21.0f);
	IM_ASSERT(inconsolataFont != nullptr);
}

void loadIcons() {
	LoadTextureFromFile("res/FileIcon.png", &fileIcon, nullptr, nullptr);
	LoadTextureFromFile("res/DirectoryIcon.png", &directoryIcon, nullptr,
						nullptr);
	LoadTextureFromFile("res/ModelIcon.png", &modelIcon, nullptr, nullptr);
	LoadTextureFromFile("res/ImageIcon.png", &imageIcon, nullptr, nullptr);
	LoadTextureFromFile("res/AudioIcon.png", &audioIcon, nullptr, nullptr);
	LoadTextureFromFile("res/BspIcon.png", &bspIcon, nullptr, nullptr);
	LoadTextureFromFile("res/DemoIcon.png", &demoIcon, nullptr, nullptr);
	LoadTextureFromFile("res/LumpIcon.png", &lumpIcon, nullptr, nullptr);
	LoadTextureFromFile("res/WadIcon.png", &wadIcon, nullptr, nullptr);
	LoadTextureFromFile("res/QcIcon.png", &qcIcon, nullptr, nullptr);
	LoadTextureFromFile("res/ExeIcon.png", &exeIcon, nullptr, nullptr);
	LoadTextureFromFile("res/CfgIcon.png", &cfgIcon, nullptr, nullptr);
	LoadTextureFromFile("res/MapIcon.png", &mapIcon, nullptr, nullptr);
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

void CreateQProjectFile() {
	if (!std::filesystem::exists(baseDirectory / ".qproj")) {
		std::ofstream output((baseDirectory / ".qproj"));
		if (output.is_open()) {
#ifdef _WIN32
			output << "quakespasm.exe" << std::endl;
#else
			output << "quakespasm" << std::endl;
#endif
			output << "prism-dark" << std::endl;
			output.close();
		}
	}
}

void UpdateQProjectFile() {
	std::ofstream output((baseDirectory / ".qproj"));
	if (output.is_open()) {
		output << projectSourcePort.filename().string() << std::endl;
		output << editorTheme << std::endl;
		output.close();
	}
}

void ReadQProjectFile() {
	std::ifstream input((baseDirectory / ".qproj"));
	if (input.is_open()) {
		std::string sourcePortName;
		std::getline(input, sourcePortName);
		projectSourcePort = baseDirectory.parent_path() / sourcePortName;
		std::getline(input, editorTheme);
		input.close();
	}
}

} // namespace QuakePrism
