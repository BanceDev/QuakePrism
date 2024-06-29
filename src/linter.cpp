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

#include "TextEditor.h"
#include "resources.h"
#include <iostream>
#include <regex>
#include <sstream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <vector>
#ifdef _WIN32
#include <direct.h>
#endif

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
		std::cout << line << std::endl;
		if (std::regex_match(line, match, combinedRegex)) {
			std::cout << "match" << std::endl;
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
#ifdef _WIN32
	// Change directory to src
	_chdir((baseDirectory / "src").string().c_str());

	// Set up the process start information
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES sa;
	HANDLE g_hChildStd_OUT_Rd = NULL;
	HANDLE g_hChildStd_OUT_Wr = NULL;
	DWORD dwRead;
	CHAR chBuf[128];
	BOOL bSuccess = FALSE;

	// Set up the security attributes struct.
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT.
	if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &sa, 0))
		return "";

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
		return "";

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.hStdError = g_hChildStd_OUT_Wr;
	si.hStdOutput = g_hChildStd_OUT_Wr;
	si.dwFlags |= STARTF_USESTDHANDLES;

	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

	std::wstring command = L"fteqcc64.exe";
	// Create the child process.
	if (!CreateProcess(NULL,
					   (LPWSTR)(command.c_str()), // command line
					   NULL,					  // process security attributes
					   NULL,			 // primary thread security attributes
					   TRUE,			 // handles are inherited
					   CREATE_NO_WINDOW, // creation flags
					   NULL,			 // use parent's environment
					   NULL,			 // use parent's current directory
					   &si,				 // STARTUPINFO pointer
					   &pi))			 // receives PROCESS_INFORMATION
	{
		CloseHandle(g_hChildStd_OUT_Wr);
		return "";
	}

	// Close the write end of the pipe before reading from the read end of the
	// pipe.
	CloseHandle(g_hChildStd_OUT_Wr);

	// Read output from the child process's pipe for STDOUT
	for (;;) {
		bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, 128, &dwRead, NULL);
		if (!bSuccess || dwRead == 0)
			break;
		compilerOutput.append(chBuf, dwRead);
	}

	// Close handles
	CloseHandle(g_hChildStd_OUT_Rd);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	// Change back to the base directory
	_chdir(baseDirectory.string().c_str());
#else
	chdir((baseDirectory / "src").string().c_str());
	std::string command = "./fteqcc64 2>&1";
	FILE *pipe = popen(command.c_str(), "r");
	if (!pipe)
		return "";
	char buffer[128];
	while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
		compilerOutput += buffer;
	}
	pclose(pipe);
	chdir(baseDirectory.string().c_str());
#endif
	// Normalize line endings
	std::string::size_type pos = 0;
	while ((pos = compilerOutput.find("\r\n", pos)) != std::string::npos) {
		compilerOutput.replace(pos, 2, "\n");
		pos += 1;
	}

	return compilerOutput;
}

void createTextEditorDiagnostics() {
	std::string compilerOutput = getCompilerOutputString();
	std::vector<Diagnostic> diagnostics = parseCompilerOutput(compilerOutput);
	for (auto &editor : editorList) {
		TextEditor::ErrorMarkers markers;
		for (const auto &diag : diagnostics) {
			std::cout << "Diag" << std::endl;
			if (editor.GetFileName() == diag.file && !editor.IsUnsaved()) {
				markers.erase(
					diag.line); // use latest warning if duplicate lines
				markers.insert(std::make_pair(diag.line, diag.message));
			}
		}
		editor.SetErrorMarkers(markers);
	}
}
} // namespace QuakePrism
