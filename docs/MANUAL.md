# Quake Prism Manual

## Introduction

Quake Prism is an all in one development solution for vanilla Quake engine games and mods based on modern game engine tools. The primary focus of this project is to enable not only modders but also those interested in making fully original games with the Quake engine. Most of the tooling is basic asset manipulation along with a QuakeC editor. 

## About this Document

This manual will act as a guide for two purposes. The first of which is to cover how to utilize the tools that come with Quake Prism. The second purpose is to document the QuakeC code in the project templates. This manual is not designed to teach how to make mods or games using the Quake engine and will only relate to the scope of the Quake Prism toolset.

## Getting Started

In this section we will cover the basics of beginning to use Quake Prism. This will include understanding the startup and initial project creation, along with an introduction to each of the project templates so that you can make the correct decision for your mod or game.

### Startup

```
insert screenshot here of startup
```

When you launch Quake Prism for the first time it will require that you select a location for it to create a Quake Prism projects directory. Quake Prism will automatically create the directory upon selection of a location for projects. If the projects directory is ever moved or deleted Quake Prism will ask you to make a new one.

### Main Window

After setting a projects directory or opening Quake Prism in future instances you will be met with the screen below. 
In order to begin using the toolset of the editor you will need to either select the new project or open project button.

```
insert screenshot here of initial startup view
```

### Open Project

```
insert screenshot here of open project popup
```

Upon pressing the open project button you will be presented with a popup that shows all viable projects by scanning for directories in your Quake Prism projects folder. Select which project you would like to open by clicking on its name and then pressing the open button. 

### New Project

```
insert screenshot here of new project popup
```

Upon pressing the new project button you will be presented with a popup that allows for you to select one of four project templates. The currently selected template can be seen above the text prompt for the new project name. In order to make a new project select the desired template by clicking on it, name your project using the prompt, and then select the make project button.

If you choose the Import PAK option then you will additionally need to import any number of Quake PAK files and then select a codebase between either the Quake Prism blank template or the id1 template from the original Quake game.

## Using the Editor

### File Explorer

```
insert screenshot here of file explorer
```

After opening or creating a new project the first thing the editor will do is load your project into the file explorer panel. This functions as a file tree structure where folders can be expanded by being clicked and files can be opened by their respective tooling with a double click. Files can also be moved by using drag and drop functionality. **NOTE: Quake Prism expects certain files to be in preconfigured locations, thus relocating certain files my lead to problems with compilation and execution of your mod/game**

### Menu Bar

In order to access some of the functionality of the Quake Prism editor the menu bar is used.

```
insert screenshot here of menu bar
```

The File menu has options for [New Project](###new-project) and [Open Project](###open-project). These function the same as the buttons presented upon startup. There is also a **Containing Folder** option which opens the current project directory in your system default file explorer to give quick access to the directory for use with other programs. Lastly, there is an **Exit** option which is an alternate way to close the application.

The Run menu has two options within it. The first option is **Compile**. This option launches fteqcc as a background process and compiles the code in the src directory of your mod. If your mod does not contain a src folder with the fteqcc command line interface executable you will be unable to utilize this feature. The second option is **Run**. This option launches the Quake engine executable in the root of your Quake Prism projects folder with the selected game being the project currently loaded in the editor. By default the Qauke engine source port used is vkQuake but can be changed by bringing in a new executable named quake.AppImage for Linux and quake.exe for Windows.

The Help menu has two options as well. The **About** option presents a popup with a short blurb about the Quake Prism project. The **Manual** option will open a link to this manual in your default web browser.

### QuakeC Editor

```
insert screenshot here of Quake C editor
```

### Model Viewer

```
insert screenshot here of model viewer
```

The model viewer is split into two panes. The top pane is the Model View pane which displays the model and accepts input for manipulation the model's position, rotation, and scale. Use the left mouse button to rotate the model, the right mouse button to move the model, and the scroll wheel to scale the model.

The Model Tools pane contains a suite of options to further manipulate the model view. In the left column (top-to-bottom) there is an animation slider that displays the progress of the animation. There is also a pause/play toggle along with buttons to manually move forward and backward through the animation when it is paused. The render scale option allows you to set the render scale of the model between 1/8, 1/4, 1/2, and 1. Texture mode allows you to choose between Textured, Untextured, or Wireframe to get different views of your model. In the right column (top-to-bottom) there is a skin selection slider to change the active skin for the current model. The next two options are toggles for animation interpolation and texture filtering view settings. The Export Texture button exports the current skin as a tga file to the progs directory. The Import Texture button will open a file prompt and then upon selecting a tga file it will be imported to the currently selected skin.

### Texture Viewer

The texture viewer is a simple pane that just renders any of the tga files that you may select in the file tree.

## Templates

As mentioned in the [Getting Started](#getting-started) section there are four project templates presently included with Quake Prism. 
