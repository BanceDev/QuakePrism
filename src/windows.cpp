#include "windows.h"
#include "TextEditor.h"
#include "imgui.h"
#include <filesystem>
#include <fstream>

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
        // editorLayer->ShowAboutModal();
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

void DrawModelViewer(const GLuint texture_id) {
  ImGui::Begin("Model Viewer");

  ImGui::Image((ImTextureID)texture_id, ImGui::GetContentRegionAvail(),
               ImVec2(0, 1), ImVec2(1, 0));

  ImGui::End();
}

void DrawTextEditor(TextEditor *editor) {

  auto lang = TextEditor::LanguageDefinition::QuakeC();
  editor->SetLanguageDefinition(lang);

  auto cpos = editor->GetCursorPosition();
  ImGui::Begin("QuakeC Editor", nullptr,
               ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar |
                   ImGuiWindowFlags_NoMove);
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Save", "Ctrl-S", nullptr)) {
        auto textToSave = editor->GetText();
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
      bool ro = editor->IsReadOnly();
      if (ImGui::MenuItem("Read-Only", nullptr, &ro))
        editor->SetReadOnly(ro);

      ImGui::Separator();

      if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr,
                          !ro && editor->CanUndo()))
        editor->Undo();
      if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor->CanRedo()))
        editor->Redo();

      ImGui::Separator();

      if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor->HasSelection()))
        editor->Copy();
      if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr,
                          !ro && editor->HasSelection()))
        editor->Cut();
      if (ImGui::MenuItem("Delete", "Del", nullptr,
                          !ro && editor->HasSelection()))
        editor->Delete();
      if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr,
                          !ro && ImGui::GetClipboardText() != nullptr))
        editor->Paste();

      ImGui::Separator();

      if (ImGui::MenuItem("Select all", nullptr, nullptr))
        editor->SetSelection(
            TextEditor::Coordinates(),
            TextEditor::Coordinates(editor->GetTotalLines(), 0));

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      if (ImGui::MenuItem("Dark Mode"))
        editor->SetPalette(TextEditor::GetDarkPalette());
      if (ImGui::MenuItem("Light Mode"))
        editor->SetPalette(TextEditor::GetLightPalette());
      if (ImGui::MenuItem("Retro Mode"))
        editor->SetPalette(TextEditor::GetRetroBluePalette());
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s ", cpos.mLine + 1,
              cpos.mColumn + 1, editor->GetTotalLines(),
              editor->IsOverwrite() ? "Ovr" : "Ins",
              editor->CanUndo() ? "*" : " ",
              editor->GetLanguageDefinition().mName.c_str());

  editor->Render("TextEditor");
  ImGui::End();
}
} // namespace QuakePrism
