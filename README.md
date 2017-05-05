[![AppVeyor build status](https://ci.appveyor.com/api/projects/status/wxhjxlhk1pcbum8g/branch/master?svg=true)](https://ci.appveyor.com/project/MHeasell/ta3d/branch/master)
[![Travis CI build status](https://travis-ci.org/MHeasell/TA3D.svg?branch=master)](https://travis-ci.org/MHeasell/TA3D)

This is a work-in-progress fork of TA3D.
The objective is to create an open-source implementation
of the Total Annihilation engine that matches the look-and-feel
of the original and is highly compatible with existing mods, maps and units.
This fork officially supports Windows and Ubuntu.

## Installation

TA3D is just an engine and does nothing useful on its own.
You need to have Total Annihilation data files in order to play
(totala1.hpi, totala2.hpi, etc.).

### Windows

1. Unzip the TA3D zip file to a location of your choice.
1. Navigate to `%AppData%` in your file explorer
   (Press Windows+R, type `%AppData%` and press Enter).
1. Create a folder here called `TA3D`.
1. Inside that folder, create a folder called `resources`.
1. Copy your Total Annihilation data files in to the `resources` folder.

### Ubuntu

Ubuntu packages are not yet available.
You may compile and install from source
if you are feeling adventurous,
but explaining how to do this is beyond the scope of this guide.

Once you have installed TA3D,
place your Total Annihilation data files in `~/.ta3d/resources`.
If this directory does not exist, create it first.

## Playing

### Windows

To launch the game, navigate to the location you unzipped TA3D to
and double-click the `ta3d.exe` program.

### Ubuntu

After installing, run `ta3d` in your terminal.
