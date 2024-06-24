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

#include "resources.h"
#include <regex>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

struct Diagnostic {
	std::string file;
	int line;
	std::string type;
	std::string message;
};

static std::vector<Diagnostic> parseCompilerOutput(const std::string &output) {
	std::vector<Diagnostic> diagnostics;
	std::regex combinedRegex(
		R"(([^:]+):(\d+): (warning|error)(?: ([^:]+))?: (.+))");
	std::smatch match;

	// Use a string stream to read the output line by line
	std::istringstream outputStream(output);
	std::string line;

	while (std::getline(outputStream, line)) {
		if (std::regex_match(line, match, combinedRegex)) {
			Diagnostic diag;
			diag.file = match[1];
			diag.line = std::stoi(match[2]);
			diag.type = match[3];
			diag.message =
				std::string(match[4]) + ": " +
				std::string(match[5]); // Include the warning/error code
			diagnostics.push_back(diag);
		}
	}

	return diagnostics;
}

namespace QuakePrism {

std::string getCompilerOutputString() {
	std::string compilerOutput;
	chdir((baseDirectory / "src").string().c_str());
#ifdef _WIN32
	std::string command = "fteqcc64.exe 2>&1";
#else
	std::string command = "./fteqcc64 2>&1";
#endif

	FILE *pipe = popen(command.c_str(), "r");
	if (!pipe)
		return "";
	char buffer[128];
	while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
		compilerOutput += buffer;
	}
	pclose(pipe);
	chdir(baseDirectory.string().c_str());
	return compilerOutput;
}

void createTextEditorDiagnostics(TextEditor &editor) {
	std::vector<Diagnostic> diagnostics =
		parseCompilerOutput(getCompilerOutputString());
	TextEditor::ErrorMarkers markers;
	for (const auto &diag : diagnostics) {
		if (editor.GetFileName() == diag.file) {
			markers.erase(diag.line); // use latest warning if duplicate lines
			markers.insert(std::make_pair(diag.line, diag.message));
		}
	}
	editor.SetErrorMarkers(markers);
}
} // namespace QuakePrism
