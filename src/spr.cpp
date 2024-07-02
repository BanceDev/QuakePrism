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
#include <cstdio>
#include <cstdlib>

namespace QuakePrism::SPR {

// create the singleton instance
sprite_t spritefile;

bool OpenSprite(const char *filename) {
	FILE *file = fopen(filename, "rb");
	if (!file) {
		return false;
	}

	// Read the header
	fread(&spritefile, sizeof(sprite_t), 1, file);

	if (spritefile.ident != IDSPRITEHEADER) {
		fprintf(stderr, "Not a valid Quake sprite file.\n");
		fclose(file);
		return false;
	}

	// Read each frame
	for (int i = 0; i < spritefile.numframes; ++i) {
		spriteframe_t frame;
		fread(&frame, sizeof(spriteframe_t), 1, file);

		// Allocate memory for the pixel data and read it
		unsigned char *pixels =
			(unsigned char *)malloc(frame.width * frame.height);
		fread(pixels, frame.width * frame.height, 1, file);

		// Process the pixel data here
		// ...

		free(pixels);
	}

	fclose(file);
	return true;
}

} // namespace QuakePrism::SPR
