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
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, image_data);
  stbi_image_free(image_data);

  *out_texture = image_texture;
  *out_width = image_width;
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
          label, ImVec2(-1, ImGui::GetFontSize() + style.FramePadding.y * 2))) {
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
  ImGui::ImageButton((ImTextureID)icon, {24, 24}, ImVec2(0, 0), ImVec2(1, 1),
                     0);
  ImGui::SameLine();
  ImGui::Text(label);
  ImGui::EndGroup();
  if (opened)
    ImGui::TreePush(label);
  return opened != 0;
}

} // namespace QuakePrism
