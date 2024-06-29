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

#include <filesystem>

namespace QuakePrism::LMP {

void Img2Lmp(std::filesystem::path filename);
void Lmp2Img(std::filesystem::path filename);
void Lmp2Tex(std::filesystem::path filename, unsigned int *texID, int *width,
			 int *height);

} // namespace QuakePrism::LMP
