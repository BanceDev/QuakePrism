<p align="center">
    <img src="https://github.com/BanceDev/QuakePrism/blob/main/banner.png" alt="Logo for Quake Prism"/>
</p>

# Quake Prism

<p align="center">
    <img src="https://github.com/BanceDev/QuakePrism/blob/main/docs/editor.png" alt="Editor Screenshot"/>
</p>

Quake Prism is an all in one development solution for Quake engine games and mods based on modern game engine tools.
The primary focus of this project is to enable not only modders but also those interested in making fully original games with the Quake engine. Most of the tooling is basic asset manipulation along with a QuakeC editor.
Quake Prism also ships with project templates to help you start development right away along with including fteqcc64 for compilation and Quakespasm for the source port to run your mods/games from the editor.

Quake Prism will not come packaged with tooling for 3D modeling see this [Quake blender plugin](https://github.com/victorfeitosa/quake-hexen2-mdl-export-import). Quake Prism also will not include mapping tools due largely to the complexity of the requirements for map tooling along with the fact that highly featured modern mapping software already exists in the form of [Trenchbroom](https://github.com/TrenchBroom/TrenchBroom).

## Community
To stay in contact with developers and users check out the community [discord](https://discord.gg/pBQ7R4GGuX) for Quake Prism. 

## Features
- [x] Model Viewer
- [x] Texture Viewer
- [x] PAK Import
- [x] File Explorer
- [x] QuakeC Editor
- [x] Mod/Game Project Templates
- [x] Color Palette Editor 
- [x] QuakeC Error/Warning Checking
- [x] LMP Tools
- [ ] WAD Editor
- [ ] SPR Tools

## Releases
- Binary builds are available from [releases](https://github.com/BanceDev/QuakePrism/releases)

## Compiling
Clone the repo recursively to install imgui dependencies
```
git clone --recursive
```

### Linux Dependencies

#### Arch Based
```
sudo pacman -S sdl2 sdl2_image glew glm
```

#### Debian Based
```
sudo apt install libsdl2-dev libsdl2-image-dev libglew-dev libglm-dev
```

### Windows Dependencies

All windows builds for this project are currently cross compiled from Linux. There is a visual studio sln file that I have made based off of the ImGui template. It has not been tested at all so if you do manage to build from windows and want to support the project submit a PR to update this portion of the readme with instructions/changes that may need to be made to the visual studio sln/project files. 

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
