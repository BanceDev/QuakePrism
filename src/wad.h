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

#include <vector>
#include <filesystem>

#pragma once
#define WADID                                                                  \
	(('2' << 24) + ('D' << 16) + ('A' << 8) + 'W') // little-endian "WAD2"
namespace QuakePrism::WAD {

typedef struct {
	int id;
	int numEntries;
	int offset;
} wad_t;

typedef struct {
	int offset;
	int dirsize;
	int size;
	char type;
	char compression;
	short dummy;
	char name[16];
} wadentry_t;

typedef struct {
	char name[16];
	int width, height;
	int offsets[4];
} miptex_t;

typedef struct {
	int width, height;
} qpic_t;

typedef struct {
	int width, height;
	bool isMip;
	std::string name;
} waddata_t;

bool OpenWad(const char *filename);
bool WriteWad(const char *filename);
void InsertImage(std::filesystem::path filename, const bool isMip);
void ExportAsImages();
void ExportImage(const int index);
void RemoveImage(const int index);
void NewWadFromImages(std::vector<std::filesystem::path> files, const bool isMip);
void CleanupWad();

} // namespace QuakePrism::WAD
