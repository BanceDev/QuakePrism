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
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstdlib>

namespace QuakePrism::SPR {

static void SpriteFrame2Tex(unsigned char *pixels, unsigned int &texID,
							int imgWidth, int imgHeight) {
	// Create a OpenGL texture identifier
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA,
				 GL_UNSIGNED_BYTE, pixels);
}

bool OpenSprite(const char *filename) {
	FILE *file = fopen(filename, "rb");
	if (!file) {
		return false;
	}

	// Read the header
	fread(&currentSprite, sizeof(sprite_t), 1, file);

	if (currentSprite.ident != IDSPRITEHEADER) {
		fclose(file);
		return false;
	}

	// Read each frame
	currentSpriteFrames.clear();
	for (int i = 0; i < currentSprite.numframes; ++i) {
		spriteframe_t frame;
		fread(&frame, sizeof(spriteframe_t), 1, file);

		// only support regular sprites at this time
		if (frame.group != 0) {
			fclose(file);
			return false;
		}
		// Allocate memory for the pixel data and read it
		unsigned char *indicies =
			(unsigned char *)malloc(frame.width * frame.height);
		fread(indicies, frame.width * frame.height, 1, file);
		unsigned char *pixels =
			(unsigned char *)malloc(frame.width * frame.height * 4);
		for (int i = 0; i < frame.width * frame.height; ++i) {
			int colorIndex = indicies[i];
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
		currentSpriteFrames.push_back(texID);

		free(indicies);
		free(pixels);
	}

	fclose(file);
	return true;
}

} // namespace QuakePrism::SPR
