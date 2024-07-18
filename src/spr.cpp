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

#include "spr.h"
#include "resources.h"
#include "stb_image.h"
#include "util.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>

namespace QuakePrism::SPR {

static void SpriteFrame2Tex(unsigned char *pixels, unsigned int &texID,
							int imgWidth, int imgHeight) {
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

bool OpenSprite(const char *filename) {
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		return false;
	}

	// Read the header
	fread(&currentSprite, sizeof(sprite_t), 1, fp);

	if (currentSprite.ident != IDSPRITEHEADER) {
		fclose(fp);
		return false;
	}

	// Read each frame
	currentSpriteTexs.clear();
	for (int i = 0; i < currentSprite.numframes; ++i) {
		spriteframe_t frame;
		fread(&frame, sizeof(spriteframe_t), 1, fp);

		// only support regular sprites at this time
		if (frame.group != 0) {
			fclose(fp);
			return false;
		}
		// Allocate memory for the pixel data and read it
		unsigned char *indices =
			(unsigned char *)malloc(frame.width * frame.height);
		fread(indices, frame.width * frame.height, 1, fp);
		unsigned char *pixels =
			(unsigned char *)malloc(frame.width * frame.height * 4);
		for (int i = 0; i < frame.width * frame.height; ++i) {
			int colorIndex = indices[i];
			pixels[(i * 4) + 0] = colormap[colorIndex][0];
			pixels[(i * 4) + 1] = colormap[colorIndex][1];
			pixels[(i * 4) + 2] = colormap[colorIndex][2];
			if (colorIndex != 255) {
				pixels[(i * 4) + 3] = 255;
			} else {
				pixels[(i * 4) + 3] = 0;
			}
		}

		unsigned int texID;
		SpriteFrame2Tex(pixels, texID, frame.width, frame.height);
		currentSpriteTexs.push_back(texID);
		currentSpriteFrames.push_back(frame);

		free(indices);
		free(pixels);
	}

	fclose(fp);
	return true;
}

bool WriteSprite(const char *filename) {
	FILE *fp = fopen(filename, "wb");
	if (!fp) {
		return false;
	}

	// Write the header
	fwrite(&currentSprite, sizeof(sprite_t), 1, fp);

	// Write each frame
	for (int i = 0; i < currentSprite.numframes; ++i) {
		fwrite(&currentSpriteFrames[i], sizeof(spriteframe_t), 1, fp);

		int width = currentSpriteFrames[i].width;
		int height = currentSpriteFrames[i].height;
		unsigned char *pixels =
			GetTexturePixels(currentSpriteTexs[i], width, height);
		unsigned char *indices = (unsigned char *)malloc(width * height);
		convertRGBAToIndices(pixels, indices, width * height);
		fwrite(indices, width * height, 1, fp);

		free(indices);
		free(pixels);
	}

	fclose(fp);
	return true;
}

void InsertFrame(const char *filename) {
	int width = 0;
	int height = 0;
	unsigned char *img =
		stbi_load(filename, &width, &height, NULL, STBI_rgb_alpha);
	if (img == NULL) {
		return;
	}

	spriteframe_t frame;
	frame.group = 0;
	frame.width = width;
	frame.height = height;
	frame.origin[0] = width / 2;
	frame.origin[1] = height / 2;

	// update max dimensions if needed
	if (width > currentSprite.width) {
		currentSprite.width = width;
	}
	if (height > currentSprite.width) {
		currentSprite.height = height;
	}

	unsigned int texID;
	SpriteFrame2Tex(img, texID, width, height);

	currentSpriteFrames.insert(currentSpriteFrames.begin() + activeSpriteFrame,
							   frame);
	currentSpriteTexs.insert(currentSpriteTexs.begin() + activeSpriteFrame,
							 texID);
	currentSprite.numframes++;

	stbi_image_free(img);
}

void ExportSpriteFrames() {
	for (int i = 0; i < currentSprite.numframes; ++i) {
		int width = currentSpriteFrames[i].width;
		int height = currentSpriteFrames[i].height;
		unsigned char *pixels =
			GetTexturePixels(currentSpriteTexs[i], width, height);
		unsigned char *indices = (unsigned char *)malloc(width * height);
		convertRGBAToIndices(pixels, indices, width * height);

		std::string imgFilename = currentSpritePath.parent_path().string() +
								  "/" + currentSpritePath.stem().string();
		if (currentSprite.numframes > 1) {
			imgFilename += "_" + std::to_string(i + 1);
		}
		imgFilename += ".png";

		convertRGBAToImage(imgFilename.c_str(), pixels, width, height);
		// Clean up
		free(pixels);
		free(indices);
	}
}

static void AddFrame(const char *filename) {
	int width = 0;
	int height = 0;
	unsigned char *img =
		stbi_load(filename, &width, &height, NULL, STBI_rgb_alpha);
	if (img == NULL) {
		return;
	}
	spriteframe_t frame;
	frame.group = 0;
	frame.width = width;
	frame.height = height;
	frame.origin[0] = width / 2;
	frame.origin[1] = height / 2;

	// update max dimensions if needed
	if (width > currentSprite.width) {
		currentSprite.width = width;
	}
	if (height > currentSprite.width) {
		currentSprite.height = height;
	}

	unsigned int texID;
	SpriteFrame2Tex(img, texID, width, height);

	currentSpriteFrames.push_back(frame);
	currentSpriteTexs.push_back(texID);
	currentSprite.numframes++;

	stbi_image_free(img);
}

void NewSpriteFromFrames(std::vector<std::filesystem::path> framePaths) {
	// init currentSprite
	currentSprite.ident = IDSPRITEHEADER;
	currentSprite.version = 1;
	currentSprite.type = 0;
	currentSprite.boundingradius = 0.0f;
	currentSprite.width = 0;
	currentSprite.height = 0;
	currentSprite.numframes = 0;
	currentSprite.beamlength = 0.0f;
	currentSprite.synctype = 0;

	currentSpriteFrames.clear();
	currentSpriteTexs.clear();

	currentSpritePath = framePaths.at(0);
	currentSpritePath.replace_extension(".spr");
	for (auto &path : framePaths) {
		AddFrame(path.string().c_str());
	}
}

} // namespace QuakePrism::SPR
