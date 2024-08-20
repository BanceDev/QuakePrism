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
#include "linter.h"
#include "lmp.h"
#include "mdl.h"
#include "pak.h"
#include "resources.h"
#include "spr.h"
#include "util.h"
#include "wad.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#include <shellapi.h>
#include <windows.h>
#endif

enum {
	MISSING_COMPILER,
	MISSING_GAME,
	PROJECT_FAILURE,
	SAVE_FAILED,
	LOAD_FAILED
};

int userError = 0;
bool isAboutOpen = false;
bool isStartupOpen = true;
bool isErrorOpen = false;
bool isOpenProjectOpen = false;
bool isNewProjectOpen = false;
bool isLauncherOpen = true;
bool isCompiling = false;
bool palLoaded = false;
bool newTabOpened = false;

namespace QuakePrism {

void DrawMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		const bool newEnabled = !baseDirectory.empty();
		if (ImGui::BeginMenu("File")) {
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
			if (ImGui::MenuItem("Containing Folder", NULL, false, newEnabled) &&
				!baseDirectory.empty()) {
#ifdef _WIN32
				ShellExecuteA(NULL, "open", baseDirectory.string().c_str(),
							  NULL, NULL, SW_SHOWDEFAULT);
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
			if (ImGui::MenuItem("Compile", NULL, false, newEnabled)) {
				isCompiling = true;
			}
			if (ImGui::MenuItem("Run", NULL, false, newEnabled)) {
				RunProject();
			}
			if (ImGui::MenuItem("Compile and Run", NULL, false, newEnabled)) {
				// this is done to ensure compiling finishes before running
				isCompiling = true;
				CompileProject();
				RunProject();
			}
			ImGui::EndMenu();
		}

		static ImGui::FileBrowser sourcePortBrowser;
		if (ImGui::BeginMenu("Settings")) {
			if (ImGui::MenuItem("Set Source Port", NULL, false, newEnabled)) {
#ifdef _WIN32
				sourcePortBrowser.SetTypeFilters({".exe"});
#endif
				sourcePortBrowser.SetTitle("Choose Source Port Executable");
				sourcePortBrowser.SetPwd(baseDirectory.parent_path());
				sourcePortBrowser.Open();
			}
			ImGui::EndMenu();
		}
		if (sourcePortBrowser.HasSelected()) {
			projectSourcePort = sourcePortBrowser.GetSelected();
			UpdateQProjectFile();
			sourcePortBrowser.ClearSelected();
		}
		sourcePortBrowser.Display();

		if (ImGui::BeginMenu("Help")) {
			if (ImGui::MenuItem("Documentation")) {
#ifdef _WIN32
				ShellExecuteA(NULL, "open",
							  "https://github.com/BanceDev/QuakePrism/blob/"
							  "main/docs/MANUAL.md",
							  NULL, NULL, SW_SHOWDEFAULT);
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
	if (ImGui::IsWindowHovered()) {
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

	const int maxFrames = MDL::totalFrames - 1;
	const float animProgress =
		MDL::totalFrames == 1 ? 0.0f : MDL::currentFrame / (float)maxFrames;
	ImGui::Begin("Model Tools", nullptr, ImGuiWindowFlags_NoMove);
	char buf[32];
	sprintf(buf, "%d/%d", (int)(animProgress * maxFrames), maxFrames);
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
			if (MDL::currentFrame < maxFrames) {
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
	texImportBrowser.SetTypeFilters({".png", ".jpg", ".tga"});
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
	ImGui::Begin("Texture Tools", nullptr, ImGuiWindowFlags_NoMove);
	if (!currentTextureName.empty()) {
		static std::string localTextureName = "";
		static GLuint currentTexViewID = 0;
		static int width, height;
		static int xOff, yOff = 0;
		static float scale = 0.9f;
		if (localTextureName != currentTextureName.string()) {
			localTextureName = currentTextureName.string();
			glDeleteTextures(1, &currentTexViewID);
			currentTexViewID = 0;
			if (currentTextureName.extension() != ".lmp") {
				LoadTextureFromFile(currentTextureName.string().c_str(),
									&currentTexViewID, &width, &height);
			} else {
				LMP::Lmp2Tex(currentTextureName.string().c_str(),
							&currentTexViewID, &width, &height);
			}
		}
		// texture view controls
		if (ImGui::IsWindowHovered()) {
			ImGuiIO &io = ImGui::GetIO();
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
				xOff += io.MouseDelta.x;
				yOff += io.MouseDelta.y;
			}
			scale += io.MouseWheel * 0.02f;
			if (scale <= 0) {
				scale = 0.1f;
			} else if (scale >= 0.9f) {
				scale = 0.9f;
			}
		}

		// Now calculate and draw the image centered below the header bar
		float displayWidth =
			width >= height
				? ImGui::GetContentRegionAvail().x
				: width * (ImGui::GetContentRegionAvail().y / (float)height);
		float displayHeight =
			height > width
				? ImGui::GetContentRegionAvail().y
				: height * (ImGui::GetContentRegionAvail().x / (float)width);

		displayWidth *= scale;
		displayHeight *= scale;

		// Get the current window size
		ImVec2 windowSize = ImGui::GetWindowSize();

		// Calculate the centered position for the image
		float posX = (windowSize.x - displayWidth) * 0.5f + xOff;
		float posY = (windowSize.y - displayHeight) * 0.5f + yOff;

		// Ensure the image stays within window bounds
		posX = posX > 0 ? posX : 0;
		posY = posY > 28.0f ? posY : 28.0f;  // Ensure it starts below the header bar

		// Apply padding if necessary
		ImGui::SetCursorPos(ImVec2(posX, posY));

		// Draw the image
		ImGui::Image((ImTextureID)(intptr_t)currentTexViewID,
					ImVec2(displayWidth, displayHeight));

		// Draw the header bar second so its under the image
		ImGui::SetCursorPos(ImVec2(0, 28));
		ImGui::GetWindowDrawList()->AddRectFilled(
			ImGui::GetCursorScreenPos(),
			ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth(),
				ImGui::GetCursorScreenPos().y + 28.0f),
			ImColor(255, 225, 135, 30));

		// After drawing the header, draw the button
		ImGui::SetCursorPos(ImVec2(5, 28));
		if (currentTextureName.extension() != ".lmp") {
			if (ImGui::Button("Convert Image to Lump")) {
				LMP::Img2Lmp(currentTextureName);
			}
		} else {
			if (ImGui::Button("Convert Lump to Image")) {
				LMP::Lmp2Img(currentTextureName);
			}
		}
		
	}
	ImGui::End();

}

void DrawSpriteTool() {
	ImGui::Begin("Sprite Tools", nullptr, ImGuiWindowFlags_NoMove);
	ImGuiID dockspace_id = ImGui::GetID("SpriteDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
	static bool first_time = true;
	const auto currentTime = std::chrono::high_resolution_clock::now();
	static std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;

	if (first_time) {
		first_time = false;
		lastTime = currentTime;
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id);
		ImGui::DockBuilderSetNodeSize(dockspace_id,
									  ImGui::GetMainViewport()->Size);

		auto dock_id_up = ImGui::DockBuilderSplitNode(
			dockspace_id, ImGuiDir_Up, 0.7f, nullptr, &dockspace_id);
		auto dock_id_down = ImGui::DockBuilderSplitNode(
			dockspace_id, ImGuiDir_Down, 0.3f, nullptr, &dockspace_id);
		ImGui::DockBuilderDockWindow("Sprite View", dock_id_up);
		ImGui::DockBuilderDockWindow("Sprite Editor", dock_id_down);

		ImGui::DockBuilderFinish(dockspace_id);
	}
	ImGui::End();

	static bool showBoundingRadius = false;
	ImGui::Begin("Sprite View", nullptr, ImGuiWindowFlags_NoMove);
	if (!currentSpriteFrames.empty()) {
		static float sprScale = 1.0f;
		const float width =
			currentSpriteFrames[activeSpriteFrame].width * sprScale;
		const float height =
			currentSpriteFrames[activeSpriteFrame].height * sprScale;
		ImGui::SetCursorPos(ImVec2((ImGui::GetWindowWidth() - width) * 0.5f,
								   (ImGui::GetWindowHeight() - height) * 0.5f));
		ImGui::Image(
			(ImTextureID)(intptr_t)currentSpriteTexs[activeSpriteFrame],
			ImVec2(width, height));
		if (showBoundingRadius) {
			ImDrawList *drawList = ImGui::GetWindowDrawList();
			ImVec2 center = ImVec2(
				ImGui::GetWindowPos().x + ImGui::GetWindowSize().x * 0.5f,
				ImGui::GetWindowPos().y + ImGui::GetWindowSize().y * 0.5f);
			float radius = currentSprite.boundingradius * sprScale;
			ImU32 boundingColor = IM_COL32(255, 0, 0, 30);
			drawList->AddCircleFilled(center, radius, boundingColor);
		}
		// sprite view controls
		if (ImGui::IsWindowHovered()) {
			ImGuiIO &io = ImGui::GetIO();
			sprScale += io.MouseWheel * 0.2f;
			if (sprScale <= 0) {
				sprScale = 0.1f;
			} else if (sprScale >= 10.0f) {
				sprScale = 10.0f;
			}
		}
	}
	ImGui::End();

	static bool paused = false;
	const int maxFrames = currentSprite.numframes - 1;
	const float animProgress = currentSprite.numframes == 1
								   ? 0.0f
								   : activeSpriteFrame / (float)maxFrames;
	ImGui::Begin("Sprite Editor", nullptr, ImGuiWindowFlags_NoMove);
	char buf[32];
	sprintf(buf, "%d/%d", (int)(animProgress * maxFrames), maxFrames);
	ImGui::Columns(2, "tools");
	ImGui::ProgressBar(animProgress, ImVec2(0.0f, 0.0f), buf);
	// Animation Control Buttons
	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::PushButtonRepeat(true);
	if (ImGui::ImageButton((ImTextureID)(intptr_t)backButton, {24, 24})) {
		if (paused && activeSpriteFrame > 0)
			activeSpriteFrame--;
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
			if (activeSpriteFrame < maxFrames) {
				activeSpriteFrame++;
			} else {
				activeSpriteFrame = 0;
			}
		}
	}
	ImGui::PopButtonRepeat();

	if (ImGui::Button("Save Sprite")) {
		SPR::WriteSprite(currentSpritePath.string().c_str());
	}
	ImGui::SameLine();
	// File browser is for import texture
	static ImGui::FileBrowser texImportBrowser;
	texImportBrowser.SetTitle("Select Frame");
	texImportBrowser.SetTypeFilters({".png", ".jpg", ".tga"});
	if (!texImportBrowser.IsOpened())
		texImportBrowser.SetPwd(baseDirectory);

	if (ImGui::Button("Insert Frame") && (paused || maxFrames == 0)) {
		texImportBrowser.Open();
	}
	ImGui::SameLine();

	const bool canRemove = paused && maxFrames > 0;
	if (ImGui::Button("Remove Frame") && canRemove) {
		glDeleteTextures(1, &currentSpriteTexs.at(activeSpriteFrame));
		currentSpriteTexs.erase(currentSpriteTexs.begin() + activeSpriteFrame);
		currentSpriteFrames.erase(currentSpriteFrames.begin() +
								  activeSpriteFrame);
		currentSprite.numframes--;
	}

	// File browser is for new sprite
	static ImGui::FileBrowser newSpriteBrowser(
		ImGuiFileBrowserFlags_MultipleSelection);
	newSpriteBrowser.SetTitle("Select Frames");
	newSpriteBrowser.SetTypeFilters({".png", ".jpg", ".tga"});
	if (!newSpriteBrowser.IsOpened())
		newSpriteBrowser.SetPwd(baseDirectory);

	if (ImGui::Button("New Sprite")) {
		newSpriteBrowser.Open();
	}
	ImGui::SameLine();
	if (ImGui::Button("Export Sprite Frames")) {
		SPR::ExportSpriteFrames();
	}

	ImGui::Checkbox("Show Bounding Radius", &showBoundingRadius);

	ImGui::NextColumn();

	// editable fields
	HelpMarker("The sprite type determines\nits orientation");
	const char *sprTypes[] = {"VP Parallel Upright", "Facing Upright",
							  "VP Parallel", "Oriented",
							  "VP Parallel Oriented"};
	ImGui::Combo("Sprite Type", &currentSprite.type, sprTypes,
				 IM_ARRAYSIZE(sprTypes));
	const char *syncTypes[] = {"Synchronized", "Random"};
	ImGui::Combo("Sync Type", &currentSprite.synctype, syncTypes,
				 IM_ARRAYSIZE(syncTypes));
	HelpMarker("For sprites used\nas a beam");
	ImGui::InputFloat("Beam Length", &currentSprite.beamlength);
	ImGui::InputFloat("Bounding Radius", &currentSprite.boundingradius);

	ImGui::End();

	texImportBrowser.Display();
	if (texImportBrowser.HasSelected()) {
		std::filesystem::path texturePath = texImportBrowser.GetSelected();
		SPR::InsertFrame(texturePath.string().c_str());
		texImportBrowser.ClearSelected();
	}

	newSpriteBrowser.Display();
	if (newSpriteBrowser.HasSelected()) {
		std::vector<std::filesystem::path> framePaths =
			newSpriteBrowser.GetMultiSelected();
		SPR::NewSpriteFromFrames(framePaths);
		newSpriteBrowser.ClearSelected();
	}

	// update the animation if 0.1 seconds have elapsed
	if (!currentSpriteFrames.empty() && !paused) {
		if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime -
																  lastTime)
				.count() >= 100) {
			lastTime = currentTime;
			if (activeSpriteFrame < currentSprite.numframes - 1) {
				activeSpriteFrame++;
			} else {
				activeSpriteFrame = 0;
			}
		}
	}
}

void DrawWADTool() {
	ImGui::Begin("WAD Tools", nullptr, ImGuiWindowFlags_NoMove);
	static int selectedEntry = -1;
	ImGui::GetWindowDrawList()->AddRectFilled(
		ImGui::GetCursorScreenPos(),
		ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth(),
				ImGui::GetCursorScreenPos().y + 28.0f),
		ImColor(255, 225, 135, 30));
	
	if (ImGui::Button("Save WAD")) {
		WAD::WriteWad(currentWadPath.string().c_str());
	}
	ImGui::SameLine();
	
	static ImGui::FileBrowser newWadBrowser(
		ImGuiFileBrowserFlags_MultipleSelection);
	newWadBrowser.SetTitle("Select Frames");
	newWadBrowser.SetTypeFilters({".png", ".jpg", ".tga"});
	if (!newWadBrowser.IsOpened())
		newWadBrowser.SetPwd(baseDirectory);

	static bool isMip = false;	
	if (ImGui::Button("New UI WAD")) {
		newWadBrowser.Open();
		isMip = false;
	}
	ImGui::SameLine();
	if (ImGui::Button("New Map WAD")) {
		newWadBrowser.Open();
		isMip = true;
	}
	
	newWadBrowser.Display();
	if (newWadBrowser.HasSelected()) {
		std::vector<std::filesystem::path> framePaths =
			newWadBrowser.GetMultiSelected();
		WAD::NewWadFromImages(framePaths, isMip);
		newWadBrowser.ClearSelected();
	}
	
	ImGui::SameLine();
	if(ImGui::Button("Export WAD")) {
		WAD::ExportAsImages();
	}
	ImGui::SameLine();
	
	// File browser is for import texture
	static ImGui::FileBrowser texImportBrowser;
	texImportBrowser.SetTitle("Select Frame");
	texImportBrowser.SetTypeFilters({".png", ".jpg", ".tga"});
	if (!texImportBrowser.IsOpened())
		texImportBrowser.SetPwd(baseDirectory);
	if (ImGui::Button("Add UI Texture")) {
		texImportBrowser.Open();
		isMip = false;
	}
	ImGui::SameLine();
	if (ImGui::Button("Add Map Texture")) {
		texImportBrowser.Open();
		isMip = true;
	}
	
	texImportBrowser.Display();
	if (texImportBrowser.HasSelected()) {
		std::filesystem::path texturePath = texImportBrowser.GetSelected();
		WAD::InsertImage(texturePath, isMip);
		texImportBrowser.ClearSelected();
	}
	if (!currentWadTexs.empty()) {

		// Begin a new group to handle image wrapping
		ImGui::BeginGroup();
		for (size_t i = 0; i < currentWadTexs.size(); ++i) {
			int width, height;

			glBindTexture(GL_TEXTURE_2D, currentWadTexs[i]);
    		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
   			glBindTexture(GL_TEXTURE_2D, 0);

			if (width != height) {
				if (width > height) {
					height = (height * 128) / width;
					width = 128;
				} else {
					width = (width * 128) / height;
					height = 128;
				}
			} else {
				width = 128;
				height = 128;
			}

			if (ImGui::ImageButton((ImTextureID)(intptr_t)currentWadTexs[i],
									ImVec2(width, height))) {
				selectedEntry = i;
				ImGui::OpenPopup("Wad Menu");
			}
			int wrap = ImGui::GetContentRegionAvail().x / 142;
			if (wrap == 0)
				wrap = 1;
			if ((i + 1) % wrap != 0) {
				ImGui::SameLine();
			}
		}
		ImGui::EndGroup();
	}
	if (ImGui::BeginPopup("Wad Menu")) {
		if (ImGui::MenuItem("Remove")) {
			WAD::RemoveImage(selectedEntry);
		}
		if (ImGui::MenuItem("Export")) {
			WAD::ExportImage(selectedEntry);
		}
		ImGui::EndPopup();
	}
	ImGui::End();
}

void DrawDebugConsole() {
	static bool consoleOpen = true;
	if (isCompiling)
		consoleOpen = true;
	if (consoleOpen) {
		ImGui::Begin("Console", &consoleOpen, ImGuiWindowFlags_NoMove);
		static std::string consoleText = "";
		if (isCompiling) {
			consoleText = getCompilerOutputString();
			isCompiling = false;
		}
		ImGui::TextUnformatted(consoleText.c_str());
		ImGui::End();
	}
}

static void SaveQuakeCFile(std::string textToSave,
						   const std::filesystem::path &currentFile) {
	// had issue with extra whitespace so this cleans that
	size_t end = textToSave.find_last_not_of(" \t\n\r");
	if (end == std::string::npos) {
		textToSave = ""; // All characters are whitespace or newlines
	}
	textToSave = textToSave.substr(0, end + 1);
	std::ofstream output(currentFile);
	if (output.is_open()) {
		output << textToSave;
		output.close();
	} else {
		isErrorOpen = true;
		userError = SAVE_FAILED;
	}
}

void SaveFromEditor(TextEditor *editor) {
	std::string textToSave = editor->GetText();
	for (int i = 0; i < editorList.size(); ++i) {
		if (&editorList.at(i) == editor) {
			SaveQuakeCFile(textToSave, currentQCFileNames.at(i));
			createTextEditorDiagnostics();
			break;
		}
	}
}

static void DrawTextTab(TextEditor &editor,
						const std::filesystem::path &currentFile, bool &tabOpen,
						const bool focused, const bool isFindOpen) {
	auto lang = TextEditor::LanguageDefinition::QuakeC();
	editor.SetLanguageDefinition(lang);
	ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
	if (focused) {
		newTabOpened = false;
		flags = ImGuiTabItemFlags_SetSelected;
	}
	if (ImGui::BeginTabItem(currentFile.filename().string().c_str(), &tabOpen,
							flags)) {
		currentTextEditor = &editor;
		auto cpos = editor.GetCursorPosition();

		ImGui::GetWindowDrawList()->AddRectFilled(
			ImGui::GetCursorScreenPos(),
			ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth(),
				   ImGui::GetCursorScreenPos().y + 28.0f),
			ImColor(255, 225, 135, 30));

		ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s ", cpos.mLine + 1,
					cpos.mColumn + 1, editor.GetTotalLines(),
					editor.IsOverwrite() ? "Ovr" : "Ins",
					editor.IsUnsaved() ? "*" : " ",
					editor.GetLanguageDefinition().mName.c_str());
		if (isFindOpen) {
			ImGui::SameLine();
			ImGui::SetNextItemWidth(256.0f);
			static char expr[32] = "";
			ImGui::InputText("##expr", expr, IM_ARRAYSIZE(expr));
			ImGui::SameLine();
			if (ImGui::Button("Next")) {
				editor.SelectNextOccurrenceOf(expr, strlen(expr));
			}
		}
		editor.Render("TextEditor");
		ImGui::EndTabItem();
	}
}

static void RemoveEditorTab(int index) {
	if (index >= 0 && index < editorList.size()) {
		editorList.erase(editorList.begin() + index);
		currentQCFileNames.erase(currentQCFileNames.begin() + index);
	}
}

void DrawTextEditor() {
	ImGui::Begin("QuakeC Editor");

	ImGuiID dockspace_id = ImGui::GetID("Editor DockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
	static bool first_time = true;
	if (first_time) {
		first_time = false;
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id);
		ImGui::DockBuilderSetNodeSize(dockspace_id,
									  ImGui::GetMainViewport()->Size);
	}
	static auto dock_id_up = ImGui::DockBuilderSplitNode(
		dockspace_id, ImGuiDir_Up, 0.85f, nullptr, &dockspace_id);
	static auto dock_id_down = ImGui::DockBuilderSplitNode(
		dockspace_id, ImGuiDir_Down, 0.15f, nullptr, &dockspace_id);

	ImGui::DockBuilderDockWindow("Editor", dock_id_up);
	ImGui::DockBuilderDockWindow("Console", dock_id_down);

	ImGui::DockBuilderFinish(dockspace_id);
	ImGui::End();

	ImGui::Begin("Editor", nullptr,
				 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar);
	if (!editorList.empty()) {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Save", "Ctrl-S", nullptr)) {
					SaveFromEditor(currentTextEditor);
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit")) {
				bool ro = currentTextEditor->IsReadOnly();
				if (ImGui::MenuItem("Read-Only", nullptr, &ro))
					currentTextEditor->SetReadOnly(ro);

				ImGui::Separator();

				if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr,
									!ro && currentTextEditor->CanUndo()))
					currentTextEditor->Undo();
				if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr,
									!ro && currentTextEditor->CanRedo()))
					currentTextEditor->Redo();
				if (ImGui::MenuItem("Find", "Ctrl-F")) {
					isFindOpen = !isFindOpen;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr,
									currentTextEditor->HasSelection()))
					currentTextEditor->Copy();
				if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr,
									!ro && currentTextEditor->HasSelection()))
					currentTextEditor->Cut();
				if (ImGui::MenuItem("Delete", "Del", nullptr,
									!ro && currentTextEditor->HasSelection()))
					currentTextEditor->Delete();
				if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr,
									!ro &&
										ImGui::GetClipboardText() != nullptr))
					currentTextEditor->Paste();

				ImGui::Separator();
				if (ImGui::MenuItem("Select all", nullptr, nullptr))
					currentTextEditor->SetSelection(
						TextEditor::Coordinates(),
						TextEditor::Coordinates(
							currentTextEditor->GetTotalLines(), 0));

				ImGui::EndMenu();
			}
		}

		if (ImGui::BeginMenu("View")) {
			if (ImGui::MenuItem("Dark Mode")) {
				editorTheme = "prism-dark";
				UpdateQProjectFile();
				for (auto &editor : editorList) {
					editor.SetPalette(TextEditor::GetDarkPalette());
				}
			}
			if (ImGui::MenuItem("Light Mode")) {
				editorTheme = "prism-light";
				UpdateQProjectFile();
				for (auto &editor : editorList) {
					editor.SetPalette(TextEditor::GetLightPalette());
				}
			}
			if (ImGui::MenuItem("Retro Mode")) {
				editorTheme = "prism-retro";
				UpdateQProjectFile();
				for (auto &editor : editorList) {
					editor.SetPalette(TextEditor::GetRetroBluePalette());
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	if (ImGui::BeginTabBar("Tab Bar")) {
		for (int i = 0; i < editorList.size(); ++i) {
			bool tabOpen = true;
			if ((i == editorList.size() - 1) && newTabOpened) {
				DrawTextTab(editorList.at(i), currentQCFileNames.at(i), tabOpen,
							true, isFindOpen);
			} else {
				DrawTextTab(editorList.at(i), currentQCFileNames.at(i), tabOpen,
							false, isFindOpen);
			}
			if (!tabOpen) {
				RemoveEditorTab(i);
				--i;
			}
		}
		ImGui::EndTabBar();
	}
	ImGui::End();

	DrawDebugConsole();
}

static bool AlphabeticalComparator(const std::filesystem::directory_entry &a,
								   const std::filesystem::directory_entry &b) {
	return a.path().filename().string() < b.path().filename().string();
}

static void DrawFileTree(const std::filesystem::path &currentPath) {
	if (!currentPath.empty()) {
		std::vector<std::filesystem::directory_entry> directoryEntries;

		// Collect all entries (both directories and files) in the current
		// path
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
				} else if (directoryEntry.path().extension() == ".tga" ||
						   directoryEntry.path().extension() == ".jpg" ||
						   directoryEntry.path().extension() == ".png") {
					icon = imageIcon;
				} else if (directoryEntry.path().extension() == ".lmp") {
					icon = lumpIcon;
				} else if (directoryEntry.path().extension() == ".bsp") {
					icon = bspIcon;
				} else if (directoryEntry.path().extension() == ".wad") {
					icon = wadIcon;
				} else if (directoryEntry.path().extension() == ".wav" ||
						   directoryEntry.path().extension() == ".ogg" ||
						   directoryEntry.path().extension() == ".mp3") {
					icon = audioIcon;
				} else if (directoryEntry.path().extension() == ".dem") {
					icon = demoIcon;
				} else if (directoryEntry.path().extension() == ".qc") {
					icon = qcIcon;
				} else if (directoryEntry.path().extension() == ".exe") {
					icon = exeIcon;
				} else if (directoryEntry.path().extension() == ".cfg") {
					icon = cfgIcon;
				} else if (directoryEntry.path().extension() == ".map") {
					icon = mapIcon;
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
						// Append the original file extension to the new
						// name
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
					std::filesystem::remove_all(path);
				}
				ImGui::EndPopup();
			}

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				// Handle file loading based on extension
				if (path.extension() == ".qc" || path.extension() == ".src" ||
					path.extension() == ".rc" || path.extension() == ".cfg") {
					std::ifstream input(path);
					if (input.good()) {
						// prevent duplicate tabs from being opened
						if (std::find(currentQCFileNames.begin(),
									  currentQCFileNames.end(),
									  path) == currentQCFileNames.end()) {
							currentQCFileNames.push_back(path);
							newTabOpened = true;
							std::string str(
								(std::istreambuf_iterator<char>(input)),
								std::istreambuf_iterator<char>());
							TextEditor editor;
							editor.SetText(str);
							editor.SetFileName(path.filename().string());
							if (editorTheme == "prism-dark") {
								editor.SetPalette(TextEditor::GetDarkPalette());
							} else if (editorTheme == "prism-light") {
								editor.SetPalette(
									TextEditor::GetLightPalette());
							} else if (editorTheme == "prism-retro") {
								editor.SetPalette(
									TextEditor::GetRetroBluePalette());
							}
							createTextEditorDiagnostics();
							editorList.push_back(editor);
						}
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
				} else if (path.extension() == ".tga" ||
						   path.extension() == ".jpg" ||
						   path.extension() == ".png" ||
						   path.extension() == ".lmp") {
					std::ifstream input(path);
					if (input.good()) {
						currentTextureName = path;
					} else {
						isErrorOpen = true;
						userError = LOAD_FAILED;
					}
					ImGui::SetWindowFocus("Texture Tools");
				} else if (path.extension() == ".spr") {
					std::ifstream input(path);
					if (input.good()) {
						activeSpriteFrame = 0;
						currentSpritePath = path;
						SPR::CleanupSprite();
						SPR::OpenSprite(path.string().c_str());
					} else {
						isErrorOpen = true;
						userError = LOAD_FAILED;
					}
					ImGui::SetWindowFocus("Sprite Tools");
				} else if (path.extension() == ".wad") {
					std::ifstream input(path);
					if (input.good()) {
						currentWadPath = path;
						WAD::CleanupWad();
						WAD::OpenWad(path.string().c_str());
					} else {
						isErrorOpen = true;
						userError = LOAD_FAILED;
					}
					ImGui::SetWindowFocus("WAD Tools");
				}
			}

			if (node_open) {
				if (directoryEntry.is_directory()) {
					DrawFileTree(directoryEntry.path());
				}
				ImGui::TreePop();
			}

			ImGui::PopID();
		}
	}
}

void DrawFileExplorer() {

	ImGui::Begin("Project Browser", nullptr, ImGuiWindowFlags_NoMove);
	// prevents crash if user deletes a project folder and then tries to
	// open it for some reason
	if (!std::filesystem::exists(baseDirectory)) {
		baseDirectory.clear();
	}
	if (baseDirectory.empty()) {
		if (ImGui::Button("New Project")) {
			isNewProjectOpen = true;
		}
		if (ImGui::Button("Open Project")) {
			isOpenProjectOpen = true;
		}
	}
	if (!baseDirectory.empty()) {
		DrawFileTree(baseDirectory);
	}

	ImGui::End();
}

void DrawPaletteTool() {
	static float colors[256][3];
	if (!palLoaded) {
		for (int i = 0; i < 256; ++i) {
			for (int j = 0; j < 3; ++j) {
				colors[i][j] = colormap[i][j] / (float)256;
			}
		}
		palLoaded = true;
	}
	ImGui::Begin("Palette Editor", nullptr, ImGuiWindowFlags_NoMove);
	ImGui::GetWindowDrawList()->AddRectFilled(
		ImGui::GetCursorScreenPos(),
		ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth(),
				ImGui::GetCursorScreenPos().y + 28.0f),
		ImColor(255, 225, 135, 30));
	
	if (ImGui::Button("Save Palette")) {
		// first expor the colors from the widgets to the colormap
		for (int i = 0; i < 256; ++i) {
			for (int j = 0; j < 3; ++j) {
				colormap[i][j] = colors[i][j] * 256;
			}
		}

		// then write the palette to the lmp file
		FILE *fp;

		fp = fopen((baseDirectory / "gfx/palette.lmp").string().c_str(), "wb");
		if (!fp) {
			return;
		}

		for (int i = 0; i < 256; ++i) {
			for (int j = 0; j < 3; ++j) {
				fwrite(&colormap[i][j], 1, sizeof(unsigned char), fp);
			}
		}
		fclose(fp);
	}
	for (int i = 0; i < 256; ++i) {
		ImGui::PushID(i);
		ImGui::ColorEdit3("##coloredit", colors[i],
						  ImGuiColorEditFlags_NoInputs |
							  ImGuiColorEditFlags_NoLabel);
		ImGui::PopID();
		if ((i + 1) % 16 != 0)
			ImGui::SameLine();
	}

	ImGui::End();
}

void DrawOpenProjectPopup() {
	if (!isOpenProjectOpen)
		return;

	ImGui::OpenPopup("Open Project");
	isOpenProjectOpen = ImGui::BeginPopupModal(
		"Open Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	if (isOpenProjectOpen) {
		static int itemCurrentIdx = 0;
		ImGui::TextUnformatted("Recent Projects");
		if (ImGui::BeginListBox("##projlistbox")) {
			for (int n = 0; n < projectsList.size(); n++) {
				const bool isSelected = (itemCurrentIdx == n);
				if (ImGui::Selectable(
						projectsList.at(n).filename().string().c_str(),
						isSelected))
					itemCurrentIdx = n;

				// Set the initial focus when opening the combo
				// (scrolling + keyboard navigation focus)
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndListBox();
		}
		ImGui::SameLine();
		static ImGui::FileBrowser projectLocationBrowser(
			ImGuiFileBrowserFlags_SelectDirectory);
		static std::filesystem::path selectedProjectDirecory;
		if (ImGui::Button("Import Project Folder")) {
			projectLocationBrowser.SetTitle("Choose Project Location");
			projectLocationBrowser.Open();
		}
		if (projectLocationBrowser.HasSelected()) {
			selectedProjectDirecory = projectLocationBrowser.GetSelected();
			projectLocationBrowser.ClearSelected();
		}
		projectLocationBrowser.Display();

		if (!selectedProjectDirecory.empty()) {
			projectsList.push_back(selectedProjectDirecory);
			AddProjectToRecents(selectedProjectDirecory);
			baseDirectory = selectedProjectDirecory;

			// Handle qproj file
			CreateQProjectFile(); // only will work if the file DNE
			ReadQProjectFile();

			currentQCFileNames.clear();
			currentModelName.clear();
			currentTextureName.clear();
			loadColormap();
			palLoaded = false;
			selectedProjectDirecory.clear();
			isOpenProjectOpen = false;
			ImGui::CloseCurrentPopup();
		}

		if (ImGui::Button("Cancel")) {
			isOpenProjectOpen = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
		if (ImGui::Button("Open")) {
			try {
				baseDirectory = projectsList.at(itemCurrentIdx);
				// Handle qproj file
				CreateQProjectFile(); // only will work if the file DNE
				ReadQProjectFile();

				currentQCFileNames.clear();
				currentModelName.clear();
				currentTextureName.clear();
				loadColormap();
				palLoaded = false;
			} catch (const std::out_of_range) {
				isNewProjectOpen = true;
			}
			isOpenProjectOpen = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

static bool CopyTemplate(const std::filesystem::path &source,
						 const std::filesystem::path &destination) {
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

static int InputTextFilterWhitespace(ImGuiInputTextCallbackData *data) {
	if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
		if (isspace(data->EventChar)) {
			return 1; // Ignore the character
		}
	}
	return 0;
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
			static bool shouldImportEngine = false;
			ImGui::Checkbox("Import Quakespasm", &shouldImportEngine);
			ImGui::Dummy(ImVec2(1, 20));
			static char projectName[32] = "";
			ImGui::TextUnformatted("Project Name");
			ImGui::SetNextItemWidth(360.0f);
			ImGui::InputText("##projectname", projectName,
							 IM_ARRAYSIZE(projectName),
							 ImGuiInputTextFlags_CallbackCharFilter,
							 InputTextFilterWhitespace);

			static ImGui::FileBrowser projectLocationBrowser(
				ImGuiFileBrowserFlags_SelectDirectory);
#ifndef _WIN32
			projectLocationBrowser.SetPwd(std::filesystem::path(std::getenv("HOME")));
#endif
			static std::filesystem::path selectedProjectDirecory;
			if (ImGui::Button("Make Project") && strcmp(projectName, "") != 0 &&
				projectType > 0) {
				projectLocationBrowser.SetTitle("Choose Project Location");
				projectLocationBrowser.Open();
			}
			if (projectLocationBrowser.HasSelected()) {
				selectedProjectDirecory =
					projectLocationBrowser.GetSelected() / projectName;
				projectLocationBrowser.ClearSelected();
			}

			projectLocationBrowser.Display();

			if (!selectedProjectDirecory.empty()) {
				if (shouldImportEngine) {
					shouldImportEngine = false;
					// import engine executable
#ifdef _WIN32
					for (const auto &entry :
						 std::filesystem::recursive_directory_iterator(
							 executingDirectory / "res/templates/Windows")) {
#else
					for (const auto &entry :
						 std::filesystem::recursive_directory_iterator(
							 executingDirectory / "res/templates/Linux")) {
#endif
						const auto &path = entry.path();
#ifdef _WIN32
						auto relative_path = std::filesystem::relative(
							path,
							(executingDirectory / "res/templates/Windows"));
#else
						auto relative_path = std::filesystem::relative(
							path, (executingDirectory / "res/templates/Linux"));
#endif
						auto dest = selectedProjectDirecory.parent_path() /
									relative_path;
						if (std::filesystem::exists(
								path)) { // prevent duplicate imports (lead
										 // to crash on windows)
							if (std::filesystem::is_directory(path)) {
								std::filesystem::create_directories(dest);
							} else if (std::filesystem::is_regular_file(path) ||
									   std::filesystem::is_symlink(path)) {
								std::filesystem::copy(
									path, dest,
									std::filesystem::copy_options::
										overwrite_existing);
							}
						}
					}
				}
				// import project files
				switch (projectType) {
				case 1: {
					std::filesystem::path blankDir =
						executingDirectory / "res/templates/Blank";
					if (!CopyTemplate(blankDir, selectedProjectDirecory)) {
						userError = PROJECT_FAILURE;
						isErrorOpen = true;
					}
					break;
				}
				case 2: {
					std::filesystem::path projectPath = selectedProjectDirecory;

					// Create the destination directory if it does not exist
					if (!std::filesystem::exists(projectPath)) {
						std::filesystem::create_directory(projectPath);
					}

					for (const auto &pak : paks) {
						if (!PAK::ExtractPAK(pak, &projectPath)) {
							userError = PROJECT_FAILURE;
							isErrorOpen = true;
							break;
						}
					}

					if (!std::filesystem::exists(projectPath / "src")) {
						std::filesystem::create_directory(projectPath / "src");
					}

					std::filesystem::path srcDir =
						selectedProjectDirecory / "src";
					std::filesystem::path quakeCodebase;
					if (codebaseType == 0) {
						quakeCodebase =
							executingDirectory / "res/templates/Id1/src";

					} else {
						quakeCodebase =
							executingDirectory / "res/templates/Blank/src";
					}
					if (!CopyTemplate(quakeCodebase, srcDir)) {
						userError = PROJECT_FAILURE;
						isErrorOpen = true;
					}
					break;
				}
				case 3: {
					std::filesystem::path libreQuakeDir =
						executingDirectory / "res/templates/LibreQuake";
					if (!CopyTemplate(libreQuakeDir, selectedProjectDirecory)) {
						userError = PROJECT_FAILURE;
						isErrorOpen = true;
					}
					break;
				}
				default:
					break;
				}

				AddProjectToRecents(selectedProjectDirecory);
				projectsList.push_back(selectedProjectDirecory);
				baseDirectory = selectedProjectDirecory;

				// Handle qproj file
				CreateQProjectFile(); // only will work if the file DNE
				ReadQProjectFile();

				selectedProjectDirecory.clear();
				currentQCFileNames.clear();
				currentModelName.clear();
				currentTextureName.clear();
				projectName[0] = '\0';

				isNewProjectOpen = false;
				paks.clear();
				ImGui::CloseCurrentPopup();
			}
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
	ImGui::EndPopup();
}

void DrawAboutPopup() {

	if (!isAboutOpen)
		return;

	ImGui::OpenPopup("About");
	isAboutOpen = ImGui::BeginPopupModal("About", nullptr,
										 ImGuiWindowFlags_AlwaysAutoResize);
	if (isAboutOpen) {
		ImGui::Image((ImTextureID)(intptr_t)appIcon, {64, 64});

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

void DrawStartupPopup() {
	if (!isStartupOpen)
		return;

	ImGui::OpenPopup("startup");
	isStartupOpen = ImGui::BeginPopupModal("startup", nullptr,
										 ImGuiWindowFlags_AlwaysAutoResize);
	if (isStartupOpen) {
		ImGui::Image((ImTextureID)(intptr_t)appIcon, {64, 64});

		ImGui::SameLine();

		ImGui::BeginGroup();
		ImGui::Text("Welcome to Quake Prism!");
		ImGui::Text("create or open a project");
		ImGui::EndGroup();

		if (ImGui::Button("New")) {
			isStartupOpen = false;
			isNewProjectOpen = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Open")) {
			isStartupOpen = false;
			isOpenProjectOpen = true;
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
		ImGui::Image((ImTextureID)(intptr_t)appIcon, {64, 64});

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
		case PROJECT_FAILURE:
			ImGui::BeginGroup();
			ImGui::Text("Error: Unable to load project.");
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
