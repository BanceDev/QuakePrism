# Quake Prism Manual

## Introduction

Quake Prism is an all in one development solution for vanilla Quake engine games and mods based on modern game engine tools. The primary focus of this project is to enable not only modders but also those interested in making fully original games with the Quake engine. Most of this apps tooling is Quake related asset manipulation along with a QuakeC editor and launcher for mods. 

## About this Document

This manual will act as a guide for two purposes. The first of which is to cover how to utilize the tools that come with Quake Prism. The second purpose is to document the QuakeC code in the project templates. This manual is not designed to teach how to make mods or games using the Quake engine and will only relate to the scope of the Quake Prism toolset.

## Getting Started

In this section we will cover the basics of beginning to use Quake Prism. This will include understanding the startup and initial project creation, along with covering the layout and basic utilities of the editor.

### Startup

<p align="center">
    <img src="https://github.com/BanceDev/QuakePrism/blob/main/docs/start.png" alt="Start Screen"/>
</p>


When you launch Quake Prism for the first time it will require that you select a location for it to create a Quake Prism projects directory. Quake Prism will automatically create the directory upon selection of a location for projects. If the projects directory is ever moved or deleted Quake Prism will ask you to make a new one.

### Main Window

After setting a projects directory or opening Quake Prism in future instances you will be met with the screen below. 
In order to begin using the toolset of the editor you will need to either select the new project or open project button.

<p align="center">
    <img src="https://github.com/BanceDev/QuakePrism/blob/main/docs/main.png" alt="Start Screen"/>
</p>

### Open Project

<p align="center">
    <img src="https://github.com/BanceDev/QuakePrism/blob/main/docs/open.png" alt="Start Screen"/>
</p>

Upon pressing the open project button you will be presented with a popup that shows all viable projects by scanning for directories in your Quake Prism projects folder. Select which project you would like to open by clicking on its name and then pressing the open button. 

### New Project

<p align="center">
    <img src="https://github.com/BanceDev/QuakePrism/blob/main/docs/new.png" alt="Start Screen"/>
</p>

Upon pressing the new project button you will be presented with a popup that allows for you to select one of three project templates. The currently selected template can be seen above the text prompt for the new project name. In order to make a new project select the desired template by clicking on it, name your project using the prompt, and then select the make project button.

If you choose the Import PAK option then you will additionally need to import any number of Quake PAK files and then select a codebase between either the Quake Prism blank template or the id1 template from the original Quake game.

## Using the Editor

### File Explorer

<p align="center">
    <img src="https://github.com/BanceDev/QuakePrism/blob/main/docs/explorer.png" alt="Start Screen"/>
</p>

After opening or creating a new project the first thing the editor will do is load your project into the file explorer panel. This functions as a file tree structure where folders can be expanded by being clicked and files can be opened by their respective tooling with a double click. Right clicking on a file will give you the options to rename or delete the file. **NOTE: Quake Prism expects certain files to be in preconfigured locations, thus relocating certain files my lead to problems with compilation and execution of your mod/game**

### Menu Bar

In order to access some of the functionality of the Quake Prism editor the menu bar is used.

The File menu has options for creating a new project or creating a new file/folder in the src directory. **NOTE: Due to the Quake engine having a pre-defined folder structure, Quake Prism is only focused on aiding with the creation of new source files.** If you want to create a new QuakeC file within a subdirectory you can do it using the following format "<subdirectory>/<filename>" which can be seen below.

<p align="center">
    <img src="https://github.com/BanceDev/QuakePrism/blob/main/docs/rename.png" alt="Start Screen"/>
</p>

Additionally there is an **Open** button for opening a different project. This functions the same as the startup open project button. There is also a **Containing Folder** option which opens the current project directory in your system default file explorer to give quick access to the directory for use with other programs. Lastly, there is an **Exit** option which is an alternate way to close the application.

The Run menu has three options within it. The first option is **Compile**. This option launches fteqcc as a background process and compiles the code in the src directory of your mod. If your mod does not contain a src folder with the fteqcc command line interface executable you will be unable to utilize this feature. The second option is **Run**. This option launches the Quake engine executable in the root of your Quake Prism projects folder with the selected game being the project currently loaded in the editor. By default the Qauke engine source port used is vkQuake but can be changed by bringing in a new executable named quake.AppImage for Linux and quake.exe for Windows. The final option is **Compile and Run** which does the compile and run functions one after the other for a quick way to update your progs and launch your mod.

The Help menu has two options for quick info about the editor. The **About** option presents a popup with a short blurb about the Quake Prism project. The **Documentation** option will open a link to this manual in your default web browser.

### QuakeC Editor

<p align="center">
    <img src="https://github.com/BanceDev/QuakePrism/blob/main/docs/editor.png" alt="Start Screen"/>
</p>

The QuakeC editor is a simple text editor that will open .qc/.src/.cfg/.rc files from your project for editing. It's primarily focused on support for the QuakeC language however. There is a menu bar with most standard file editing utilities along with the ability to change the theme of the editor between dark, light, and retro modes. The editor also comes fully featured with syntax highlighting for QuakeC as well as embedded warning and error checking using fteqcc in the background. This process will automatically compile and analyze your code after the program detects the user is no longer presently typing in the file. Due to the limitations of fteqcc and the way that QuakeC compiles this process will need to save your files before each compile. You will still have acess to undo of course if you want to reverse changes.

### Model Viewer

<p align="center">
    <img src="https://github.com/BanceDev/QuakePrism/blob/main/docs/mdlview.png" alt="Start Screen"/>
</p>

The model viewer is split into two panes. The top pane is the Model View pane which displays the model and accepts input for manipulation the model's position, rotation, and scale. Use the left mouse button to rotate the model, the right mouse button to move the model, and the scroll wheel to scale the model.

The Model Tools pane contains a suite of options to further manipulate the model view. In the left column (top-to-bottom) there is an animation slider that displays the progress of the animation. There is also a pause/play toggle along with buttons to manually move forward and backward through the animation when it is paused. The render scale option allows you to set the render scale of the model between 1/8, 1/4, 1/2, and 1. Texture mode allows you to choose between Textured, Untextured, or Wireframe to get different views of your model. In the right column (top-to-bottom) there is a skin selection slider to change the active skin for the current model. The next two options are toggles for animation interpolation and texture filtering view settings. The Export Texture button exports the current skin as a tga file to the progs directory. The Import Texture button will open a file prompt and then upon selecting a tga file it will be imported to the currently selected skin.

### Texture Viewer

The texture viewer is a simple pane that just renders any of the tga files that you may select in the file tree.

### Palette Editor

<p align="center">
    <img src="https://github.com/BanceDev/QuakePrism/blob/main/docs/palette.png" alt="Start Screen"/>
</p>

The palette editor is composed of an array of colors that loads in the palette.lmp file from your projects gfx directory on startup. To edit the palette click on one of the colors and a popup will apear that allows you to change the color. To save your palette to your project's palette.lmp file press the export button at the top of the pane.

## Templates

As mentioned in the [Getting Started](#getting-started) section there are three project templates presently included with Quake Prism. Each template is designed to fulfill a slightly different purpose and will ideally be a great starting point for any Quake mod/game.

### Blank Game

This project template is designed for those seeking to have a clean slate for making a wholly original game using the vanilla Quake engine. Thus this project includes the bare minimum, all the essential gfx for the menu are from (LibreQuake)[https://github.com/lavenderdotpet/LibreQuake] and the gfx.wad is from the prototype pack made by (Kebby-Quake)[https://github.com/Kebby-Quake]. There is a single map, start.bsp, which is just a box with prototype textures on it. The only other asset is the player.mdl also borrowed from LibreQuake. 

The QuakeC is designed to be very minimal, the only functionality in the code is the ability to spawn in the player and allow them to move around. Only the essential functions required by the engine are supplied. This means an entirely clean starting point but also no out of the box functionality for the likes of enemies, weapons, items, etc.

### Import PAK

This project template is the ideal template for those seeking to mod the original Quake game or expand upon other already existing mods. As mentioned in the Getting Started section you need to select any number of pak files and a QuakeC codebase to create this project. Thus this templates acts more as a do-it-yourself sort of option that should get you set up for making a mod with nothing more than a copy of Quake in no time.

### LibreQuake

This project template contains the source code and assets of the (LibreQuake)[https://github.com/lavenderdotpet/LibreQuake] project. This project is a fully free and open source asset conversion of Quake. This template is here if you desire to either mod LibreQuake or do not own Quake but want to get into making mods on the engine without having to start with something like the **Blank Game** template.
