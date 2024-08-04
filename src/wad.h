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

bool OpenWad(const char *filename);
bool WriteWad(const char *filename);
void InsertImage(const char *filename);
void ExportAsImages();
void ExportImage(int index);
void NewWadFromImages();
void CleanupWad();

} // namespace QuakePrism::WAD
