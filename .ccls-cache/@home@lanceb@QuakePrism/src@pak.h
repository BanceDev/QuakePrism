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
#include <filesystem>

/* Holds the pak header */
struct header;

/* A directory entry */
typedef struct {
	char file_name[120];
	int file_pos;
	int file_length; // in case of is_compressed: size after decompression

	// the following two are only used by daikatana
	int compressed_length; // size of compressed data in the pak
	int is_compressed;	   // 0: uncompressed, else compressed

} directory;

namespace QuakePrism::PAK {
void mktree(const char *s);

bool read_header(FILE *fd);

bool read_dir_entry(directory *entry, FILE *fd);

directory *read_directory(FILE *fd, int *num_entries);

void extract_raw(FILE *in, directory *d);

void extract_files(FILE *fd, directory *dirs, int num_entries);

bool ExtractPAK(const std::filesystem::path filename,
				std::filesystem::path *out_dir);

} // namespace QuakePrism::PAK
