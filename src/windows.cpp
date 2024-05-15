#include "windows.h"
#include "TextEditor.h"
#include "framebuffer.h"
#include "imgui.h"
#include "mdl.h"
#include "util.h"
#include <filesystem>
#include <fstream>
#include <list>

enum {
  MISSING_COMPILER,
  MISSING_GAME,
  MISSING_PROJECTS,
  SAVE_FAILED,
  LOAD_FAILED
};

int userError;
bool errorOpen;

std::filesystem::path currentFileName;
std::filesystem::path currentDirectory;
std::filesystem::path baseDirectory;
namespace QuakePrism {

void DrawMenuBar() {
  if (ImGui::BeginMainMenuBar()) {

    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Open")) {
        // editorLayer->ShowOpenProject();
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
        // Draw about popup
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
  ImGui::Begin("Model Viewer");

  // we access the ImGui window size
  const float window_width = ImGui::GetContentRegionAvail().x;
  const float window_height = ImGui::GetContentRegionAvail().y;

  // we rescale the framebuffer to the actual window size here and reset the
  // glViewport

  glViewport(0, 0, window_width, window_height);
  QuakePrism::rescaleFramebuffer(window_width, window_height, RBO, texture_id);
  ImGui::Image((ImTextureID)texture_id, ImGui::GetContentRegionAvail(),
               ImVec2(0, 1), ImVec2(1, 0));

  ImGui::End();

  // Render the model for the framebuffer
  QuakePrism::bindFramebuffer(FBO);

  // Render model
  MDL::cleanup();
  MDL::reshape(window_width, window_height);
  MDL::render();

  // and unbind it again
  QuakePrism::unbindFramebuffer();
}

void DrawTextEditor(TextEditor &editor) {

  auto lang = TextEditor::LanguageDefinition::QuakeC();
  editor.SetLanguageDefinition(lang);

  auto cpos = editor.GetCursorPosition();
  ImGui::Begin("QuakeC Editor", nullptr,
               ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar |
                   ImGuiWindowFlags_NoMove);
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Save", "Ctrl-S", nullptr)) {
        auto textToSave = editor.GetText();
        std::ofstream output(currentFileName);
        if (output.is_open()) {
          output << textToSave;
          output.close();
        } else {
          errorOpen = true;
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
      if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor.CanRedo()))
        editor.Redo();

      ImGui::Separator();

      if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
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
        editor.SetSelection(TextEditor::Coordinates(),
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

    // Theres probabaly a better way to do this without iterating 3 times but I
    // dont know it rn
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
      int width, height;

      if (directoryEntry.is_directory()) {

        QuakePrism::LoadTextureFromFile("./res/DirectoryIcon.png", &icon,
                                        &width, &height);
      } else {
        QuakePrism::LoadTextureFromFile("./res/FileIcon.png", &icon, &width,
                                        &height);
      }

      if (QuakePrism::ImageTreeNode(filenameString.c_str(), icon)) {
        if (ImGui::IsItemHovered() &&
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
          if (path.extension() == ".qc") {
            currentFileName = path;
            std::ifstream input(path);
            if (input.good()) {
              std::string str((std::istreambuf_iterator<char>(input)),
                              std::istreambuf_iterator<char>());
              editor.SetText(str);
            } else {
              errorOpen = true;
              userError = LOAD_FAILED;
            }

            input.close();
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
      // openProjectOpen = true;
    }
  }
  if (!baseDirectory.empty()) {
    DrawFileTree(baseDirectory, editor);
  }

  ImGui::End();
}

void DrawAboutPopup() {

  GLuint icon;
  int width, height;

  ImGui::OpenPopup("About");
  ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  QuakePrism::LoadTextureFromFile("./res/prism_small.png", &icon, &width,
                                  &height);
  ImGui::Image((ImTextureID)icon, {48, 48});

  ImGui::SameLine();

  // TODO: See if there is a way to add spacing later

  ImGui::BeginGroup();
  ImGui::Text("Quake Prism Development Toolkit");
  ImGui::Text("by Bance.");
  ImGui::EndGroup();

  if (ImGui::Button("Close")) {
    ImGui::CloseCurrentPopup();
  }

  ImGui::EndPopup();
}
} // namespace QuakePrism