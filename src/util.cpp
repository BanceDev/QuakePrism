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

#include "util.h"
#include "imgui.h"
#include "resources.h"
#include "stb_image.h"
#include <cstdint>
#include <fstream>
#include <ostream>
#include <string>
#include <unistd.h>
#ifdef _WIN32
#include <direct.h>
#include <iostream>
#include <windows.h>
#endif

namespace QuakePrism {

// Simple helper function to load an image into a OpenGL texture with common
// settings
bool LoadTextureFromFile(const char *filename, GLuint *out_texture,
						 int *out_width, int *out_height) {
	// Load from file
	int image_width = 0;
	int image_height = 0;
	unsigned char *image_data =
		stbi_load(filename, &image_width, &image_height, NULL, 4);
	if (image_data == NULL)
		return false;

	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
					GL_CLAMP_TO_EDGE); // This is required on WebGL for non
									   // power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0,
				 GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);

	if (out_texture != nullptr)
		*out_texture = image_texture;
	if (out_width != nullptr)
		*out_width = image_width;
	if (out_height != nullptr)
		*out_height = image_height;

	return true;
}

static float colorDistance(const unsigned char *color1,
						   const unsigned char *color2) {
	return std::sqrt(std::pow(color1[0] - color2[0], 2) +
					 std::pow(color1[1] - color2[1], 2) +
					 std::pow(color1[2] - color2[2], 2));
}

static int findClosestColorIndex(const unsigned char *color) {
	float minDistance = std::numeric_limits<float>::max();
	int closestIndex = 0;

	for (int i = 0; i < 256; ++i) {
		float distance = colorDistance(color, colormap[i]);
		if (distance < minDistance) {
			minDistance = distance;
			closestIndex = i;
		}
	}

	return closestIndex;
}

void convertRGBToIndices(unsigned char *pixels, unsigned char *indices,
						 const int size) {
	for (int i = 0; i < size; ++i) {
		const unsigned char color[3] = {
			pixels[(i * 3) + 0], pixels[(i * 3) + 1], pixels[(i * 3) + 2]};
		indices[i] = findClosestColorIndex(color);
	}
}

void convertRGBAToIndices(unsigned char *pixels, unsigned char *indices,
						  const int size) {
	for (int i = 0; i < size; ++i) {
		const unsigned char color[3] = {
			pixels[(i * 4) + 0], pixels[(i * 4) + 1], pixels[(i * 4) + 2]};
		if (pixels[(i * 4) + 3] > 0) {
			indices[i] = findClosestColorIndex(color);
		} else {
			indices[i] = 255;
		}
	}
}

bool ImageTreeNode(const char *label, const GLuint icon) {
	const ImGuiStyle &style = ImGui::GetStyle();
	ImGuiStorage *storage = ImGui::GetStateStorage();

	ImU32 id = ImGui::GetID(label);
	int opened = storage->GetInt(id, 0);
	float x = ImGui::GetCursorPosX();
	ImGui::BeginGroup();
	if (ImGui::InvisibleButton(
			label,
			ImVec2(-1, ImGui::GetFontSize() + style.FramePadding.y * 2))) {
		int *p_opened = storage->GetIntRef(id, 0);
		opened = *p_opened = !*p_opened;
	}
	bool hovered = ImGui::IsItemHovered();
	bool active = ImGui::IsItemActive();
	if (hovered || active)
		ImGui::GetWindowDrawList()->AddRectFilled(
			ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
			ImColor(ImGui::GetStyle().Colors[active ? ImGuiCol_HeaderActive
													: ImGuiCol_HeaderHovered]));

	// Icon, text
	ImGui::SameLine(x);
	ImGui::ImageButton((ImTextureID)(intptr_t)icon, {24, 24}, ImVec2(0, 0),
					   ImVec2(1, 1), 0);
	ImGui::SameLine();
	ImGui::TextUnformatted(label);
	ImGui::EndGroup();
	if (opened)
		ImGui::TreePush(label);
	return opened != 0;
}

void HelpMarker(const char *desc) {
	ImGui::TextDisabled("(i)");
	if (ImGui::BeginItemTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

bool ButtonRight(const char *label, float offset_from_right = 10.0f) {
	// Get the window size and cursor position
	ImVec2 windowSize = ImGui::GetWindowSize();

	// Calculate the new cursor position for the button
	float buttonWidth =
		ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.0f;
	float newCursorX = windowSize.x - buttonWidth - offset_from_right;

	// Set the new cursor position
	ImGui::SetCursorPosX(newCursorX);

	// Create the button
	if (ImGui::Button(label)) {
		return true;
	}

	// Move to the next line
	ImGui::SameLine();

	return false;
}

bool ButtonCentered(const char *label) {
	// Get the window size and cursor position
	ImVec2 windowSize = ImGui::GetWindowSize();

	// Calculate the new cursor position for the button
	float buttonWidth =
		ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.0f;
	float newCursorX = (windowSize.x - buttonWidth) / 2.0f;

	// Set the new cursor position
	ImGui::SetCursorPosX(newCursorX);

	// Create the button
	if (ImGui::Button(label)) {
		return true;
	}

	// Move to the next line
	ImGui::SameLine();

	return false;
}

bool CompileProject() {

#ifdef _WIN32
	_chdir((baseDirectory / "src").string().c_str());
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));
	std::wstring cmd = L"fteqcc64.exe";
	bool result = CreateProcess(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE,
								CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
	_chdir(baseDirectory.string().c_str());
#else
	chdir((baseDirectory / "src").string().c_str());
	bool result = system("./fteqcc64") != -1;
	chdir(baseDirectory.string().c_str());
#endif
	return result;
}

bool RunProject() {

#ifdef _WIN32
	_chdir(baseDirectory.parent_path().string().c_str());
	std::wstring cmd = projectSourcePort.filename().wstring() + L" -game " +
					   baseDirectory.filename().wstring();
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));
	bool result = CreateProcess(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE,
								CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
	_chdir(baseDirectory.string().c_str());
#else
	chdir(baseDirectory.parent_path().string().c_str());
	std::string cmd = "./" + projectSourcePort.filename().string() + " -game " +
					  baseDirectory.filename().string();
	bool result = system(cmd.c_str()) != -1;
	chdir(baseDirectory.string().c_str());
#endif
	return result;
}

void CreateFile(const char *filename) {
	if (!std::filesystem::exists(baseDirectory / "src" / filename)) {
#ifdef _WIN32
		_chdir((baseDirectory / "src").string().c_str());
#else
		chdir((baseDirectory / "src").string().c_str());
#endif
		std::string s = filename;
		s += ".qc";
		std::ofstream{s};
	}
}

void CreateFolder(const char *dirname) {
	if (!std::filesystem::exists(baseDirectory / "src" / dirname))
		std::filesystem::create_directory(baseDirectory / "src" / dirname);
}

void AddProjectToRecents(const std::filesystem::path &projectPath) {
	std::ofstream output(configFile, std::ios::app);

	if (output.is_open()) {
		output << projectPath.string() << std::endl;
	}
}

void InitializeRecentProjectsList() {
	std::string fileContent = "";
	std::ifstream input(configFile);

	if (input.is_open()) {
		std::string line;
		while (std::getline(input, line)) {
			std::filesystem::path projectPath(line);
			if (std::filesystem::exists(projectPath)) {
				fileContent += line + "\n";
				projectsList.push_back(projectPath);
			}
		}
	}
	input.close();

	std::ofstream output(configFile);
	if (output.is_open())
		output << fileContent;
	output.close();
}

} // namespace QuakePrism
