// SDL2_OpenGL.cpp : Defines the entry point for the console application.
// MDL loader http://tfc.duke.free.fr/coding/mdl-specs-en.html ported to SDL2 by
// Memorix101

#include "main.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "mdl.h"
#include <SDL_opengl.h>
#include <iostream>
#include <stdio.h>

bool init() {
  // Initialization flag
  bool success = true;

  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
    success = false;
  } else {
    // Request an OpenGL 3.1 context (should be core)
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);

    // Create window
    gWindow = SDL_CreateWindow(
        "QuakePrism", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (gWindow == NULL) {
      printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
      success = false;
    } else {

      // Get window surface
      gScreenSurface = SDL_GetWindowSurface(gWindow);

      // Create context
      gContext = SDL_GL_CreateContext(gWindow);
      if (gContext == NULL) {
        printf("OpenGL context could not be created! SDL Error: %s\n",
               SDL_GetError());
        success = false;
      } else {
        // Use Vsync
        if (SDL_GL_SetSwapInterval(1) < 0) {
          printf("Warning: Unable to set VSync! SDL Error: %s\n",
                 SDL_GetError());
        }

        // Initialize OpenGL
        if (!initGL()) {
          printf("Unable to initialize OpenGL!\n");
          success = false;
        }
      }
      // Setup Dear ImGui context
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();

      // Setup Dear ImGui style
      ImGui::StyleColorsDark();
      // ImGui::StyleColorsLight();

      // Setup Platform/Renderer backends
      ImGui_ImplSDL2_InitForOpenGL(gWindow, gContext);
      ImGui_ImplOpenGL3_Init("#version 130");

      glewInit();
    }
  }

  return success;
}

bool initGL() {
  bool success = true;

  GLenum error = GL_NO_ERROR;

  // Initialize Projection Matrix
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  // Check for error
  error = glGetError();
  if (error != GL_NO_ERROR) {
    printf("Error initializing OpenGL! %s\n", gluErrorString(error));
    success = false;
  }

  // Initialize Modelview Matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Check for error
  error = glGetError();
  if (error != GL_NO_ERROR) {
    printf("Error initializing OpenGL! %s\n", gluErrorString(error));
    success = false;
  }

  // Initialize clear color
  glClearColor(0.f, 0.f, 0.f, 1.f);

  // Check for error
  error = glGetError();
  if (error != GL_NO_ERROR) {
    printf("Error initializing OpenGL! %s\n", gluErrorString(error));
    success = false;
  }

  return success;
}

void close() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  glDeleteFramebuffers(1, &FBO);
  glDeleteTextures(1, &texture_id);
  glDeleteRenderbuffers(1, &RBO);
  // Destroy window
  SDL_GL_DeleteContext(gContext);
  SDL_DestroyWindow(gWindow);
  gWindow = NULL;

  // Quit SDL subsystems
  SDL_Quit();
}

void create_framebuffer() {
  glGenFramebuffers(1, &FBO);
  glBindFramebuffer(GL_FRAMEBUFFER, FBO);

  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB,
               GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture_id, 0);

  glGenRenderbuffers(1, &RBO);
  glBindRenderbuffer(GL_RENDERBUFFER, RBO);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_WIDTH,
                        SCREEN_HEIGHT);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, RBO);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

// here we bind our framebuffer
void bind_framebuffer() { glBindFramebuffer(GL_FRAMEBUFFER, FBO); }

// here we unbind our framebuffer
void unbind_framebuffer() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

// and we rescale the buffer, so we're able to resize the window
void rescale_framebuffer(float width, float height) {
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture_id, 0);

  glBindRenderbuffer(GL_RENDERBUFFER, RBO);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, RBO);
}

int main(int argc, char *args[]) {
  // Start up SDL and create window
  if (!init()) {
    printf("Failed to initialize!\n");
  } else {
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    // Main loop flag
    bool quit = false;

    // Event handler
    SDL_Event event;

    // Enable text input
    SDL_StartTextInput();

    // While application is running
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

    create_framebuffer();

    while (!quit) {
      // Handle events on queue
      while (SDL_PollEvent(&event) != 0) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
          quit = true;
        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(gWindow))
          quit = true;
      }

      // Start the Dear ImGui frame
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplSDL2_NewFrame();
      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      ImGui::NewFrame();

      ImGui::ShowDemoWindow();

      // 1. Show the big demo window (Most of the sample code is in
      // ImGui::ShowDemoWindow()! You can browse its code to learn more about
      // Dear ImGui!).
      ImGui::Begin("My Scene");

      // we access the ImGui window size
      const float window_width = ImGui::GetContentRegionAvail().x;
      const float window_height = ImGui::GetContentRegionAvail().y;

      // we rescale the framebuffer to the actual window size here and reset the
      // glViewport
      rescale_framebuffer(window_width, window_height);
      glViewport(0, 0, window_width, window_height);

      // we get the screen position of the window
      ImVec2 pos = ImGui::GetCursorScreenPos();

      // and here we can add our created texture as image to ImGui
      // unfortunately we need to use the cast to void* or I didn't find another
      // way tbh
      /*ImGui::GetWindowDrawList()->AddImage(
          (ImTextureID)texture_id, ImVec2(pos.x, pos.y),
          ImVec2(pos.x + window_width, pos.y + window_height), ImVec2(0, 1),
          ImVec2(1, 0));*/
      ImGui::Image(
			(ImTextureID)texture_id, 
			ImGui::GetContentRegionAvail(), 
			ImVec2(0, 1), 
			ImVec2(1, 0));

      ImGui::End();
      ImGui::Render();
      
      
      
      // now we can bind our framebuffer
      bind_framebuffer();

      // Render model
      MDL::cleanup();
      MDL::reshape(SCREEN_WIDTH, SCREEN_HEIGHT);
      MDL::render();

      // and unbind it again
      unbind_framebuffer();
      

      
      glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
      glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                   clear_color.z * clear_color.w, clear_color.w);
      glClear(GL_COLOR_BUFFER_BIT);
      MDL::cleanup();
      MDL::reshape(SCREEN_WIDTH, SCREEN_HEIGHT);
      MDL::render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      
      

      // Update screen
      SDL_GL_SwapWindow(gWindow);
    }

    // Disable text input
    SDL_StopTextInput();
  }

  // Free resources and close SDL
  close();

  return 0;
}
