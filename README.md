SparkFun OpenLog Artemis : Geophone Logger
===========================================================

[![SparkFun OpenLog Artemis](https://cdn.sparkfun.com//assets/parts/1/5/7/5/3/16832-SparkFun_OpenLog_Artemis-01.jpg)](https://www.sparkfun.com/products/16832)

[*SparkFun OpenLog Artemis (SPX-16832)*](https://www.sparkfun.com/products/16832)

The OpenLog Artemis is an open source datalogger that comes preprogrammed to automatically log IMU, GPS, serial data, and various pressure, humidity, and distance sensors. All without writing a single line of code! OLA automatically detects, configures, and logs Qwiic sensors. OLA is designed for users who just need to capture a bunch of data to SD and get back to their larger project.

The firmware in this repo is dedicated to logging seismic data from the [SM-24 Geophone](https://www.sparkfun.com/products/11744)
using the ADS122C04 ADC on the [Qwiic PT100](https://www.sparkfun.com/products/16770). [You can find the main OpenLog Artemis repo here](https://github.com/sparkfun/OpenLog_Artemis).

Fast Fourier Transformed frequency and amplitude data is logged to SD card in CSV format. The CSV files can be analyzed with your favorite spreadsheet software or analysis tool.
You can also use the Arduino IDE Serial Plotter to visualize the live FFT data.

![SerialPlotter.gif](img/SerialPlotter.gif)

OpenLog Artemis is highly configurable over an easy to use serial interface. Simply plug in a USB C cable and open a terminal at 115200kbps. The logging output is automatically streamed to both the terminal and the microSD. Pressing any key will open the configuration menu.

The OpenLog Artemis automatically scans for, detects, configures, and logs data from the ADS122C04.

The menus will let you configure the:

* ADS122C04 Gain (x1 to x128)
* Amplitude Threshold - so you only log seismic events that exceed the set threshold
* Artemis Real Time Clock - each seismic event is date and timestamped. The RTC can be synchronized accurately to GNSS time too.

New features are constantly being added so we’ve released an easy to use firmware upgrade tool. No need to install Arduino or a bunch of libraries, simply open the [Artemis Firmware Upload GUI](https://github.com/sparkfun/Artemis-Firmware-Upload-GUI), load the latest OLA firmware, and add features to OpenLog Artemis as they come out! Full instructions are available in [UPGRADE.md](UPGRADE.md).

Repository Contents
-------------------

* **/Binaries** - The binary files for the different versions of the OLA firmware
* **/Firmware** - The main sketch that runs on the OpenLog Artemis.

Documentation
--------------

* **[HOOKUP.md](HOOKUP.md)** - the comprehensive hookup guide
* **[UPGRADE.md](UPGRADE.md)** - contains full instructions on how to upgrade the firmware on the OLA using the [Artemis Firmware Upload GUI](https://github.com/sparkfun/Artemis-Firmware-Upload-GUI)
* **[Installing an Arduino Library Guide](https://learn.sparkfun.com/tutorials/installing-an-arduino-library)** - OLA includes a large number of libraries that will need to be installed before compiling will work.

License Information
-------------------

This product is _**open source**_!

Various bits of the code have different licenses applied. Anything SparkFun wrote is beerware; if you see me (or any other SparkFun employee) at the local, and you've found our code helpful, please buy us a round!

Please use, reuse, and modify these files as you see fit. Please maintain attribution to SparkFun Electronics and release anything derivative under the same license.

Distributed as-is; no warranty is given.

- Your friends at SparkFun.
