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
#include <cstdint>
#include <filesystem>
#include <fstream>
#define STB_IMAGE_IMPLEMENTATION
#include "resources.h"
#include "stb_image.h"
#include "unistd.h"

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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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

	chdir((baseDirectory / "src").string().c_str());
#ifdef _WIN32
	bool result = system("./fteqcc64.exe") != -1;
#else
	bool result = system("./fteqcc64") != -1;
#endif
	chdir(baseDirectory.string().c_str());
	return result;
}

bool RunProject() {
	chdir(projectsDirectory.string().c_str());
#ifdef _WIN32
	std::string cmd = "./quake.exe -game " + baseDirectory.filename().string();
	bool result = system(cmd.c_str()) != -1;
#else
	std::string cmd =
		"./quake.AppImage -game " + baseDirectory.filename().string();
	bool result = system(cmd.c_str()) != -1;
#endif
	chdir(baseDirectory.string().c_str());
	return result;
}

void CreateFile(const char *filename) {
	if (!std::filesystem::exists(baseDirectory / "src" / filename)) {
		chdir((baseDirectory / "src").string().c_str());
		std::string s = filename;
		s += ".qc";
		std::ofstream{s};
	}
}

void CreateFolder(const char *dirname) {
	if (!std::filesystem::exists(baseDirectory / "src" / dirname))
		std::filesystem::create_directory(baseDirectory / "src" / dirname);
}

} // namespace QuakePrism
