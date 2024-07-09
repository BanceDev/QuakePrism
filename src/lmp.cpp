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
#include "resources.h"
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

void Lmp2Img(std::filesystem::path filename) {
	// TOFIX: This check is pretty brute force. Should probabaly be done with
	// header
	if (filename.filename() == "colormap.lmp" ||
		filename.filename() == "palette.lmp")
		return;
	int imgWidth, imgHeight;
	FILE *fp = fopen(filename.string().c_str(), "rb");
	if (!fp)
		return;
	// Read header
	fread(&imgWidth, sizeof(int), 1, fp);
	fread(&imgHeight, sizeof(int), 1, fp);

	unsigned char *indices = (unsigned char *)malloc(imgWidth * imgHeight);
	// Read pixel data
	fread(indices, sizeof(unsigned char), imgWidth * imgHeight, fp);
	unsigned char *pixels = (unsigned char *)malloc(imgWidth * imgHeight * 4);
	for (int i = 0; i < imgWidth * imgHeight; ++i) {
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
	filename.replace_extension(".png");
	convertRGBAToImage(filename.string().c_str(), pixels, imgWidth, imgHeight);

	// Clean up
	fclose(fp);
	free(pixels);
	free(indices);
}

void Lmp2Tex(std::filesystem::path filename, unsigned int *texID, int *width,
			 int *height) {
	// header
	if (filename.filename() == "colormap.lmp" ||
		filename.filename() == "palette.lmp")
		return;
	int imgWidth, imgHeight;
	FILE *fp = fopen(filename.string().c_str(), "rb");
	if (!fp)
		return;
	// Read header
	fread(&imgWidth, sizeof(int), 1, fp);
	fread(&imgHeight, sizeof(int), 1, fp);

	unsigned char *indices = (unsigned char *)malloc(imgWidth * imgHeight);
	// Read pixel data
	fread(indices, sizeof(unsigned char), imgWidth * imgHeight, fp);
	unsigned char *pixels = (unsigned char *)malloc(imgWidth * imgHeight * 4);
	for (int i = 0; i < imgWidth * imgHeight; ++i) {
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

	// Create a OpenGL texture identifier
	GLuint imgTex;
	glGenTextures(1, &imgTex);
	glBindTexture(GL_TEXTURE_2D, imgTex);

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

	if (texID != nullptr)
		*texID = imgTex;
	if (width != nullptr)
		*width = imgWidth;
	if (height != nullptr)
		*height = imgHeight;

	fclose(fp);
	free(pixels);
	free(indices);
}

} // namespace QuakePrism::LMP
