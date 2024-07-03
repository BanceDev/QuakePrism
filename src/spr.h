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
#define IDSPRITEHEADER                                                         \
	(('P' << 24) + ('S' << 16) + ('D' << 8) + 'I') // little-endian "IDSP"
namespace QuakePrism::SPR {

typedef struct {
	int ident; // should be IDSPRITEHEADER
	int version;
	int type;
	float boundingradius;
	int width;
	int height;
	int numframes;
	float beamlength;
	int synctype;
} sprite_t;

typedef struct {
	int group;
	int origin[2];
	int width;
	int height;
} spriteframe_t;

bool OpenSprite(const char *filename);
void RemoveSpriteFrame();
void SpriteFrame2Img();
void Img2SpriteFrame();

} // namespace QuakePrism::SPR
