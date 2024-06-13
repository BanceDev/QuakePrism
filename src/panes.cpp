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

#include "panes.h"
#include "TextEditor.h"
#include "framebuffer.h"
#include "imfilebrowser.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "mdl.h"
#include "pak.h"
#include "resources.h"
#include "util.h"
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#include <shellapi.h>
#include <windows.h>
#endif

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
bool isNewProjectOpen = false;
bool isLauncherOpen = true;

namespace QuakePrism {

void DrawMenuBar() {
	if (ImGui::BeginMainMenuBar()) {

		if (ImGui::BeginMenu("File")) {
			const bool newEnabled = !baseDirectory.empty();
			if (ImGui::BeginMenu("New")) {
				if (ImGui::BeginMenu("QC File", newEnabled)) {
					static char filename[32] = "";
					ImGui::InputText("file name", filename,
									 IM_ARRAYSIZE(filename));
					if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
						CreateFile(filename);
						filename[0] = '\0';
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Src Folder", newEnabled)) {
					static char dirname[32] = "";
					ImGui::InputText("folder name", dirname,
									 IM_ARRAYSIZE(dirname));
					if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
						CreateFolder(dirname);
						dirname[0] = '\0';
					}
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Project")) {
					isNewProjectOpen = true;
				}
				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Open")) {
				isOpenProjectOpen = true;
			}
			if (ImGui::MenuItem("Containing Folder") &&
				!baseDirectory.empty()) {
#ifdef _WIN32
				ShellExecute(NULL, L"open", baseDirectory.c_str(), NULL, NULL,
							 SW_SHOWDEFAULT);
#else
				std::string command = "xdg-open " + baseDirectory.string();
				system(command.c_str());
#endif
			}
			if (ImGui::MenuItem("Exit")) {

				SDL_Event quitEvent;
				quitEvent.type = SDL_QUIT;
				SDL_PushEvent(&quitEvent);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Run")) {
			if (ImGui::MenuItem("Compile")) {
				CompileProject();
			}
			if (ImGui::MenuItem("Run")) {
				RunProject();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help")) {
			if (ImGui::MenuItem("Documentation")) {
#ifdef _WIN32
				ShellExecute(0, 0,
							 L"https://github.com/BanceDev/QuakePrism/blob/"
							 L"main/docs/MANUAL.md",
							 0, 0, SW_SHOW);
#else
				system("xdg-open "
					   "https://github.com/BanceDev/QuakePrism/blob/main/docs/"
					   "MANUAL.md");
#endif
			}
			if (ImGui::MenuItem("About")) {
				isAboutOpen = true;
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
			dockspace_id, ImGuiDir_Up, 0.7f, nullptr, &dockspace_id);
		auto dock_id_down = ImGui::DockBuilderSplitNode(
			dockspace_id, ImGuiDir_Down, 0.3f, nullptr, &dockspace_id);
		ImGui::DockBuilderDockWindow("Model View", dock_id_up);
		ImGui::DockBuilderDockWindow("Model Tools", dock_id_down);

		ImGui::DockBuilderFinish(dockspace_id);
	}
	ImGui::End();

	ImGui::Begin("Model View", nullptr, ImGuiWindowFlags_NoMove);
	const float window_width = ImGui::GetContentRegionAvail().x;
	const float window_height = ImGui::GetContentRegionAvail().y;
	const bool shouldRender = ImGui::IsItemVisible();
	static bool paused = false;
	static float renderScale = 1.0f;

	glViewport(0, 0, window_width, window_height);
	QuakePrism::rescaleFramebuffer(window_width * renderScale,
								   window_height * renderScale, RBO,
								   texture_id);
	if (!currentModelName.empty())
		ImGui::Image((ImTextureID)(intptr_t)texture_id,
					 ImGui::GetContentRegionAvail(), ImVec2(0, 1),
					 ImVec2(1, 0));

	// Model Viewer Controls
	if (ImGui::IsItemHovered()) {
		ImGuiIO &io = ImGui::GetIO();
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			MDL::modelAngles[0] += io.MouseDelta.y;
			MDL::modelAngles[2] += io.MouseDelta.x;
		}
		if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
			MDL::modelPosition[0] += io.MouseDelta.x * 0.5f;
			MDL::modelPosition[1] -= io.MouseDelta.y * 0.5f;
		}
		MDL::modelScale += io.MouseWheel * 0.1f;
		if (MDL::modelScale <= 0) {
			MDL::modelScale = 0.1f;
		} else if (MDL::modelScale >= 3.0f) {
			MDL::modelScale = 3.0f;
		}
	}

	ImGui::End();

	const float animProgress =
		MDL::totalFrames == 0 ? 0.0f
							  : MDL::currentFrame / (float)MDL::totalFrames;
	ImGui::Begin("Model Tools", nullptr, ImGuiWindowFlags_NoMove);
	char buf[32];
	sprintf(buf, "%d/%d", (int)(animProgress * MDL::totalFrames),
			MDL::totalFrames);
	ImGui::Columns(2, "tools");
	ImGui::ProgressBar(animProgress, ImVec2(0.0f, 0.0f), buf);
	// Animation Control Buttons
	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::PushButtonRepeat(true);
	if (ImGui::ImageButton((ImTextureID)(intptr_t)backButton, {24, 24})) {
		if (paused && MDL::currentFrame > 0)
			MDL::currentFrame--;
	}
	ImGui::PopButtonRepeat();
	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
	if (ImGui::ImageButton((ImTextureID)(intptr_t)playButton, {24, 24})) {
		paused = !paused;
	}
	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::PushButtonRepeat(true);
	if (ImGui::ImageButton((ImTextureID)(intptr_t)forwardButton, {24, 24})) {
		if (paused) {
			if (MDL::currentFrame < MDL::totalFrames) {
				MDL::currentFrame++;
			} else {
				MDL::currentFrame = 0;
			}
		}
	}
	ImGui::PopButtonRepeat();

	enum RenderFraction { EIGTH, FOURTH, HALF, ONE, COUNT };
	static int sliderPos = ONE;
	const char *renderScaleLabels[] = {"1/8", "1/4", "1/2", "1"};
	const float renderScaleOptions[] = {0.125f, 0.25f, 0.5f, 1.0f};
	const char *renderScaleText = (sliderPos >= 0 && sliderPos < COUNT)
									  ? renderScaleLabels[sliderPos]
									  : "Unknown";

	if (ImGui::SliderInt("Render Scale", &sliderPos, 0, COUNT - 1,
						 renderScaleText)) {
		renderScale = renderScaleOptions[sliderPos];
	}

	static int textureMode = MDL::TEXTURED_MODE;
	const char *textureModes[] = {"Textured", "Textureless", "Wireframe"};
	ImGui::Combo("Texture Mode", &textureMode, textureModes,
				 IM_ARRAYSIZE(textureModes));

	ImGui::NextColumn();

	ImGui::SliderInt("skin", &MDL::currentSkin, 1, MDL::totalSkins);

	static bool lerpEnabled = true;
	ImGui::Checkbox("Interpolation", &lerpEnabled);

	static bool filteringEnabled = true;
	ImGui::Checkbox("Texture Filtering", &filteringEnabled);

	// File browser is for import texture
	static ImGui::FileBrowser texImportBrowser;
	texImportBrowser.SetTitle("Select Texture");
	texImportBrowser.SetTypeFilters({".tga"});
	if (!texImportBrowser.IsOpened())
		texImportBrowser.SetPwd(baseDirectory);

	// Texture Export Button
	if (ImGui::Button("Export Texture")) {
		if (!MDL::mdlTextureExport(currentModelName)) {
			isErrorOpen = true;
			userError = SAVE_FAILED;
		}
	}
	if (ImGui::Button("Import Texture")) {
		texImportBrowser.Open();
	}

	ImGui::End();

	texImportBrowser.Display();
	if (texImportBrowser.HasSelected()) {
		std::filesystem::path texturePath = texImportBrowser.GetSelected();
		if (!MDL::mdlTextureImport(texturePath, currentModelName)) {
			isErrorOpen = true;
			userError = LOAD_FAILED;
		}
		texImportBrowser.ClearSelected();
	}

	// Handle all of the model rendering after the UI is complete
	if (!currentModelName.empty() && shouldRender) {

		QuakePrism::bindFramebuffer(FBO);

		MDL::cleanup();
		MDL::reshape(window_width * renderScale, window_height * renderScale);
		MDL::render(currentModelName, textureMode, paused, lerpEnabled,
					filteringEnabled);

		QuakePrism::unbindFramebuffer();
	}
}

void DrawTextureViewer() {
	ImGui::Begin("Texture Viewer", nullptr, ImGuiWindowFlags_NoMove);
	if (!currentTextureName.empty()) {
		GLuint texture;
		int width, height;
		LoadTextureFromFile(currentTextureName.string().c_str(), &texture,
							&width, &height);
		ImGui::Image(
			(ImTextureID)(intptr_t)texture,
			ImVec2(ImGui::GetContentRegionAvail().x,
				   ImGui::GetContentRegionAvail().x * (height / (float)width)));
	}
	ImGui::End();
}

void SaveQuakeCFile(std::string textToSave) {
	// had issue with extra whitespace so this cleans that
	size_t end = textToSave.find_last_not_of(" \t\n\r");
	if (end == std::string::npos) {
		textToSave = ""; // All characters are whitespace or newlines
	}
	textToSave = textToSave.substr(0, end + 1);
	std::ofstream output(currentQCFileName);
	if (output.is_open()) {
		output << textToSave;
		output.close();
	} else {
		isErrorOpen = true;
		userError = SAVE_FAILED;
	}
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
				std::string textToSave = editor.GetText();
				SaveQuakeCFile(textToSave);
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
				editor.IsUnsaved() ? "*" : " ",
				editor.GetLanguageDefinition().mName.c_str());

	editor.Render("TextEditor");
	ImGui::End();
}

bool AlphabeticalComparator(const std::filesystem::directory_entry &a,
							const std::filesystem::directory_entry &b) {
	return a.path().filename().string() < b.path().filename().string();
}

void DrawFileTree(const std::filesystem::path &currentPath,
				  TextEditor &editor) {
	if (!currentPath.empty()) {
		std::vector<std::filesystem::directory_entry> directoryEntries;

		// Collect all entries (both directories and files) in the current path
		for (auto &directoryEntry :
			 std::filesystem::directory_iterator(currentPath)) {
			directoryEntries.push_back(directoryEntry);
		}

		// Sort entries alphabetically
		std::sort(directoryEntries.begin(), directoryEntries.end(),
				  AlphabeticalComparator);

		// Separate directories and files
		std::vector<std::filesystem::directory_entry> directories;
		std::vector<std::filesystem::directory_entry> files;

		for (const auto &entry : directoryEntries) {
			if (entry.is_directory()) {
				directories.push_back(entry);
			} else {
				files.push_back(entry);
			}
		}

		// Merge directories and files back into the entryList
		directoryEntries.clear();
		directoryEntries.insert(directoryEntries.end(), directories.begin(),
								directories.end());
		directoryEntries.insert(directoryEntries.end(), files.begin(),
								files.end());

		for (auto &directoryEntry : directoryEntries) {
			const auto &path = directoryEntry.path();
			std::string filenameString = path.filename().string();

			ImGui::PushID(filenameString.c_str());
			GLuint icon;

			if (directoryEntry.is_directory()) {
				icon = directoryIcon;
			} else {
				if (directoryEntry.path().extension() == ".mdl") {
					icon = modelIcon;
				} else if (directoryEntry.path().extension() == ".tga") {
					icon = imageIcon;
				} else {
					icon = fileIcon;
				}
			}

			bool node_open =
				QuakePrism::ImageTreeNode(filenameString.c_str(), icon);

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				ImGui::OpenPopup("ContextMenu");
			}

			if (ImGui::BeginPopup("ContextMenu")) {
				if (ImGui::BeginMenu("Rename")) {
					static char rename[32] = "";
					ImGui::InputText("##rename", rename, IM_ARRAYSIZE(rename));
					if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
						// Extract the original file extension
						std::string originalExtension =
							path.extension().string();
						// Append the original file extension to the new name
						std::filesystem::path newPath =
							path.parent_path() /
							(std::string(rename) + originalExtension);
						// Perform rename operation
						std::filesystem::rename(path, newPath);
						// Clear rename input
						rename[0] = '\0';
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Delete")) {
					std::filesystem::remove(path);
				}
				ImGui::EndPopup();
			}

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				// Handle file loading based on extension
				if (path.extension() == ".qc" || path.extension() == ".src" ||
					path.extension() == ".rc" || path.extension() == ".cfg") {
					std::ifstream input(path);
					if (input.good()) {
						currentQCFileName = path;
						std::string str((std::istreambuf_iterator<char>(input)),
										std::istreambuf_iterator<char>());
						editor.SetText(str);
					} else {
						isErrorOpen = true;
						userError = LOAD_FAILED;
					}
					input.close();
					ImGui::SetWindowFocus("QuakeC Editor");
				} else if (path.extension() == ".mdl") {
					std::ifstream input(path);
					if (input.good()) {
						currentModelName = path;
						MDL::currentFrame = 0;
						MDL::currentSkin = 1;
						MDL::modelAngles[0] = -90.0f;
						MDL::modelAngles[1] = 0.0f;
						MDL::modelAngles[2] = -90.0f;
						MDL::modelPosition[0] = 0.0f;
						MDL::modelPosition[1] = 0.0f;
						MDL::modelPosition[2] = -100.0f;
						MDL::modelScale = 1.0f;
					} else {
						isErrorOpen = true;
						userError = LOAD_FAILED;
					}
					ImGui::SetWindowFocus("Model Viewer");
				} else if (path.extension() == ".tga") {
					std::ifstream input(path);
					if (input.good()) {
						currentTextureName = path;
					} else {
						isErrorOpen = true;
						userError = LOAD_FAILED;
					}
					ImGui::SetWindowFocus("Texture Viewer");
				}
			}

			if (node_open) {
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
			isNewProjectOpen = true;
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
			for (auto &directoryEntry :
				 std::filesystem::directory_iterator(projectsDirectory)) {
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
		if (ImGui::Button("Cancel")) {
			isOpenProjectOpen = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
		if (ImGui::Button("Open")) {
			try {
				baseDirectory = projectList.at(item_current_idx).path();
				currentQCFileName.clear();
				currentModelName.clear();
				currentTextureName.clear();
			} catch (const std::out_of_range) {
				// Go to new project Menu
			}
			isOpenProjectOpen = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

static bool CopyTemplate(const std::filesystem::path &source,
						 const char *projectName) {
	std::filesystem::path destination = projectsDirectory / projectName;
	try {
		// Check if the source directory exists
		if (!std::filesystem::exists(source) ||
			!std::filesystem::is_directory(source)) {
			return false;
		}

		// Create the destination directory if it does not exist
		if (!std::filesystem::exists(destination)) {
			std::filesystem::create_directory(destination);
		}

		// Iterate over the source directory and copy its contents
		for (const auto &entry :
			 std::filesystem::recursive_directory_iterator(source)) {
			const auto &path = entry.path();
			auto relative_path = std::filesystem::relative(path, source);
			auto dest = destination / relative_path;

			if (std::filesystem::is_directory(path)) {
				std::filesystem::create_directories(dest);
			} else if (std::filesystem::is_regular_file(path) ||
					   std::filesystem::is_symlink(path)) {
				std::filesystem::copy(
					path, dest,
					std::filesystem::copy_options::overwrite_existing);
			}
		}

	} catch (const std::filesystem::filesystem_error &e) {
		return false;
	} catch (const std::exception &e) {
		return false;
	}

	return true;
}

void DrawNewProjectPopup() {
	if (!isNewProjectOpen)
		return;

	// These variables only used by the second case
	static std::vector<std::filesystem::path> paks;
	static int codebaseType = 0;

	ImGui::OpenPopup("New Project");
	isNewProjectOpen = ImGui::BeginPopupModal(
		"New Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	if (isNewProjectOpen) {
		static int projectType = 0;
		if (ImGui::BeginTable("table1", 2)) {
			ImGui::TableSetupColumn("Column1", ImGuiTableColumnFlags_WidthFixed,
									300.0f);
			ImGui::TableSetupColumn("Column2",
									ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Templates");
			if (ImGui::BeginTable("table2", 2)) {

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				HelpMarker(
					"Template for a bare minimum Quake project,\nideal for "
					"original games");
				if (ImGui::ImageButton((ImTextureID)(intptr_t)newCard,
									   ImVec2(128, 160))) {
					projectType = 1;
				}
				ImGui::TableSetColumnIndex(1);
				HelpMarker("Template for modding Quake or a Quake mod using\n"
						   "pak files.");
				if (ImGui::ImageButton((ImTextureID)(intptr_t)importCard,
									   ImVec2(128, 160))) {
					projectType = 2;
				}

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				HelpMarker("Template for modding Libre Quake");
				if (ImGui::ImageButton((ImTextureID)(intptr_t)libreCard,
									   ImVec2(128, 160))) {
					projectType = 3;
				}

				ImGui::EndTable();
			}
			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted("Settings: ");
			ImGui::SameLine();
			const std::string projectTypeList[4] = {
				"None Selected", "Blank Game", "Import PAK", "LibreQuake"};
			ImGui::TextUnformatted(projectTypeList[projectType].c_str());
			ImGui::Dummy(ImVec2(1, 20));
			static char projectName[32] = "";
			ImGui::TextUnformatted("Project Name");
			ImGui::SetNextItemWidth(360.0f);
			ImGui::InputText("##projectname", projectName,
							 IM_ARRAYSIZE(projectName));

			if (ImGui::Button("Make Project") && strcmp(projectName, "") != 0) {
				switch (projectType) {
				case 1: {
					std::filesystem::path blankDir =
						executingDirectory / ".res/templates/Blank";
					if (!CopyTemplate(blankDir, projectName)) {
						userError = MISSING_PROJECTS;
						isErrorOpen = true;
					}
					break;
				}
				case 2: {
					std::filesystem::path projectPath =
						projectsDirectory / projectName;

					// Create the destination directory if it does not exist
					if (!std::filesystem::exists(projectPath)) {
						std::filesystem::create_directory(projectPath);
					}

					for (const auto &pak : paks) {
						if (!PAK::ExtractPAK(pak, &projectPath)) {
							userError = MISSING_PROJECTS;
							isErrorOpen = true;
							break;
						}
					}

					if (!std::filesystem::exists(projectPath / "src")) {
						std::filesystem::create_directory(projectPath / "src");
					}

					std::string srcDir = projectName;
					srcDir += "/src";
					if (codebaseType == 0) {
						std::filesystem::path quakeCodebase =
							executingDirectory / ".res/templates/Id1/src";
						if (!CopyTemplate(quakeCodebase, srcDir.c_str())) {
							userError = MISSING_PROJECTS;
							isErrorOpen = true;
						}
					} else {
						std::filesystem::path blankCodebase =
							executingDirectory / ".res/templates/Blank/src";
						if (!CopyTemplate(blankCodebase, srcDir.c_str())) {
							userError = MISSING_PROJECTS;
							isErrorOpen = true;
						}
					}

					break;
				}
				case 3: {
					std::filesystem::path libreQuakeDir =
						executingDirectory / ".res/templates/LibreQuake";
					if (!CopyTemplate(libreQuakeDir, projectName)) {
						userError = MISSING_PROJECTS;
						isErrorOpen = true;
					}
					break;
				}
				default:
					break;
				}

				baseDirectory = projectsDirectory / projectName;
				currentQCFileName.clear();
				currentModelName.clear();
				currentTextureName.clear();
				projectName[0] = '\0';

				isNewProjectOpen = false;
				paks.clear();
				ImGui::CloseCurrentPopup();
			}

			static ImGui::FileBrowser pakImportBrowser;
			pakImportBrowser.SetTitle("Import PAK");
			pakImportBrowser.SetTypeFilters({".pak", ".PAK"});
			if (projectType == 2) { // Import PAK Type
				ImGui::TextUnformatted("Select QuakeC codebase:");
				const char *items[] = {"Quake", "Blank"};
				ImGui::Combo("##codebasetype", &codebaseType, items,
							 IM_ARRAYSIZE(items));

				if (ImGui::Button("Add PAK File")) {
					pakImportBrowser.Open();
				}

				pakImportBrowser.Display();
				if (pakImportBrowser.HasSelected()) {
					paks.push_back(pakImportBrowser.GetSelected());
					pakImportBrowser.ClearSelected();
				}
				ImGui::Dummy(ImVec2(1, 20));
				ImGui::TextUnformatted("Loaded pak files:");
				for (const auto &pak : paks) {
					ImGui::TextUnformatted(pak.filename().string().c_str());
				}
			}

			ImGui::EndTable();
		}

		if (ImGui::Button("Cancel")) {
			paks.clear();
			isNewProjectOpen = false;
			ImGui::CloseCurrentPopup();
		}
	}
	ImGui::EndPopup();
}

void DrawLauncherPopup() {
	if (!isLauncherOpen)
		return;

	if (configFound()) {
		isLauncherOpen = false;
		return;
	}

	ImGui::OpenPopup("Welcome to QuakePrism");
	isLauncherOpen = ImGui::BeginPopupModal("Welcome to QuakePrism", nullptr,
											ImGuiWindowFlags_AlwaysAutoResize);

	if (isLauncherOpen) {
		ImGui::Image((ImTextureID)(intptr_t)appIcon, {48, 48});
		ImGui::SameLine();

		ImGui::BeginGroup();
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text("Welcome to QuakePrism");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::Text(
			"To get started choose a location\n for the projects directory");
		ImGui::EndGroup();
		static ImGui::FileBrowser projectBrowser(
			ImGuiFileBrowserFlags_SelectDirectory);
		projectBrowser.SetTitle("Select Location For Projects Folder");

		if (ButtonCentered("Ok")) {
			projectBrowser.Open();
		}

		projectBrowser.Display();
		if (projectBrowser.HasSelected()) {
			projectsDirectory = projectBrowser.GetSelected() / "projects";
			std::ofstream out("quakeprism.cfg");
			out << projectsDirectory.string();
			out.close();
			// Create the projects directory if it does not exist
			if (!std::filesystem::exists(projectsDirectory)) {
				std::filesystem::create_directory(projectsDirectory);
			}

			// Copy the correct executable into the projects directory
#ifdef _WIN32
			for (const auto &entry :
				 std::filesystem::recursive_directory_iterator(
					 executingDirectory / ".res/templates/Windows")) {
#else
			for (const auto &entry :
				 std::filesystem::recursive_directory_iterator(
					 executingDirectory / ".res/templates/Linux")) {
#endif
				const auto &path = entry.path();
#ifdef _WIN32
				auto relative_path = std::filesystem::relative(
					path, (executingDirectory / ".res/templates/Windows"));
#else
				auto relative_path = std::filesystem::relative(
					path, (executingDirectory / ".res/templates/Linux"));
#endif
				auto dest = projectsDirectory / relative_path;

				if (std::filesystem::is_directory(path)) {
					std::filesystem::create_directories(dest);
				} else if (std::filesystem::is_regular_file(path) ||
						   std::filesystem::is_symlink(path)) {
					std::filesystem::copy(
						path, dest,
						std::filesystem::copy_options::overwrite_existing);
				}
			}

			isLauncherOpen = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void DrawAboutPopup() {

	if (!isAboutOpen)
		return;

	ImGui::OpenPopup("About");
	isAboutOpen = ImGui::BeginPopupModal("About", nullptr,
										 ImGuiWindowFlags_AlwaysAutoResize);
	if (isAboutOpen) {
		ImGui::Image((ImTextureID)(intptr_t)appIcon, {48, 48});

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

	ImGui::OpenPopup("Error");
	isErrorOpen = ImGui::BeginPopupModal("Error", nullptr,
										 ImGuiWindowFlags_AlwaysAutoResize);
	if (isErrorOpen) {
		ImGui::Image((ImTextureID)(intptr_t)appIcon, {48, 48});

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
