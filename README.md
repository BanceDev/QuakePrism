<p align="center">
    <img src="https://github.com/BanceDev/QuakePrism/blob/main/logo.png" alt="Logo for Quake Prism" width="256" height="256"/>
</p>

# Quake Prism

Quake Prism is an all in one development solution for vanilla Quake engine games and mods based on modern game engine tools.
The primary focus of this project is to enable not only modders but also those interested in making fully original games with the Quake engine. Most of the tooling is basic asset manipulation along with a QuakeC editor.
Quake Prism also ships with project templates to help you start development right away along with including fteqcc64 for compilation and Ironwail for the source port to run your mods/games from the editor.

Quake Prism will not come packaged with tooling for 3D modeling see this [Quake blender plugin](https://github.com/victorfeitosa/quake-hexen2-mdl-export-import) for that. Quake Prism also will not include mapping tools due largely to the complexity of the requirements for map tooling along with the fact that highly featured modern mapping software already exists in the form of [Trenchbroom](https://github.com/TrenchBroom/TrenchBroom).

## Features
- [x] Model Viewer
- [x] Texture Viewer
- [x] PAK Import
- [x] File Explorer
- [x] QuakeC Editor
- [x] Mod/Game Project Templates
- [x] Color Palette Editor 
- [ ] WAD Editor
- [ ] QuakeC Error/Warning Checking
- [ ] SPR and LMP Tools

## Changing Source Port

In order to allow for a fully offline functionality Quake Prism ships with Ironwail in the templates folder. If you want to use a different source port with your mod/game first launch Quake Prism and make a projects directory. Afterwards you can replace the source port files that appear in the projects directory. In order to be compatible with the one click compile and run from the editor the executable must be named quake.AppImage or quake.exe depending on your operating system.

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
- [Libre Quake](https://github.com/MissLavender-LQ/LibreQuake)
- [Kebby-Quake](https://github.com/Kebby-Quake)
