# Quake Prism

Quake Prism is an all in one development solution for vanilla Quake engine games and mods based on modern game engine tools.

## Features
- Model Viewer
- Texture Viewer
- WAD Editor
- PAK Import/Export
- File Explorer
- QuakeC Editor
- QuakeC Project Templates
- In depth docs

## Releases
- Binary builds are available from [releases](https://github.com/BanceDev/QuakePrism/releases)

## Compiling
Clone the repo recursively to install imgui dependencies
```
git clone --recursive
```
Other dependencies include sld2, glew, and glm. These can either be installed using the packagage manager for your respective Linux distro, or if you are on windows by downloading the installers for those libraries.
Run the make command in the QuakePrism directory and it will output to a build directory. The makefile currently supports Linux fully, for Windows developers it is reccomended to use the Visual Studio project instead.

## Contributing
- For bug reports and feature suggestions please use [issues](https://github.com/BanceDev/QuakePrism/issues).
- If you wish to contribute code of your own please submit a [pull request](https://github.com/BanceDev/QuakePrism/pulls).
- Note: It is likely best to submit an issue before a PR to see if the feature is wanted before spending time making a commit.
- All help is welcome!

### Credits
- [Dear ImGui](https://github.com/ocornut/imgui)
- [Ubuntu Font](https://design.ubuntu.com/font)
- [YamagiQ2 PakExtract](https://github.com/yquake2/pakextract)
