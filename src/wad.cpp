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
#include "SDL_opengl.h"
#include "resources.h"
#include "util.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
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
			waddata_t data;
			data.width = pic.width;
			data.height = pic.height;
			data.isMip = false;
			data.name = currentWadEntries[i].name;
			currentWadData.push_back(data);
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
			waddata_t data;
			data.width = mipWidth;
			data.height = mipHeight;
			data.isMip = true;
			data.name = currentWadEntries[i].name;
			currentWadData.push_back(data);
			free(pixels);
		}
		free(lumpData);
	}

	return true;
}

// Helper to align length to 4-byte boundary
static int AlignLen(int len) {
    return (len + 3) & ~3;
}

// Write lump data and apply padding if necessary
static void WriteLumpData(std::ofstream &outFile, unsigned char* data, int length) {
    outFile.write(reinterpret_cast<char*>(data), length);

    int padding = AlignLen(length) - length;
    if (padding > 0) {
        static const unsigned char zeros[4] = {0};
        outFile.write(reinterpret_cast<const char*>(zeros), padding);
    }
}

// Finalize lump data
static void FinalizeLump(wadentry_t &entry, int dataLength) {
    entry.size = AlignLen(dataLength);  // Include padding in the size
    entry.dirsize = entry.size;
}

static unsigned char* generateQpicIndices(int idx, int width, int height) {
	unsigned char *rgbaPixels = GetTexturePixels(currentWadTexs[idx], width, height, GL_RGBA, GL_UNSIGNED_BYTE);

	// Allocate memory for indexed color data
	unsigned char *indices = new unsigned char[width * height];

	// Convert RGBA to indexed color
	convertRGBAToIndices(rgbaPixels, indices, width * height);

	free(rgbaPixels);
	return indices;
}

static unsigned char* generateMipIndices(int idx, int width, int height, int mipLevel) {
    int mipWidth = width >> mipLevel;
    int mipHeight = height >> mipLevel;

    if (mipWidth <= 0) mipWidth = 1;
    if (mipHeight <= 0) mipHeight = 1;


	unsigned char* originalData = generateQpicIndices(idx, width, height);

    unsigned char* mipData = new unsigned char[mipWidth * mipHeight];

    for (int y = 0; y < mipHeight; ++y) {
        for (int x = 0; x < mipWidth; ++x) {
            // Simple nearest neighbor downsampling
            int srcX = x << mipLevel;
            int srcY = y << mipLevel;

            mipData[y * mipWidth + x] = originalData[srcY * width + srcX];
        }
    }
	delete[] originalData;
    return mipData;
}


// Main function to write the WAD file
bool WriteWad(const char *filename) {
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile.is_open()) {
        return false;
    }

    // Write a dummy WAD header
    wad_t dummyHeader = {};
    outFile.write(reinterpret_cast<char*>(&dummyHeader), sizeof(wad_t));

    std::vector<wadentry_t> directoryEntries(currentWadData.size());

    for (size_t i = 0; i < currentWadData.size(); ++i) {
        waddata_t &data = currentWadData[i];
        wadentry_t entry = {};

        entry.offset = static_cast<int>(outFile.tellp()); // Record the current offset

        if (data.isMip) {
            // Handle miptex lump
            miptex_t miptex = {};
            strncpy(miptex.name, data.name.c_str(), 16);
            miptex.width = data.width;
            miptex.height = data.height;

            int pixelCount = data.width * data.height;
            int mipSizes[4] = {pixelCount, (data.width / 2) * (data.height / 2), (data.width / 4) * (data.height / 4), (data.width / 8) * (data.height / 8)};
            int dataOffset = sizeof(miptex_t);

            for (int mip = 0; mip < 4; ++mip) {
                miptex.offsets[mip] = dataOffset;
                dataOffset += AlignLen(mipSizes[mip]);
            }

            // Write miptex header
            outFile.write(reinterpret_cast<char*>(&miptex), sizeof(miptex_t));

            // Write mip levels with padding
            for (int mip = 0; mip < 4; ++mip) {
                unsigned char* mipIndices = generateMipIndices(i, data.width, data.height, mip);
                WriteLumpData(outFile, mipIndices, mipSizes[mip]);
                delete[] mipIndices;
            }

            FinalizeLump(entry, dataOffset);

        } else {
            // Handle qpic lump
            qpic_t qpic = {};
            qpic.width = data.width;
            qpic.height = data.height;

            int pixelCount = data.width * data.height;

            // Write qpic header
            outFile.write(reinterpret_cast<char*>(&qpic), sizeof(qpic_t));

            unsigned char* indices = generateQpicIndices(i, data.width, data.height);
            WriteLumpData(outFile, indices, pixelCount);
            delete[] indices;

			FinalizeLump(entry, sizeof(qpic_t) + AlignLen(pixelCount));
        }

        entry.dirsize = entry.size;
        entry.type = data.isMip ? 'D' : 'B';
        entry.compression = 0;
        entry.dummy = 0;
        strncpy(entry.name, data.name.c_str(), 16);

        directoryEntries[i] = entry;
    }

    // Write directory at the end of the file
    int directoryOffset = static_cast<int>(outFile.tellp());
    outFile.write(reinterpret_cast<char*>(directoryEntries.data()), sizeof(wadentry_t) * directoryEntries.size());

    // Correct WAD header
    currentWad.offset = directoryOffset;
    currentWad.numEntries = directoryEntries.size();

    // Rewind and write the correct header
    outFile.seekp(0, std::ios::beg);
    outFile.write(reinterpret_cast<char*>(&currentWad), sizeof(wad_t));

    // Close the file
    outFile.close();
    return true;
}

void InsertImage(std::filesystem::path filename, const bool isMip) {
	int width, height;
	unsigned int texID;
	LoadTextureFromFile(filename.string().c_str(), &texID, &width, &height);
	waddata_t data;
	data.width = width;
	data.height = height;
	data.isMip = isMip;
	filename.replace_extension("");
	data.name = filename.filename().string();
	currentWadData.push_back(data);
	currentWadTexs.push_back(texID);
}

void ExportAsImages() {
	std::filesystem::path outDir = currentWadPath.parent_path();	
	outDir /= currentWadPath.filename();
	outDir.replace_extension("");
	// Create the output directory if it does not exist
	if (!std::filesystem::exists(outDir)) {
		std::filesystem::create_directory(outDir);
	}
	for (int i = 0; i < currentWadTexs.size(); ++i) {
		std::filesystem::path outFile = outDir / currentWadData[i].name;
		outFile.replace_extension(".png");
		int width = currentWadData[i].width;
		int height = currentWadData[i].height;
		unsigned char *pixels = GetTexturePixels(currentWadTexs[i], width, height);
		convertRGBAToImage(outFile.string().c_str(), pixels, width, height);
	}	

}

void ExportImage(const int index) {
	std::string filename = currentWadPath.parent_path().string();
	filename += "/";
	filename += currentWadData[index].name;
	filename += ".png";
	int width = currentWadData[index].width;
	int height = currentWadData[index].height;
	unsigned char *pixels = GetTexturePixels(currentWadTexs[index], width, height);
	convertRGBAToImage(filename.c_str(), pixels, width, height);	
}

void RemoveImage(const int index) {
	unsigned int texID = currentWadTexs[index];
	currentWadTexs.erase(currentWadTexs.begin() + index);
	currentWadData.erase(currentWadData.begin() + index);
	glDeleteTextures(1, &texID);
}

void NewWadFromImages(std::vector<std::filesystem::path> files, const bool isMip) {
	CleanupWad();
	for (auto &file : files) {
		InsertImage(file.string().c_str(), isMip);
	}
	currentWadPath = baseDirectory / files[0].filename();
	currentWadPath = currentWadPath.replace_extension(".wad");
}

void CleanupWad() {
	for (auto &texID : currentWadTexs) {
		glDeleteTextures(1, &texID);
	}
	currentWadTexs.clear();
	currentWadData.clear();
	currentWadEntries.clear();
}

} // namespace QuakePrism::WAD
