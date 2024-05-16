#include "TextEditor.h"
#include "mdl.h"
#include <GL/gl.h>
#include <filesystem>

namespace QuakePrism {

void DrawMenuBar();

void DrawModelViewer(GLuint &texture_id, GLuint &RBO, GLuint &FBO);

void DrawTextEditor(TextEditor &editor);

void DrawErrorPopup();

void DrawAboutPopup();

void DrawFileTree(const std::filesystem::path &currentPath, TextEditor &editor);

void DrawFileExplorer(TextEditor &editor);

void DrawOpenProjectPopup();

} // namespace QuakePrism
