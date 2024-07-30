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

#include "wad.h"
#include "resources.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace QuakePrism::WAD {

static void QPic2Tex(unsigned char *pixels, unsigned int &texID, int imgWidth,
					 int imgHeight) {
	// Create a OpenGL texture identifier
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
					GL_CLAMP_TO_EDGE); // This is required on WebGL for non
									   // power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA,
				 GL_UNSIGNED_BYTE, pixels);
}

bool OpenWad(const char *filename) {
	std::ifstream file(filename, std::ios::binary);
	if (!file) {
		std::cerr << "Failed to open file: " << filename << std::endl;
		return false;
	}

	// Read the header
	file.read(reinterpret_cast<char *>(&currentWad), sizeof(wad_t));
	if (!file) {
		std::cerr << "Failed to read WAD header." << std::endl;
		return false;
	}

	// Resize the vector to hold the directory entries
	currentWadEntries.resize(currentWad.numEntries);

	// Seek to the directory and read it
	file.seekg(currentWad.offset, std::ios::beg);
	file.read(reinterpret_cast<char *>(currentWadEntries.data()),
			  currentWad.numEntries * sizeof(wadentry_t));
	if (!file) {
		std::cerr << "Failed to read WAD directory." << std::endl;
		return false;
	}

	// Read the lump data
	for (size_t i = 0; i < currentWadEntries.size(); ++i) {
		unsigned char *lumpData =
			(unsigned char *)malloc(currentWadEntries[i].dirsize);

		file.seekg(currentWadEntries[i].offset, std::ios::beg);
		file.read(reinterpret_cast<char *>(lumpData),
				  currentWadEntries[i].dirsize);
		if (!file) {
			std::cerr << "Failed to read lump data for "
					  << currentWadEntries[i].name << std::endl;
			return false;
		}

		if (currentWadEntries[i].type == 'B' ||
			currentWadEntries[i].type == 'E') {
			qpic_t pic;
			memcpy(&pic, lumpData, sizeof(qpic_t));
			unsigned char *pixels =
				(unsigned char *)malloc(pic.width * pic.height * 4);
			for (int j = 0; j < pic.width * pic.height; ++j) {
				const int idx = sizeof(qpic_t) + j;
				const int colorIndex = lumpData[idx];
				pixels[(j * 4) + 0] = colormap[colorIndex][0];
				pixels[(j * 4) + 1] = colormap[colorIndex][1];
				pixels[(j * 4) + 2] = colormap[colorIndex][2];
				if (colorIndex == 255) {
					pixels[(j * 4) + 3] = 0;
				} else {
					pixels[(j * 4) + 3] = 255;
				}
			}
			unsigned int texID;
			QPic2Tex(pixels, texID, pic.width, pic.height);
			currentWadTexs.push_back(texID);
			free(pixels);
		} else if (currentWadEntries[i].type == 'D') {
			miptex_t miptex;
			memcpy(&miptex, lumpData, sizeof(miptex_t));

			// The largest miptex is the first one, offset is in
			// miptex.offsets[0]
			int mipWidth = miptex.width;
			int mipHeight = miptex.height;

			unsigned char *pixels =
				(unsigned char *)malloc(mipWidth * mipHeight * 4);
			for (int j = 0; j < mipWidth * mipHeight; ++j) {
				const int idx = sizeof(miptex_t) + j;
				const int colorIndex = lumpData[idx];
				pixels[(j * 4) + 0] = colormap[colorIndex][0];
				pixels[(j * 4) + 1] = colormap[colorIndex][1];
				pixels[(j * 4) + 2] = colormap[colorIndex][2];
				if (colorIndex == 255) {
					pixels[(j * 4) + 3] = 0;
				} else {
					pixels[(j * 4) + 3] = 255;
				}
			}
			unsigned int texID;
			QPic2Tex(pixels, texID, mipWidth, mipHeight);
			currentWadTexs.push_back(texID);

			free(pixels);
		}
		free(lumpData);
	}

	return true;
}

bool WriteWad(const char *filename) { return false; }

void InsertLump(const char *filename) {}

void ExportAsLumps() {}

void ExportAsImages() {}

void NewWadFromLumps() {}

void NewWadFromImages() {}
} // namespace QuakePrism::WAD
