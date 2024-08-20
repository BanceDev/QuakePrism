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

static unsigned char* DownscaleMip(unsigned char* srcPixels, int srcWidth, int srcHeight, int destWidth, int destHeight) {
    unsigned char* destPixels = new unsigned char[destWidth * destHeight * 4]; // RGBA
    for (int y = 0; y < destHeight; ++y) {
        for (int x = 0; x < destWidth; ++x) {
            int srcX = x * 2;
            int srcY = y * 2;
            int srcIndex = (srcY * srcWidth + srcX) * 4;
            int destIndex = (y * destWidth + x) * 4;

            // Average the color of 2x2 block
            for (int i = 0; i < 4; ++i) {
                destPixels[destIndex + i] = (
                    srcPixels[srcIndex + i] +
                    srcPixels[srcIndex + 4 + i] +
                    srcPixels[srcIndex + srcWidth * 4 + i] +
                    srcPixels[srcIndex + srcWidth * 4 + 4 + i]
                ) / 4;
            }
        }
    }
    return destPixels;
}

bool WriteWad(const char *filename) {
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile.is_open()) {
        return false; // Failed to open the file
    }

    // Initialize WAD header
    currentWad.id = WADID;
    currentWad.numEntries = static_cast<int>(currentWadData.size());
    currentWad.offset = sizeof(wad_t) + sizeof(wadentry_t) * currentWad.numEntries;

    outFile.write(reinterpret_cast<char*>(&currentWad), sizeof(wad_t));

    // Placeholder for the directory
    std::vector<wadentry_t> directoryEntries(currentWadData.size());

    // Write texture data and build the directory
    for (size_t i = 0; i < currentWadData.size(); ++i) {
        waddata_t &data = currentWadData[i];
        wadentry_t entry = {};

        entry.offset = static_cast<int>(outFile.tellp()); // Capture the current position in the file
        entry.type = data.isMip ? 'D' : 'B'; // Assuming 'D' for miptex, 'B' for qpic
        entry.compression = 0; // No compression
        strncpy(entry.name, data.name.c_str(), 16); // Set the name of the texture

        int pixelCount = data.width * data.height;

        if (data.isMip) {
            // Handle miptex

            // Retrieve RGBA data from the OpenGL texture
            unsigned char *rgbaPixels = GetTexturePixels(currentWadTexs[i], data.width, data.height, GL_RGBA, GL_UNSIGNED_BYTE);

            // Allocate memory for indexed color data
            unsigned char *indices = new unsigned char[pixelCount];

            // Convert RGBA to indexed color
            convertRGBAToIndices(rgbaPixels, indices, pixelCount);

            // Create miptex structure
            miptex_t miptex;
            memset(&miptex, 0, sizeof(miptex_t));
            strncpy(miptex.name, data.name.c_str(), 16);
            miptex.width = data.width;
            miptex.height = data.height;

            int mipSizes[4] = {pixelCount,
                               (data.width / 2) * (data.height / 2),
                               (data.width / 4) * (data.height / 4),
                               (data.width / 8) * (data.height / 8)};

            int dataOffset = sizeof(miptex_t);
            for (int mip = 0; mip < 4; ++mip) {
                miptex.offsets[mip] = dataOffset;
                dataOffset += mipSizes[mip];
            }

            // Write miptex header
            outFile.write(reinterpret_cast<char*>(&miptex), sizeof(miptex_t));

            // Write the indexed color data for each mip level
            unsigned char* currentPixels = rgbaPixels;
            int currentWidth = data.width;
            int currentHeight = data.height;

            for (int mip = 0; mip < 4; ++mip) {
                unsigned char* mipIndices = new unsigned char[mipSizes[mip]];
                convertRGBAToIndices(currentPixels, mipIndices, mipSizes[mip]);

                outFile.write(reinterpret_cast<char*>(mipIndices), mipSizes[mip]);

                delete[] mipIndices;

                if (mip < 3) { // Downscale for next mip level
                    unsigned char* nextPixels = DownscaleMip(currentPixels, currentWidth, currentHeight, currentWidth / 2, currentHeight / 2);
                    if (mip > 0) {
                        delete[] currentPixels;
                    }
                    currentPixels = nextPixels;
                    currentWidth /= 2;
                    currentHeight /= 2;
                }
            }

            if (currentPixels != rgbaPixels) {
                delete[] currentPixels;
            }

            entry.size = dataOffset;

            // Clean up
            delete[] indices;
            free(rgbaPixels);

        } else {
            // Handle qpic

            // Retrieve RGBA data from the OpenGL texture
            unsigned char *rgbaPixels = GetTexturePixels(currentWadTexs[i], data.width, data.height, GL_RGBA, GL_UNSIGNED_BYTE);

            // Allocate memory for indexed color data
            unsigned char *indices = new unsigned char[pixelCount];

            // Convert RGBA to indexed color
            convertRGBAToIndices(rgbaPixels, indices, pixelCount);

            // Create qpic structure
            qpic_t qpic;
            qpic.width = data.width;
            qpic.height = data.height;

            // Write qpic header
            outFile.write(reinterpret_cast<char*>(&qpic), sizeof(qpic_t));

            // Write the indexed color data
            outFile.write(reinterpret_cast<char*>(indices), pixelCount);

            entry.size = sizeof(qpic_t) + pixelCount;

            // Clean up
            delete[] indices;
            free(rgbaPixels);
        }

        entry.dirsize = entry.size;

        // Add the directory entry to the list
        directoryEntries[i] = entry;
    }

    // Write the directory at the end of the file
    outFile.seekp(currentWad.offset, std::ios::beg);
    outFile.write(reinterpret_cast<char*>(directoryEntries.data()), sizeof(wadentry_t) * directoryEntries.size());

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
