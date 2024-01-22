# Blue-64
Blue-64 is a plug & play bluetooth adapter for the Commodore 64 that plugs onto the keyboard header inside the computer and can control the keyboard lines and emulate keystrokes and joystick inputs. The goal of the project is to support two bluetooth controllers and a bluetooth keyboard simultaneously, in order to be able to controll the C64 completely wirelessly.

![](https://github.com/sideprojectslab/blue-64/blob/main/doc/pictures/IMG_0158.png)

## Button Mapping (xInput)

| Joystick     | Controller  | Controller Alt. |
|:---:         |:---:        |:---:            |
| UP           | D-Pad UP    | Button A        |
| DOWN         | D-Pad DOWN  | Right Trigger   |
| LEFT         | D-Pad LEFT  | -               |
| RIGHT        | D-Pad RIGHT | -               |
| FIRE         | Button B    | -               |

### Additional functions

Blue-64 has an internal menu that is visualized by printing text on screen. The menu allows typing and executing frequently used macros like loading the tape, loading and/or running programs from disk drive etc.

| Function         | Button(s)   |
|:---:             |:---:        |
| Swap Player 1-2  | View + Y    |
| Keyboard "space" | Menu        |
| Cycle Menu       | View + A/B  |
| Select/Run Menu  | View + Menu |

## Compatibility
Blue-64 aims to be compatible with all versions of the C64 motherboard. If and when incompatibilities become known they will be detailed in this README file.

## Limitations
Blue-64 can only interact with the lines present on the keyboard header, thus it has no access to the "paddle" control lines. Therefore it cannot emulate the Commodore mouse, paddle controls, and does not support additional joystick fire buttons (other than the primary one) that are based on paddle control.
At the moment bluetooth keyboard support is still work in progress.

## Installation & Precautions
Instruction manuals on how to install and operate the board are available in PDF format alongside the fabrication data (work in progress). Users shall read these instructions carefully and fully understand the circuit's limitations before installing and/or using it. Incorrect installation of the board supply or failure to comply with the recommended operating conditions may result in damage to the board and/or to the computer, with risk of overheating, fire and/or explosion.

## Firmware Update
Firmware binaries can be found in the "Releases" section of the GitHub page. The three files in the "binaries.zip" folder are necessary to perform a firmware update:
- bootloader.bin
- partition-table.bin
- blue-64-app.bin

Download and install the CP210X Universal Windows Drivers for the on-board programmer at this website:
https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads

Download the Espressif Flash Download Tool at this website:
https://www.espressif.com/en/support/download/other-tools

Run the .exe application and select "ESP32" as target and "Develop" as work mode when prompted. In the following screen upload the three binary files in the order at the addresses shown in the screenshot below:

![](https://github.com/sideprojectslab/blue-64/blob/main/doc/pictures/flash_tool.png)

Connect the PC to the USB port on the Blue-64 and select the appropriate COM port in the Flash Download Tool. Press the "erase" button to erase the FLASH and finally press "start" and the new firmware will be downloaded to the board (should take less than a minute).

In case the method above does not work, please try the online tool at this website:
https://espressif.github.io/esptool-js/

Don't worry you can't brick it (as far as I know), if something fails you will always be able to re-try flashing the new firmware.

## License
License information is included on top of all software source files as well as in all schematics. Files that do not contain explicit licensing information are subject to the licensing terms stated in the LICENSE.txt provided in the main project folder:

Unless stated otherwise in individual files, all hardware design Schematics, Bill of Materials, Gerber files and manuals are licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International. To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/4.0/

Unless otherwise stated in individual files, all software source files are Licensed under the Apache License, Version 2.0. You may obtain a copy of this license at http://www.apache.org/licenses/LICENSE-2.0

## Disclaimer
All material is provided on an 'AS IS' BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND in accordance to the license deed applicable to
each individual file.
