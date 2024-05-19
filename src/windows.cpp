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

#include "windows.h"
#include "TextEditor.h"
#include "framebuffer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "mdl.h"
#include "resources.h"
#include "util.h"
#include <filesystem>
#include <fstream>
#include <list>
#include <string>

enum {
	MISSING_COMPILER,
	MISSING_GAME,
	MISSING_PROJECTS,
	SAVE_FAILED,
	LOAD_FAILED
};

int userError = 0;
bool isAboutOpen = false;
bool isErrorOpen = false;
bool isOpenProjectOpen = false;

std::filesystem::path currentFileName;
std::filesystem::path currentModelName;
std::filesystem::path currentDirectory;
std::filesystem::path baseDirectory;
std::filesystem::path executingDirectory = std::filesystem::current_path();

namespace QuakePrism {

void DrawMenuBar() {
	if (ImGui::BeginMainMenuBar()) {

		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open")) {
				isOpenProjectOpen = true;
			}
			if (ImGui::MenuItem("Exit")) {

				SDL_Event quitEvent;
				quitEvent.type = SDL_QUIT;
				SDL_PushEvent(&quitEvent);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help")) {
			if (ImGui::MenuItem("About")) {
				isAboutOpen = true;
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Run")) {
			if (ImGui::MenuItem("Compile")) {
				// editorLayer->CompileProject();
			}
			if (ImGui::MenuItem("Run")) {
				// editorLayer->RunProject();
			}
			if (ImGui::MenuItem("Compile and Run")) {
				// editorLayer->CompileRunProject();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void DrawModelViewer(GLuint &texture_id, GLuint &RBO, GLuint &FBO) {

	ImGui::Begin("Model Viewer", nullptr, ImGuiWindowFlags_NoMove);
	ImGuiID dockspace_id = ImGui::GetID("DockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
	static bool first_time = true;
	if (first_time) {
		first_time = false;
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id);
		ImGui::DockBuilderSetNodeSize(dockspace_id,
									  ImGui::GetMainViewport()->Size);

		auto dock_id_up = ImGui::DockBuilderSplitNode(
			dockspace_id, ImGuiDir_Up, 0.8f, nullptr, &dockspace_id);
		auto dock_id_down = ImGui::DockBuilderSplitNode(
			dockspace_id, ImGuiDir_Down, 0.2f, nullptr, &dockspace_id);
		ImGui::DockBuilderDockWindow("Model View", dock_id_up);
		ImGui::DockBuilderDockWindow("Model Tools", dock_id_down);

		ImGui::DockBuilderFinish(dockspace_id);
	}
	ImGui::End();

	ImGui::Begin("Model View", nullptr, ImGuiWindowFlags_NoMove);
	const float window_width = ImGui::GetContentRegionAvail().x;
	const float window_height = ImGui::GetContentRegionAvail().y;
	static bool paused = false;

	glViewport(0, 0, window_width, window_height);
	QuakePrism::rescaleFramebuffer(window_width, window_height, RBO,
								   texture_id);
	if (!currentModelName.empty())
		ImGui::Image((ImTextureID)texture_id, ImGui::GetContentRegionAvail(),
					 ImVec2(0, 1), ImVec2(1, 0));

	// Model Viewer Controls
	static vec3_t modelAngles = {-90.0f, 0.0f, -90.0f};
	if (ImGui::IsItemHovered()) {
		ImGuiIO &io = ImGui::GetIO();
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			modelAngles[0] += io.MouseDelta.y;
			modelAngles[2] += io.MouseDelta.x;
		}
	}

	ImGui::End();

	if (!currentModelName.empty()) {

		QuakePrism::bindFramebuffer(FBO);

		MDL::cleanup();
		MDL::reshape(window_width, window_height);
		MDL::render(currentModelName, modelAngles, paused);

		QuakePrism::unbindFramebuffer();
	}

	const float animProgress =
		MDL::totalFrames == 0 ? 0.0f
							  : MDL::currentFrame / (float)MDL::totalFrames;
	ImGui::Begin("Model Tools", nullptr, ImGuiWindowFlags_NoMove);
	ImGui::TextUnformatted(currentModelName.filename().c_str());
	char buf[32];
	sprintf(buf, "%d/%d", (int)(animProgress * MDL::totalFrames),
			MDL::totalFrames);
	ImGui::ProgressBar(animProgress, ImVec2(0.0f, 0.0f), buf);
	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::Text("Animation");

	// Animation Control Buttons
	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
	if (ImGui::ImageButton((ImTextureID)QuakePrism::UI::backButton, {24, 24})) {
		if (paused)
			MDL::currentFrame--;
	}
	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
	if (ImGui::ImageButton((ImTextureID)QuakePrism::UI::playButton, {24, 24})) {
		paused = !paused;
	}
	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
	if (ImGui::ImageButton((ImTextureID)QuakePrism::UI::forwardButton,
						   {24, 24})) {
		if (paused)
			MDL::currentFrame++;
	}
	ImGui::End();
}

void DrawTextEditor(TextEditor &editor) {

	auto lang = TextEditor::LanguageDefinition::QuakeC();
	editor.SetLanguageDefinition(lang);

	auto cpos = editor.GetCursorPosition();
	ImGui::Begin("QuakeC Editor", nullptr,
				 ImGuiWindowFlags_HorizontalScrollbar |
					 ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoMove);
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Save", "Ctrl-S", nullptr)) {
				auto textToSave = editor.GetText();
				std::ofstream output(currentFileName);
				if (output.is_open()) {
					output << textToSave;
					output.close();
				} else {
					isErrorOpen = true;
					userError = SAVE_FAILED;
				}
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit")) {
			bool ro = editor.IsReadOnly();
			if (ImGui::MenuItem("Read-Only", nullptr, &ro))
				editor.SetReadOnly(ro);

			ImGui::Separator();

			if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr,
								!ro && editor.CanUndo()))
				editor.Undo();
			if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr,
								!ro && editor.CanRedo()))
				editor.Redo();

			ImGui::Separator();

			if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr,
								editor.HasSelection()))
				editor.Copy();
			if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr,
								!ro && editor.HasSelection()))
				editor.Cut();
			if (ImGui::MenuItem("Delete", "Del", nullptr,
								!ro && editor.HasSelection()))
				editor.Delete();
			if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr,
								!ro && ImGui::GetClipboardText() != nullptr))
				editor.Paste();

			ImGui::Separator();
			if (ImGui::MenuItem("Select all", nullptr, nullptr))
				editor.SetSelection(
					TextEditor::Coordinates(),
					TextEditor::Coordinates(editor.GetTotalLines(), 0));

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			if (ImGui::MenuItem("Dark Mode"))
				editor.SetPalette(TextEditor::GetDarkPalette());
			if (ImGui::MenuItem("Light Mode"))
				editor.SetPalette(TextEditor::GetLightPalette());
			if (ImGui::MenuItem("Retro Mode"))
				editor.SetPalette(TextEditor::GetRetroBluePalette());
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s ", cpos.mLine + 1,
				cpos.mColumn + 1, editor.GetTotalLines(),
				editor.IsOverwrite() ? "Ovr" : "Ins",
				editor.CanUndo() ? "*" : " ",
				editor.GetLanguageDefinition().mName.c_str());

	editor.Render("TextEditor");
	ImGui::End();
}

void DrawFileTree(const std::filesystem::path &currentPath,
				  TextEditor &editor) {
	if (!currentPath.empty()) {
		std::list<std::filesystem::directory_entry> entryList;

		// Theres probabaly a better way to do this without iterating 3
		// times but I dont know it rn
		for (auto &directoryEntry :
			 std::filesystem::directory_iterator(currentPath)) {
			if (directoryEntry.is_directory())
				entryList.push_back(directoryEntry);
		}

		for (auto &directoryEntry :
			 std::filesystem::directory_iterator(currentPath)) {
			if (!directoryEntry.is_directory())
				entryList.push_back(directoryEntry);
		}

		for (auto &directoryEntry : entryList) {
			const auto &path = directoryEntry.path();
			std::string filenameString = path.filename().string();

			ImGui::PushID(filenameString.c_str());
			GLuint icon;

			if (directoryEntry.is_directory()) {

				icon = QuakePrism::UI::directoryIcon;
			} else {
				icon = QuakePrism::UI::fileIcon;
			}

			if (QuakePrism::ImageTreeNode(filenameString.c_str(), icon)) {
				if (ImGui::IsItemHovered() &&
					ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					if (path.extension() == ".qc") {
						std::ifstream input(path);
						if (input.good()) {
							currentFileName = path;
							std::string str(
								(std::istreambuf_iterator<char>(input)),
								std::istreambuf_iterator<char>());
							editor.SetText(str);
						} else {
							isErrorOpen = true;
							userError = LOAD_FAILED;
						}

						input.close();
					}
					if (path.extension() == ".mdl") {
						// This only exists to validate
						// the file actual loading is in
						// the render function in the
						// mdl file
						std::ifstream input(path);
						if (input.good()) {
							currentModelName = path;
							MDL::currentFrame = 0;
						} else {
							isErrorOpen = true;
							userError = LOAD_FAILED;
						}
					}
				}
				if (directoryEntry.is_directory()) {
					DrawFileTree(directoryEntry.path(), editor);
				}
				ImGui::TreePop();
			}

			ImGui::PopID();
		}
	}
}

void DrawFileExplorer(TextEditor &editor) {

	ImGui::Begin("Project Browser", nullptr, ImGuiWindowFlags_NoMove);
	if (baseDirectory.empty()) {
		if (ImGui::Button("New Project")) {
		}
		if (ImGui::Button("Open Project")) {
			isOpenProjectOpen = true;
		}
	}
	if (!baseDirectory.empty()) {
		DrawFileTree(baseDirectory, editor);
	}

	ImGui::End();
}

void DrawOpenProjectPopup() {
	if (!isOpenProjectOpen)
		return;

	ImGui::OpenPopup("Open Project");
	isOpenProjectOpen = ImGui::BeginPopupModal(
		"Open Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	std::vector<std::filesystem::directory_entry> projectList;
	if (isOpenProjectOpen) {
		try {
			for (auto &directoryEntry : std::filesystem::directory_iterator(
					 executingDirectory / "projects")) {
				if (directoryEntry.is_directory())
					projectList.push_back(directoryEntry);
			}
		} catch (const std::filesystem::filesystem_error &ex) {
			isErrorOpen = true;
			isOpenProjectOpen = false;
			userError = MISSING_PROJECTS;
		}

		static int item_current_idx = 0;
		if (ImGui::BeginListBox("Projects")) {
			for (int n = 0; n < projectList.size(); n++) {
				const bool is_selected = (item_current_idx == n);
				if (ImGui::Selectable(
						projectList.at(n).path().filename().string().c_str(),
						is_selected))
					item_current_idx = n;

				// Set the initial focus when opening the combo
				// (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndListBox();
		}

		if (ImGui::Button("Open")) {
			baseDirectory = projectList.at(item_current_idx).path();
			currentDirectory = projectList.at(item_current_idx).path();
			isOpenProjectOpen = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void DrawAboutPopup() {

	if (!isAboutOpen)
		return;

	GLuint icon;
	int width, height;

	ImGui::OpenPopup("About");
	isAboutOpen = ImGui::BeginPopupModal("About", nullptr,
										 ImGuiWindowFlags_AlwaysAutoResize);
	if (isAboutOpen) {
		QuakePrism::LoadTextureFromFile("res/prism_small.png", &icon, &width,
										&height);
		ImGui::Image((ImTextureID)icon, {48, 48});

		ImGui::SameLine();

		ImGui::BeginGroup();
		ImGui::Text("Quake Prism Development Toolkit");
		ImGui::Text("by Bance.");
		ImGui::EndGroup();

		if (ImGui::Button("Close")) {
			isAboutOpen = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void DrawErrorPopup() {
	if (!isErrorOpen)
		return;

	GLuint icon;
	int width, height;

	ImGui::OpenPopup("Error");
	isErrorOpen = ImGui::BeginPopupModal("Error", nullptr,
										 ImGuiWindowFlags_AlwaysAutoResize);
	if (isErrorOpen) {
		QuakePrism::LoadTextureFromFile("res/prism_small.png", &icon, &width,
										&height);
		ImGui::Image((ImTextureID)icon, {48, 48});

		ImGui::SameLine();
		switch (userError) {
		case MISSING_COMPILER:
			ImGui::BeginGroup();
			ImGui::Text("Error: Unable to compile progs.");
			ImGui::Text("fteqcc64 executable is missing in src.");
			ImGui::EndGroup();
			break;
		case MISSING_GAME:
			ImGui::BeginGroup();
			ImGui::Text("Error: Unable to launch quake.");
			ImGui::Text("Quake engine executable is missing.");
			ImGui::EndGroup();
			break;
		case MISSING_PROJECTS:
			ImGui::BeginGroup();
			ImGui::Text("Error: Unable to load project.");
			ImGui::Text("Projects directory is missing.");
			ImGui::EndGroup();
			break;
		case SAVE_FAILED:
			ImGui::BeginGroup();
			ImGui::Text("Error: Unable to save to output file.");
			ImGui::EndGroup();
			break;
		case LOAD_FAILED:
			ImGui::BeginGroup();
			ImGui::Text("Error: Unable to load file.");
			ImGui::EndGroup();
			break;
		default:
			break;
		}

		if (ImGui::Button("Close")) {
			isErrorOpen = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}
} // namespace QuakePrism
