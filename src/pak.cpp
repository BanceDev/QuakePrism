/*
Copyright (C) 2024 Lance Borden
Copyright (c) 2012-2016 Yamagi Burmeister
			  2015-2017 Daniel Gibson

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

#include "pak.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <unistd.h>
#ifdef _WIN32
#include <direct.h>
#endif

const int HDR_LEN = 64;

const int DIR_FILENAME_LEN = 56;

/* Holds the pak header */
struct {
	char signature[4];
	int dir_offset;
	int dir_length;
} header;

namespace QuakePrism::PAK {
/*
 * Creates a directory tree.
 *
 *  *s -> The path to create. The last
 *        part (the file itself) is
 *        ommitted
 */
void mktree(const char *s) {
	char dir[128] = {0};
	int i;
	int sLen = strlen(s);

	strncpy(dir, s, sizeof(dir) - 1);

	for (i = 1; i < sLen; ++i) {
		if (dir[i] == '/') {
			dir[i] = '\0'; // this ends the string at this point and allows
						   // creating the directory up to here
#ifdef _WIN32
			_mkdir(dir);
#else
			mkdir(dir, 0700);
#endif
			dir[i] = '/'; // restore the / so we can can go on to create next
						  // directory
		}
	}
}

/*
 * Reads the pak file header and
 * stores it into the global struct
 * "header".
 *
 *  *fd -> A file descriptor holding
 *         the pack to be read.
 */
bool read_header(FILE *fd) {
	if (fread(header.signature, 4, 1, fd) != 1) {
		return false;
	}

	if (fread(&header.dir_offset, 4, 1, fd) != 1) {
		return false;
	}

	if (fread(&header.dir_length, 4, 1, fd) != 1) {
		return false;
	}

	if (strncmp(header.signature, "PACK", 4) != 0) {
		return false;
	}

	return true;
}

bool read_dir_entry(directory *entry, FILE *fd) {
	if (fread(entry->file_name, DIR_FILENAME_LEN, 1, fd) != 1)
		return false;
	if (fread(&(entry->file_pos), 4, 1, fd) != 1)
		return false;
	if (fread(&(entry->file_length), 4, 1, fd) != 1)
		return false;

	entry->compressed_length = 0;
	entry->is_compressed = 0;

	return true;
}

/*
 * Reads the directory of a pak file
 * into a linked list and returns
 * a pointer to the first element.
 *
 *  *fd -> a file descriptor holding
 *         holding the pak to be read
 */
directory *read_directory(FILE *fd, int *num_entries) {
	int i;
	int direntry_len = HDR_LEN;
	int num_dir_entries = header.dir_length / direntry_len;
	directory *dir = (directory *)calloc(num_dir_entries, sizeof(directory));

	if (dir == NULL) {
		return NULL;
	}

	/* Navigate to the directory */
	fseek(fd, header.dir_offset, SEEK_SET);

	for (i = 0; i < num_dir_entries; ++i) {
		directory *cur = &dir[i];

		if (!read_dir_entry(cur, fd)) {
			*num_entries = 0;
			free(dir);
			return NULL;
		}
	}

	*num_entries = num_dir_entries;
	return dir;
}

void extract_raw(FILE *in, directory *d) {
	FILE *out = fopen(d->file_name, "wb");
	if (out == NULL) {
		return;
	}

	// just copy the data from the .pak to the output file (in chunks for speed)
	int bytes_left = d->file_length;
	char buf[2048];

	fseek(in, d->file_pos, SEEK_SET);

	while (bytes_left >= sizeof(buf)) {
		fread(buf, sizeof(buf), 1, in);
		fwrite(buf, sizeof(buf), 1, out);
		bytes_left -= sizeof(buf);
	}
	if (bytes_left > 0) {
		fread(buf, bytes_left, 1, in);
		fwrite(buf, bytes_left, 1, out);
	}

	fclose(out);
}

/*
 * Extract the files from a pak.
 *
 *  *d -> a pointer to the first element
 *        of the pak directory
 *
 *  *fd -> a file descriptor holding
 *         the pak to be extracted
 */
void extract_files(FILE *fd, directory *dirs, int num_entries) {
	int i;

	for (i = 0; i < num_entries; ++i) {
		directory *d = &dirs[i];
		mktree(d->file_name);

		extract_raw(fd, d);
	}
}

bool ExtractPAK(const std::filesystem::path filename,
				std::filesystem::path *out_dir) {
	directory *d = NULL;
	FILE *fd = NULL;
	int list_only = 0;
	int num_entries = 0;

	/* Open the pak file */
	fd = fopen(filename.string().c_str(), "rb");
	if (fd == NULL) {
		return false;
	}

	/* Read the header */
	if (!read_header(fd)) {
		fclose(fd);
		return false;
	}

	/* Read the directory */
	d = read_directory(fd, &num_entries);
	if (d == NULL) {
		fclose(fd);
		return false;
	}

	if (out_dir != NULL) {
#ifdef _WIN32
		if (_chdir(out_dir->string().c_str()) != 0) {
#else
		if (chdir(out_dir->string().c_str()) != 0) {
#endif
			return false;
		}
	}

	if (!list_only) {
		/* And now extract the files */
		extract_files(fd, d, num_entries);
	}

	/* cleanup */
	fclose(fd);

	free(d);

	return true;
}
} // namespace QuakePrism::PAK
