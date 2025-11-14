# Ascom Driver and Firmware for Custom Filter Wheel Driver

This repository contains an Ascom filter driver and arduino scripts to control an add-on electronic driver for the ZWO 1.25" manual astronomy filter wheel.

> [!NOTE]
> A second repository with 3D design files and overall description of the solution can be found here:
> [Foggy-Astro-Filter-Wheel-Automation](https://github.com/pharrisuk/Foggy-Astro-Filter-Wheel-Automation/blob/main/README.md)

## Firmware
The filter driver is controlled by a [Seeduino Xiao ESP32C3](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/) microcontroller. The three scripts supplied here are:

- [Tester Script](filterwheel_firmware/filter_wheel_script_v1.1/tester2.ino) : Script to test basic funtionality of the stepper motor, buttons and hall sensor.
- [Magnet Offset Calibration Tool](filterwheel_firmware/filter_wheel_script_v1.1/fw_magnet_offset_calibrator.ino) : Script to measure the number of steps between the magnet inserted into the filter wheel and the first filter (stepping clockwise).
- [Filter Wheel Driver](filterwheel_firmware/filter_wheel_script_v1.1/filter_wheel_script_v1.1.ino): The main driver (Set FILTER1_POS to the number of steps measured by the script above for initial calibration).


## Ascom Filter Wheel Driver
The main contents of the repository is the Visual Studio solution 'FoggyFilterDriver'. It was created with the Visual Studio Extension:
[ASCOM Platform 7 Project Templates (VS2022)](https://marketplace.visualstudio.com/items?itemName=PeterSimpson.ASCOMPlatform7Templates2022)

I used this Youtube videa as my starting point. It was very useful but quite old now. The Ascom driver design in V7 is different, but the initial steps to install the Ascom templates and generate a sample were the same from what I recall.
[ASCOM Driver tutorial for Arduino Astronomy Hardware using Visual Studio](https://www.youtube.com/watch?v=XVlrDyIBd5I)

### Installing the driver
You can install the driver using this setup program (uses Inno Setup): [FoggyAstroFW Setup.exe](installer/FoggyAstroFW%20Setup.exe)

Or load the repo into VisualStudio, and follow the Ascom template instructions to register the exe with Ascom : [ReadMe.htm](https://htmlpreview.github.io/?https://github.com/pharrisuk/FoggyFilterDriver/blob/main/FoggyFilterDriver/ReadMe.htm)



