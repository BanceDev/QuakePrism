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

#include "framebuffer.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "panes.h"
#include "resources.h"
#include "theme.h"
#include "util.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <X11/X.h>
#include <filesystem>
#include <stdio.h>
#ifndef _WIN32
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <SDL2/SDL_image.h>
#endif // !#endif
// global defined indices for OpenGL
GLuint VAO;		   // vertex array object
GLuint VBO;		   // vertex buffer object
GLuint FBO;		   // frame buffer object
GLuint RBO;		   // rendering buffer object
GLuint texture_id; // the texture id we'll need later to create a texture

#ifndef _WIN32
// Function to load an image into an SDL_Surface
SDL_Surface *LoadSurface(const char *path) {
	SDL_Surface *surface = IMG_Load(path);
	if (!surface) {
		printf("IMG_Load: %s\n", IMG_GetError());
	}
	return surface;
}
#endif

// Main code
int main(int, char **) {
	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) !=
		0) {
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	// GL 3.0 + GLSL 130
	const char *glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
						SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_WindowFlags window_flags =
		(SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
						  SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_Window *window =
		SDL_CreateWindow("QuakePrism", SDL_WINDOWPOS_CENTERED,
						 SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
	if (window == nullptr) {
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		return -1;
	}

	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |=
		ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |=
		ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	ImGuiStyle &style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	QuakePrism::SetImguiTheme();
	QuakePrism::loadIcons();
	QuakePrism::InitializeRecentProjectsList();
	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Initialize the framebuffer for the model viewer
	glewInit();

	// Load in the fonts used by the editor
	QuakePrism::loadFonts();
	// Our state
	ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

	// create the framebuffer right before the main loop
	QuakePrism::createFramebuffer(FBO, RBO, texture_id);
	// Set Window Icon
	// Main loop
	bool done = false;
#ifndef _WIN32
	std::filesystem::path iconPath =
		std::filesystem::current_path() / "res/prism_small.png";
#endif
	while (!done) {
#ifndef _WIN32
		SDL_Surface *iconSurface = LoadSurface(iconPath.c_str());
		if (iconSurface) {
			SDL_SetWindowIcon(window, iconSurface);
			SDL_FreeSurface(iconSurface);
		}
#endif
		// Poll and handle events (inputs, window resize, etc.)
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT &&
				event.window.event == SDL_WINDOWEVENT_CLOSE &&
				event.window.windowID == SDL_GetWindowID(window))
				done = true;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
		ImGui::PushFont(QuakePrism::notoSansFont);
		QuakePrism::DrawMenuBar();
		ImGui::DockSpaceOverViewport();

		QuakePrism::DrawModelViewer(texture_id, RBO, FBO);
		QuakePrism::DrawTextureViewer();
		QuakePrism::DrawPaletteTool();
		ImGui::PopFont();

		ImGui::PushFont(QuakePrism::inconsolataFont);
		QuakePrism::DrawTextEditor();
		ImGui::PopFont();

		ImGui::PushFont(QuakePrism::notoSansFont);
		QuakePrism::DrawFileExplorer();

		// Display all active popups
		QuakePrism::DrawAboutPopup();
		QuakePrism::DrawErrorPopup();
		QuakePrism::DrawOpenProjectPopup();
		QuakePrism::DrawNewProjectPopup();

		ImGui::PopFont();

		ImGui::Render();

		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w,
					 clear_color.y * clear_color.w,
					 clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			SDL_Window *backup_current_window = SDL_GL_GetCurrentWindow();
			SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
		}

		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	glDeleteFramebuffers(1, &FBO);
	glDeleteTextures(1, &texture_id);
	glDeleteRenderbuffers(1, &RBO);

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
