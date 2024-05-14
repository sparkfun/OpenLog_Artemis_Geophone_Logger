# OpenLog Artemis : Upgrade Instructions

The [**/Binaries**](./Binaries) folder contains the binary files for the different versions of the OLA firmware.

You can upload these to the OLA using the [Artemis Firmware Upload GUI](https://github.com/sparkfun/Artemis-Firmware-Upload-GUI) instead of the Arduino IDE.
Which is handy if you want to quickly update the firmware in the field, or are not familiar with the IDE.

The firmware is customized for the different versions of the OLA hardware. You will find versions for the **X04 SparkX (Black) OLA** and **V10 SparkFun (Red) OLA** plus any subsequent revisions. The filename tells you which hardware the firmware is for and what version it is:

* OpenLog_Artemis_Geophone_Logger-V10-v20.bin - is the _stable_ version for the **V10 SparkFun (Red) OLA**
* OpenLog_Artemis_Geophone_Logger-X04-v20.bin - is the _stable_ version for the **X04 SparkX (Black) OLA**

From time to time, you may see firmware binaries with **BETA** in the filename. These are _beta_ versions containing new features and improvements which are still being tested and documented

**OpenLog_Artemis_Geophone_Logger-V10-v20-NoPowerLossProtection.bin** is a special build of v2.0 which has no power loss protection:

With the normal firmware, the OLA goes into deep sleep if the power fails or is disconnected. This allows the Real Time Clock to keep running, but it does mean that a reset is required to wake the OLA again.
With the NoPowerLossProtection firmware, the code does not go into deep sleep when the power is lost, meaning that it will restart automatically as soon as power is reconnected.
This can be useful for embedded OLA systems which are hard to access. The downside is that the Real Time Clock setting is lost when the OLA restarts.

## To use:

* Follow the instructions in the [Artemis-Firmware-Upload-GUI repo](https://github.com/sparkfun/Artemis-Firmware-Upload-GUI) to install the Upload GUI on your platform
* Download and extract the [Geophone Logger repo ZIP](https://github.com/sparkfun/OpenLog_Artemis_Geophone_Logger/archive/main.zip)
* Select the OLA firmware file you'd like to upload from the OLA **/Binaries** folder (should end in *.bin*)
* Attach the OLA target board using USB
* Select the COM port (hit Refresh to refresh the list of USB devices)
* Press **Upload Firmware**

The GUI does take a few seconds to load and run. _**Don't Panic**_ if the GUI does not start right away.

- Your friends at SparkFun.
