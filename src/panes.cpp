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
#include "resources.h"
#include "util.h"
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
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
bool isNewProjectOpen = false;

std::filesystem::path currentQCFileName;
std::filesystem::path currentModelName;
std::filesystem::path currentTextureName;
std::filesystem::path currentDirectory;
std::filesystem::path baseDirectory;
std::filesystem::path executingDirectory = std::filesystem::current_path();

namespace QuakePrism {

void DrawMenuBar() {
	if (ImGui::BeginMainMenuBar()) {

		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New")) {
				isNewProjectOpen = true;
			}
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

		if (ImGui::BeginMenu("Help")) {
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
	ImGui::TextUnformatted(currentModelName.filename().string().c_str());
	char buf[32];
	sprintf(buf, "%d/%d", (int)(animProgress * MDL::totalFrames),
			MDL::totalFrames);
	ImGui::Columns(2, "tools");
	ImGui::ProgressBar(animProgress, ImVec2(0.0f, 0.0f), buf);
	// Animation Control Buttons
	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::PushButtonRepeat(true);
	if (ImGui::ImageButton((ImTextureID)(intptr_t)QuakePrism::UI::backButton,
						   {24, 24})) {
		if (paused && MDL::currentFrame > 0)
			MDL::currentFrame--;
	}
	ImGui::PopButtonRepeat();
	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
	if (ImGui::ImageButton((ImTextureID)(intptr_t)QuakePrism::UI::playButton,
						   {24, 24})) {
		paused = !paused;
	}
	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::PushButtonRepeat(true);
	if (ImGui::ImageButton((ImTextureID)(intptr_t)QuakePrism::UI::forwardButton,
						   {24, 24})) {
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

	static bool lerpEnabled = true;
	ImGui::Checkbox("Interpolation", &lerpEnabled);

	static bool filteringEnabled = true;
	ImGui::Checkbox("Texture Filtering", &filteringEnabled);

	// File browser is for import texture
	static ImGui::FileBrowser texImportBrowser;
	texImportBrowser.SetTitle("Select Texture");
	texImportBrowser.SetTypeFilters({".tga"});
	if (!texImportBrowser.IsOpened())
		texImportBrowser.SetPwd(currentDirectory);

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
				std::ofstream output(currentQCFileName);
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

				icon = UI::directoryIcon;
			} else {
				if (directoryEntry.path().extension() == ".mdl") {
					icon = UI::modelIcon;
				} else if (directoryEntry.path().extension() == ".tga") {
					icon = UI::imageIcon;
				} else {
					icon = UI::fileIcon;
				}
			}

			if (QuakePrism::ImageTreeNode(filenameString.c_str(), icon)) {
				if (ImGui::IsItemClicked()) {
					if (path.extension() == ".qc" ||
						path.extension() == ".src") {
						// Now load in the new file
						std::ifstream input(path);
						if (input.good()) {
							currentQCFileName = path;
							std::string str(
								(std::istreambuf_iterator<char>(input)),
								std::istreambuf_iterator<char>());
							editor.SetText(str);
						} else {
							isErrorOpen = true;
							userError = LOAD_FAILED;
						}

						input.close();
						ImGui::SetWindowFocus("QuakeC Editor");
					}
					if (path.extension() == ".mdl") {
						/* This only exists to validate
						 the file actual loading is in
						 the render function in the
						 mdl file */
						std::ifstream input(path);
						if (input.good()) {
							currentModelName = path;
							MDL::currentFrame = 0;
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
					}
					if (path.extension() == ".tga") {
						/* This only exists to validate
						 the file actual loading is in
						 the render function in the
						 mdl file */
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
			try {
				baseDirectory = projectList.at(item_current_idx).path();
				currentDirectory = projectList.at(item_current_idx).path();
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
	std::filesystem::path destination =
		executingDirectory / "projects" / projectName;
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
				if (ImGui::ImageButton((ImTextureID)(intptr_t)UI::newCard,
									   ImVec2(128, 160))) {
					projectType = 1;
				}
				ImGui::TableSetColumnIndex(1);
				HelpMarker("Template for modding the full version of Quake,\n"
						   "requires pak1.pak");
				if (ImGui::ImageButton((ImTextureID)(intptr_t)UI::importCard,
									   ImVec2(128, 160))) {
					projectType = 2;
				}

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				HelpMarker(
					"Template for modding the shareware version of Quake");
				if (ImGui::ImageButton((ImTextureID)(intptr_t)UI::sharewareCard,
									   ImVec2(128, 160))) {
					projectType = 3;
				}
				ImGui::TableSetColumnIndex(1);
				HelpMarker("Template for modding Libre Quake");
				if (ImGui::ImageButton((ImTextureID)(intptr_t)UI::libreCard,
									   ImVec2(128, 160))) {
					projectType = 4;
				}

				ImGui::EndTable();
			}
			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted("Settings: ");
			ImGui::SameLine();
			const std::string projectTypeList[5] = {"None Selected",
													"Blank Game", "Import PAK",
													"Shareware", "LibreQuake"};
			ImGui::TextUnformatted(projectTypeList[projectType].c_str());
			ImGui::Dummy(ImVec2(1, 20));
			static char projectName[32] = "";
			ImGui::TextUnformatted("Project Name");
			ImGui::SetNextItemWidth(360.0f);
			ImGui::InputText("", projectName, IM_ARRAYSIZE(projectName));
			if (ImGui::Button("Make Project")) {
				switch (projectType) {
				case 1:
					std::filesystem::path blankDir =
						executingDirectory / "res/.templates/Blank";
					if (!CopyTemplate(blankDir, projectName)) {
						userError = MISSING_PROJECTS;
						isErrorOpen = true;
					}
					baseDirectory =
						executingDirectory / "projects" / projectName;
					currentDirectory = baseDirectory;
					currentQCFileName.clear();
					currentModelName.clear();
					currentTextureName.clear();

					isNewProjectOpen = false;
					ImGui::CloseCurrentPopup();

					break;
				case 2:
					break;
				case 3: {
					std::filesystem::path sharewareDir =
						executingDirectory / "res/.templates/Shareware";
					if (!CopyTemplate(sharewareDir, projectName)) {
						userError = MISSING_PROJECTS;
						isErrorOpen = true;
					}
					baseDirectory =
						executingDirectory / "projects" / projectName;
					currentDirectory = baseDirectory;
					currentQCFileName.clear();
					currentModelName.clear();
					currentTextureName.clear();

					isNewProjectOpen = false;
					ImGui::CloseCurrentPopup();
					break;
				}
				case 4: {
					std::filesystem::path libreQuakeDir =
						executingDirectory / "res/.templates/LibreQuake";
					if (!CopyTemplate(libreQuakeDir, projectName)) {
						userError = MISSING_PROJECTS;
						isErrorOpen = true;
					}
					baseDirectory =
						executingDirectory / "projects" / projectName;
					currentDirectory = baseDirectory;
					currentQCFileName.clear();
					currentModelName.clear();
					currentTextureName.clear();

					isNewProjectOpen = false;
					ImGui::CloseCurrentPopup();
					break;
				}
				default:
					break;
				}
			}

			static ImGui::FileBrowser pakImportBrowser;
			pakImportBrowser.SetTitle("Import PAK");
			pakImportBrowser.SetTypeFilters({".pak", ".PAK"});
			if (projectType == 2) { // Import PAK Type
				ImGui::Dummy(ImVec2(1, 138));
				if (ImGui::Button("Import PAK File")) {
					pakImportBrowser.Open();
				}

				pakImportBrowser.Display();
				static std::filesystem::path pakPath;
				if (pakImportBrowser.HasSelected()) {
					pakPath = pakImportBrowser.GetSelected();
					pakImportBrowser.ClearSelected();
				}

				ImGui::Dummy(ImVec2(1, 20));
				ImGui::TextUnformatted(pakPath.filename().c_str());
			}

			ImGui::EndTable();
		}

		if (ImGui::Button("Close")) {
			isNewProjectOpen = false;
			ImGui::CloseCurrentPopup();
		}
	}
	ImGui::EndPopup();
}

void DrawAboutPopup() {

	if (!isAboutOpen)
		return;

	ImGui::OpenPopup("About");
	isAboutOpen = ImGui::BeginPopupModal("About", nullptr,
										 ImGuiWindowFlags_AlwaysAutoResize);
	if (isAboutOpen) {
		ImGui::Image((ImTextureID)(intptr_t)QuakePrism::UI::appIcon, {48, 48});

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
		ImGui::Image((ImTextureID)(intptr_t)QuakePrism::UI::appIcon, {48, 48});

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
