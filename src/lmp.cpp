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
#include "lmp.h"
#include "stb_image.h"
#include "util.h"
#include <cstdio>
#include <cstdlib>

namespace QuakePrism::LMP {

void Img2Lmp(std::filesystem::path filename) {
	int imgWidth = 0;
	int imgHeight = 0;
	unsigned char *img = stbi_load(filename.string().c_str(), &imgWidth,
								   &imgHeight, NULL, STBI_rgb_alpha);
	if (img == NULL) {
		return;
	}

	// Allocate memory for the pixels array
	unsigned char *pixels = (unsigned char *)malloc(imgWidth * imgHeight);
	if (pixels == NULL) {
		stbi_image_free(img);
		return;
	}

	// Convert the RGB image to Quake palette indices
	convertRGBAToIndices(img, pixels, imgWidth * imgHeight);

	// Write out the lump file
	filename.replace_extension(".lmp");
	FILE *fp = fopen(filename.string().c_str(), "wb");
	if (!fp) {
		free(pixels);
		stbi_image_free(img);
		return;
	}

	// Write header
	fwrite(&imgWidth, sizeof(int), 1, fp);
	fwrite(&imgHeight, sizeof(int), 1, fp);

	// Write pixel data
	fwrite(pixels, sizeof(unsigned char), imgWidth * imgHeight, fp);

	// Clean up
	fclose(fp);
	free(pixels);
	stbi_image_free(img);
}
} // namespace QuakePrism::LMP
