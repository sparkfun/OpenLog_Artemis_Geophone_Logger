/*
  OpenLog Artemis Geophone Logger
  By: Paul Clark (PaulZC)
  Date: July 17th, 2020
  Version: V1.0

  This firmware runs on the OpenLog Artemis and is dedicated to logging data from the SM-24 geophone:
  https://www.sparkfun.com/products/11744
  The geophone signal is sampled by the ADS122C04 24-bit ADC found on the Qwiic PT100:
  https://www.sparkfun.com/products/16770

  This code is inspired by Ole Wolf's geophone: https://github.com/olewolf/geophone
  Thank you Ole!

  Based on Ambiq Micro's stimer.c example:
  https://github.com/sparkfun/AmbiqSuiteSDK/tree/master/boards/apollo3_evb/examples/stimer
  The FFT analysis code is taken from Ambiq's pdm_fft example:
  https://github.com/sparkfun/Arduino_Apollo3/blob/master/libraries/PDM/examples/Example1_MicrophoneOutput/Example1_MicrophoneOutput.ino

  The sample rate is set to 500Hz. 1024 samples are taken and then analysed using an FFT.
  The resulting frequency and amplitude 'spectrum' is written to SD card and 'plotted' via the Serial port as required.
  The 500 amplitude values, which are logged/displayed, correspond to 0.5Hz to 250.5Hz in 0.5Hz bins.
  You can use the Arduino IDE Serial Plotter to display the real-time spectrum.

  The RTC can be set by a u-blox GNSS module.

  Based on v14 of:
  OpenLog Artemis
  By: Nathan Seidle
  SparkFun Electronics
  Date: November 26th, 2019
  License: This code is public domain but you buy me a beer if you use this
  and we meet someday (Beerware license).
  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/15793

  This firmware runs the OpenLog Artemis. A large variety of system settings can be
  adjusted by connecting at 115200bps.

*/

const int FIRMWARE_VERSION_MAJOR = 1;
const int FIRMWARE_VERSION_MINOR = 0;

#include "settings.h"

//Define the pin functions
//Depends on hardware version. This can be found as a marking on the PCB.
//x04 was the SparkX 'black' version.
//v10 was the first red version.
//05 and 06 are test versions based on x04 hardware. For x06, qwiic power is provide by a second AP2112K.
#define HARDWARE_VERSION_MAJOR 0
#define HARDWARE_VERSION_MINOR 4

#if(HARDWARE_VERSION_MAJOR == 0 && HARDWARE_VERSION_MINOR == 4)
const byte PIN_MICROSD_CHIP_SELECT = 10;
const byte PIN_IMU_POWER = 22;
#elif(HARDWARE_VERSION_MAJOR == 0 && HARDWARE_VERSION_MINOR == 5)
const byte PIN_MICROSD_CHIP_SELECT = 10;
const byte PIN_IMU_POWER = 22;
#elif(HARDWARE_VERSION_MAJOR == 0 && HARDWARE_VERSION_MINOR == 6)
const byte PIN_MICROSD_CHIP_SELECT = 10;
const byte PIN_IMU_POWER = 22;
#elif(HARDWARE_VERSION_MAJOR == 1 && HARDWARE_VERSION_MINOR == 0)
const byte PIN_MICROSD_CHIP_SELECT = 23;
const byte PIN_IMU_POWER = 27;
const byte PIN_PWR_LED = 29;
const byte PIN_VREG_ENABLE = 25;
const byte PIN_VIN_MONITOR = 34; // VIN/3 (1M/2M - will require a correction factor)
#endif

const byte PIN_POWER_LOSS = 3;
//const byte PIN_LOGIC_DEBUG = 11;
const byte PIN_MICROSD_POWER = 15;
const byte PIN_QWIIC_POWER = 18;
const byte PIN_STAT_LED = 19;
const byte PIN_IMU_INT = 37;
const byte PIN_IMU_CHIP_SELECT = 44;

enum returnStatus {
  STATUS_GETBYTE_TIMEOUT = 255,
  STATUS_GETNUMBER_TIMEOUT = -123455555,
  STATUS_PRESSED_X,
};

//Setup Qwiic Port
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <Wire.h>
TwoWire qwiic(1); //Will use pads 8/9
#define QWIIC_PULLUPS 1 // Default to 1k pull-ups on the Qwiic bus
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//EEPROM for storing settings
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <EEPROM.h>
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//microSD Interface
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <SPI.h>
#include <SdFat.h> //SdFat (FAT32) by Bill Greiman: http://librarymanager/All#SdFat
SdFat sd;
SdFile sensorDataFile; //File that all sensor data is written to

char sensorDataFileName[30] = ""; //We keep a record of this file name so that we can re-open it upon wakeup from sleep
const int sdPowerDownDelay = 100; //Delay for this many ms before turning off the SD card power
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Add RTC interface for Artemis
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include "RTC.h" //Include RTC library included with the Aruino_Apollo3 core
APM3_RTC myRTC; //Create instance of RTC class
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Header files for all compatible Qwiic sensors
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "SparkFun_I2C_Mux_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_I2C_Mux
#include "SparkFun_Ublox_Arduino_Library.h" //http://librarymanager/All#SparkFun_Ublox_GPS
#include "SparkFun_ADS122C04_ADC_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_ADS122C04

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
uint64_t measurementStartTime; //Used to calc the actual update rate. Max is ~80,000,000ms in a 24 hour period.
uint64_t lastSDFileNameChangeTime; //Used to calculate the interval since the last SD filename change
unsigned long measurementCount = 0; //Used to calc the actual update rate.
char geophoneData[16384]; //Array to hold the geophone data. Factor of 512 for easier recording to SD in 512 chunks
char geophoneDataSerial[16384]; //Array to hold the geophone data for serial plotter mode
char dateTime[50]; //Array to hold the log date and/or time
char peakFreq[50]; //Array to hold the peak frequency and amplitude and count
unsigned long lastReadTime = 0; //Used to delay until user wants to record a new reading
const byte menuTimeout = 15; //Menus will exit/timeout after this number of seconds
volatile static bool samplingEnabled = true; // Flag to indicate if sampling is enabled (sampling is paused while the menu is open)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Geophone settings
//#define TEST_PERIOD_1 20000 // Uncomment to enable sinusoidal test signal (period in micros) (20000 = 50Hz)
//#define TEST_AMPLITUDE_1 100 // Amplitude of the sinusoidal test signal
//#define TEST_PERIOD_2 30303 // Uncomment to enable second (mixed) cosinusoidal test signal (period in micros) (30303 = 33Hz)
//#define TEST_AMPLITUDE_2 50 // Amplitude of the sinusoidal test signal
#define SAMPLE_INTERVAL 6000 // Interval for 500Hz with a 3MHz clock

#define DUMP(varname) {if (settings.serialPlotterMode == false) Serial.printf("%s: %llu\n", #varname, varname)}

void setup() {
  //If 3.3V rail drops below 3V, system will power down and maintain RTC
  pinMode(PIN_POWER_LOSS, INPUT); // BD49K30G-TL has CMOS output and does not need a pull-up
  
  delay(1); // Let PIN_POWER_LOSS stabilize

  if (digitalRead(PIN_POWER_LOSS) == LOW) powerDown(); //Check PIN_POWER_LOSS just in case we missed the falling edge
  attachInterrupt(digitalPinToInterrupt(PIN_POWER_LOSS), powerDown, FALLING); //Attach the interrupt
  
  powerLEDOn(); // Turn the power LED on - if the hardware supports it
  
  pinMode(PIN_STAT_LED, OUTPUT);
  digitalWrite(PIN_STAT_LED, HIGH); // Turn the STAT LED on while we configure everything

  Serial.begin(115200); //Default for initial debug messages if necessary
  if (settings.serialPlotterMode == false) Serial.println();

  SPI.begin(); //Needed if SD is disabled

  beginSD(); //285 - 293ms

  loadSettings(); //50 - 250ms

  Serial.flush(); //Complete any previous prints
  Serial.begin(settings.serialTerminalBaudRate);
  if (settings.serialPlotterMode == false) Serial.printf("Artemis OpenLog Geophone Logger v%d.%d\n", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);

  beginQwiic(); //Enable qwiic power and start I2C
  delay(250); // Allow extra time for the qwiic sensors to power up

  analogReadResolution(14); //Increase from default of 10

  beginDataLogging(); //180ms
  lastSDFileNameChangeTime = rtcMillis(); // Record the time of the file name change

  disableIMU(); //Make sure the IMU is powered down (it shares the SPI bus with the SD card)

  if ((online.microSD == true) && (settings.serialPlotterMode == false)) Serial.println("SD card online");
  else if (settings.serialPlotterMode == false) Serial.println("SD card offline");

  if ((online.dataLogging == true) && (settings.serialPlotterMode == false)) Serial.println("Data logging online");
  else if (settings.serialPlotterMode == false) Serial.println("Datalogging offline");

  if ((settings.enableTerminalOutput == false) && (settings.logData == true) && (settings.serialPlotterMode == false)) Serial.println("Logging to microSD card with no terminal output");

  if (detectQwiicDevices() == true) //159 - 865ms but varies based on number of devices attached
  {
    beginQwiicDevices(); //Begin() each device in the node list
    loadDeviceSettingsFromFile(); //Load config settings into node list
    configureQwiicDevices(); //Apply config settings to each device in the node list
    printOnlineDevice();
  }
  else
    if (settings.serialPlotterMode == false) Serial.println("No Qwiic devices detected");

  //Serial.printf("Setup time: %.02f ms\n", (micros() - startTime) / 1000.0);

  //Print the helper text
  if ((settings.enableTerminalOutput == true)  && (settings.serialPlotterMode == false))
  {
    if (settings.logDate) Serial.print("Date,");
    if (settings.logTime) Serial.print("Time,");
    Serial.print("Peak_Frequency(Hz),Peak_Amplitude");
    if (settings.printMeasurementCount) Serial.print(",Count");
    Serial.println();
  }

  digitalWrite(PIN_STAT_LED, LOW); // Turn the STAT LED off now that everything is configured

  geophone_setup(); // Set up the ADC and start the interrupts
}

void loop() {
  
  if (Serial.available()) // Check if the user pressed a key
  {
    samplingEnabled = false; // Disable sampling while menu is open
    
    //Close sensor data file while we update the settings
    if (online.dataLogging == true)
    {
      sensorDataFile.sync();
      sensorDataFile.close();
    }

    menuMain(); //Present user menu
    
    // Increment the log file number
    // Comment the next line to reopen the same log file
    strcpy(sensorDataFileName, findNextAvailableLog(settings.nextDataLogNumber, "dataLog"));
    
    beginDataLogging(); //(Re)open the log file
    lastSDFileNameChangeTime = rtcMillis(); // Record the time of the file name change
    
    //Print the helper text
    if ((settings.enableTerminalOutput == true)  && (settings.serialPlotterMode == false))
    {
      if (settings.logDate) Serial.print("Date,");
      if (settings.logTime) Serial.print("Time,");
      Serial.print("Peak_Frequency(Hz),Peak_Amplitude");
      if (settings.printMeasurementCount) Serial.print(",Count");
      Serial.println();
    }
    
    samplingEnabled = true; // Re-enable sampling
  }

  //Is it time to record new data?
  if (geophone_loop()) //Check if we have collected 1024 samples and that the amplitude theshold has been exceeded
  {
    measurementCount++; //Increment the measurement count

    getDateTime(); //Collect the date and time

    // First, write the data to SD card (if enabled). Give the code up to 1.3 seconds to do that
    // just in case the SD card glitches.
    // Next, write the serial plotter data to the terminal (if enabled). Drip-feed the serial
    // data to avoid conflicts with the stimer interrupts.
    // Finally, open a new log file if it is time to do that.

    //Record to SD
    if (settings.logData == true)
    {
      if (settings.enableSD && online.microSD)
      {
        digitalWrite(PIN_STAT_LED, HIGH);
        
        int dataLength = strlen(dateTime);
        if (dataLength > 0)
        {
          if (sensorDataFile.write(&dateTime, dataLength) != dataLength) //Write the date and time to the log file
          {
            printDebug("*** sensorDataFile.write (dateTime) data length mismatch! ***\n");
          }
        }
        
        dataLength = strlen(geophoneData);
        int startByte = 0;
        unsigned long writeStart = millis(); // Record the time we started writing
        // Keep writing until all data is written or write time exceeds 1.3 seconds.
        // Note: the 1.3 second write time limit is defined the sample rate and the baud rate for serial plotter mode.
        // Don't change the limit unless you know what you are doing.
        while ((dataLength > 0) && (millis() < (writeStart + 1300)))
        {
          int bytesToWrite;
          if (dataLength > 512)
            bytesToWrite = 512;
          else
            bytesToWrite = dataLength;
          if (sd.card()->isBusy()) //Check if the card is still busy from the previous write
          {
//            printDebug("sd.card isBusy. bytesToWrite=");
//            printDebug((String)dataLength);
//            printDebug("\n");
            for (int i = 0; i < 10; i++) //Wait for 10ms before trying again
            {
              delay(1);
            }
          }
          else
          {
            if (sensorDataFile.write(&(geophoneData[startByte]), bytesToWrite) != bytesToWrite) //Record the buffer to the card
            {
              printDebug("*** sensorDataFile.write data length mismatch! ***\n");
            }
            dataLength -= bytesToWrite;
            startByte += bytesToWrite;
          }
        }

        if (dataLength > 0) printDebug("*** sensorDataFile.write timed out! ***\n");

        sensorDataFile.sync(); //Sync every two seconds
        updateLogFileAccess(); //Update the "Date modified"
        
        //Check if it is time to open a new log file
        uint64_t secsSinceLastFileNameChange = rtcMillis() - lastSDFileNameChangeTime; // Calculate how long we have been logging for
        secsSinceLastFileNameChange /= 1000ULL; // Convert to secs
        if ((settings.openNewLogFilesAfter > 0) && (((unsigned long)secsSinceLastFileNameChange) >= settings.openNewLogFilesAfter))
        {
          //Close existings files
          if (online.dataLogging == true)
          {
            samplingEnabled = false; // Disable sampling while we change the log file
            updateLogFileAccess(); //Update the "Date modified"
            sensorDataFile.sync(); //Sync any remaining data
            sensorDataFile.close(); //Close the file
            strcpy(sensorDataFileName, findNextAvailableLog(settings.nextDataLogNumber, "dataLog")); //Find the next available file
            beginDataLogging(); //180ms
            samplingEnabled = true; // Re-enable sampling
          }

          lastSDFileNameChangeTime = rtcMillis(); // Record the time of the file name change
        }

        digitalWrite(PIN_STAT_LED, LOW);
      }
    }

    if ((settings.enableTerminalOutput == true)  && (settings.serialPlotterMode == false)) // If terminal output is enabled, print the date, time and peakFreq
    {
      Serial.print(dateTime);
      Serial.println(peakFreq);
    }
    // If serial plotter mode is enabled, drip-feed the serial data to avoid interrupt conflicts
    // (Trying to write the data all in one go causes problems.)
    else if ((settings.enableTerminalOutput == true)  && (settings.serialPlotterMode == true))
    {
      int dataLength = strlen(geophoneDataSerial);
      int delayEveryBytes = settings.serialTerminalBaudRate / 10000; // Calculate how many bytes can be sent in a ms (assumes 10 bits per byte)
      //delayEveryBytes -= 1; // Round down to create gaps (but the ADC samples also help to create gaps - ~180us every 1.95ms
      int bytesSent = 0;
      int thisByte = 0;
      while (dataLength > 0)
      {
        Serial.write(geophoneDataSerial[thisByte]);
        thisByte++;
        bytesSent++;
        dataLength--;
        if (bytesSent == delayEveryBytes)
        {
          delay(1);
          bytesSent = 0;
        }
      }
    }
  }
}

void beginQwiic()
{
  pinMode(PIN_QWIIC_POWER, OUTPUT);
  qwiicPowerOn();
  qwiic.begin();
  qwiic.setPullups(QWIIC_PULLUPS); //Just to make it really clear what pull-ups are being used, set pullups here.
}

void beginSD()
{
  pinMode(PIN_MICROSD_POWER, OUTPUT);
  pinMode(PIN_MICROSD_CHIP_SELECT, OUTPUT);
  digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected

  if (settings.enableSD == true)
  {
    microSDPowerOn();

    //Max power up time is 250ms: https://www.kingston.com/datasheets/SDCIT-specsheet-64gb_en.pdf
    //Max current is 200mA average across 1s, peak 300mA
    for (int i = 0; i < 10; i++) //Wait
    {
      delay(1);
    }

    if (sd.begin(PIN_MICROSD_CHIP_SELECT, SD_SCK_MHZ(24)) == false) //Standard SdFat
    {
      printDebug("SD init failed (first attempt). Trying again...\n");
      for (int i = 0; i < 250; i++) //Give SD more time to power up, then try again
      {
        delay(1);
      }
      if (sd.begin(PIN_MICROSD_CHIP_SELECT, SD_SCK_MHZ(24)) == false) //Standard SdFat
      {
        //if (settings.serialPlotterMode == false) Serial.println("SD init failed (second attempt). Is card present? Formatted?");
        digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected
        online.microSD = false;
        return;
      }
    }

    //Change to root directory. All new file creation will be in root.
    if (sd.chdir() == false)
    {
      //if (settings.serialPlotterMode == false) Serial.println("SD change directory failed");
      online.microSD = false;
      return;
    }

    online.microSD = true;
  }
  else
  {
    microSDPowerOff();
    online.microSD = false;
  }
}

void disableIMU()
{
  pinMode(PIN_IMU_POWER, OUTPUT);
  pinMode(PIN_IMU_CHIP_SELECT, OUTPUT);
  digitalWrite(PIN_IMU_CHIP_SELECT, HIGH); //Be sure IMU is deselected

  imuPowerOff();
}

void beginDataLogging()
{
  if (online.microSD == true && settings.logData == true)
  {
    //If we don't have a file yet, create one. Otherwise, re-open the last used file
    if (strlen(sensorDataFileName) == 0)
      strcpy(sensorDataFileName, findNextAvailableLog(settings.nextDataLogNumber, "dataLog"));

    // O_CREAT - create the file if it does not exist
    // O_APPEND - seek to the end of the file prior to each write
    // O_WRITE - open for write
    if (sensorDataFile.open(sensorDataFileName, O_CREAT | O_APPEND | O_WRITE) == false)
    {
      if (settings.serialPlotterMode == false) Serial.println("Failed to create sensor data file");
      online.dataLogging = false;
      return;
    }
    updateLogFileCreate(); //Update the "Date created"

    online.dataLogging = true;
  }
  else
    online.dataLogging = false;

}

void updateLogFileCreate()
{
  myRTC.getTime(); //Get the RTC time so we can use it to update the create time
  //Update the file create time
  bool result = sensorDataFile.timestamp(T_CREATE, (myRTC.year + 2000), myRTC.month, myRTC.dayOfMonth, myRTC.hour, myRTC.minute, myRTC.seconds);
  //printDebug("updateDataFileCreate: gnssDataFile.timestamp T_CREATE returned ");
  //printDebug((String)result);
  //printDebug("\n");
}

void updateLogFileAccess()
{
  myRTC.getTime(); //Get the RTC time so we can use it to update the last modified time
  //Update the file access time
  bool result = sensorDataFile.timestamp(T_ACCESS, (myRTC.year + 2000), myRTC.month, myRTC.dayOfMonth, myRTC.hour, myRTC.minute, myRTC.seconds);
  //printDebug("updateDataFileAccess: gnssDataFile.timestamp T_ACCESS returned ");
  //printDebug((String)result);
  //printDebug("\n");
  //Update the file write time
  result = sensorDataFile.timestamp(T_WRITE, (myRTC.year + 2000), myRTC.month, myRTC.dayOfMonth, myRTC.hour, myRTC.minute, myRTC.seconds);
  //printDebug("updateDataFileAccess: gnssDataFile.timestamp T_WRITE returned ");
  //printDebug((String)result);
  //printDebug("\n");
}

void updateLogFileWrite()
{
  myRTC.getTime(); //Get the RTC time so we can use it to update the last modified time
  //Update the file write time
  bool result = sensorDataFile.timestamp(T_WRITE, (myRTC.year + 2000), myRTC.month, myRTC.dayOfMonth, myRTC.hour, myRTC.minute, myRTC.seconds);
  //printDebug("updateDataFileAccess: gnssDataFile.timestamp T_WRITE returned ");
  //printDebug((String)result);
  //printDebug("\n");
}

// stimer compare ISR - used to set the ADS122C04 sample rate
extern "C" void am_stimer_cmpr5_isr(void)
{
  //
  // Check the timer interrupt status.
  //
  am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREF);
  am_hal_stimer_compare_delta_set(5, SAMPLE_INTERVAL);

  if (samplingEnabled == true)
    sampling_interrupt();
}
