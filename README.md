# Room133

Room133 is an ESP32-based device which helps with various room-monitoring tasks in a house, for use with [Home Assistant](https://www.home-assistant.io/).

It is based on the [og3](https://github.com/chl33/og3) C++ software framework for ESP devices.

![Room133 board render](images/room133_board_render.png)

## PCBA design: KiCAD

This project contains KiCAD sources for the Room133 circuit board.  It requires the [chl33_kicad_sym](https://github.com/chl33/chl33_kicad_sym) library.  I'm not experienced with sharing KiCAD projects in github.  I expect you will need to fix some path names if you try to open this project in KiCAD.

## EBox design: OpenSCAD

This project contains OpenSCAD sources for 3D-printing EBoxes for Room133 devices.  They use the library in [ProjectBox](https://github.com/chl33/ProjectBox).

![EBox for use with PIR sensor and photoresistor, with OLED display](images/scad_ebox_oled_pirl_vitamins.png)

You can open [`Ebox/Room133_test.scad`](Ebox/Room133_test.scad) with OpenSCAD and run [`Ebox/gui.sh`](Ebox/gui.sh), and use the user interface to customize the box depending on the application


![GUI for customizing the Ebox](images/scad_ebox_oled_pirl_vitamins.png).

I slice my EBox models with PrusaSlicer and print them in PETG.

