
//Display the options
//If user doesn't respond within a few seconds, return to main loop
void menuMain()
{
  while (1)
  {
    Serial.println();
    Serial.println("Menu: Main Menu");

    Serial.println("1) Configure Terminal Output");

    Serial.println("2) Configure Time Stamp");

    //Serial.println("3) Detect / Configure Attached Devices");
    Serial.println("3) Configure Qwiic Settings");

    Serial.println("4) Configure Geophone Gain and Threshold");

#if(HARDWARE_VERSION_MAJOR == 0)
    Serial.println("5) Configure Power Options");
#endif

    Serial.println("r) Reset all settings to default");

    Serial.println("q) Quit: Close log files and power down");

    //Serial.println("d) Debug Menu");

    Serial.println("x) Return to logging");

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
      menuLogRate();
    else if (incoming == '2')
      menuTimeStamp();
    //else if (incoming == '3')
    //  menuAttachedDevices();
    else if (incoming == '3')
      menuConfigure_QwiicBus();
    else if (incoming == '4')
      menuThreshold();
#if(HARDWARE_VERSION_MAJOR == 0)
    else if (incoming == '5')
      menuPower();
#endif
    else if (incoming == 'd')
      menuDebug();
    else if (incoming == 'r')
    {
      Serial.println("\n\rResetting to factory defaults. Press 'y' to confirm:");
      byte bContinue = getByteChoice(menuTimeout);
      if (bContinue == 'y')
      {
        EEPROM.erase();
        if (sd.exists("OLA_Geophone_settings.txt"))
          sd.remove("OLA_Geophone_settings.txt");
        if (sd.exists("OLA_Geophone_deviceSettings.txt"))
          sd.remove("OLA_Geophone_deviceSettings.txt");

        Serial.print("Settings erased. Please reset OpenLog Artemis and open a terminal at ");
        Serial.print((String)settings.serialTerminalBaudRate);
        Serial.println("bps...");
        while (1);
      }
      else
        Serial.println("Reset aborted");
    }
    else if (incoming == 'q')
    {
      Serial.println("\n\rQuit? Press 'y' to confirm:");
      byte bContinue = getByteChoice(menuTimeout);
      if (bContinue == 'y')
      {
        //Save files before going to sleep
        if (online.dataLogging == true)
        {
          sensorDataFile.sync();
          sensorDataFile.close(); //No need to close files. https://forum.arduino.cc/index.php?topic=149504.msg1125098#msg1125098
        }
        Serial.print("Log files are closed. Please reset OpenLog Artemis and open a terminal at ");
        Serial.print((String)settings.serialTerminalBaudRate);
        Serial.println("bps...");
        delay(sdPowerDownDelay); // Give the SD card time to shut down
        powerDown();
      }
      else
        Serial.println("Quit aborted");
    }
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  recordSystemSettings(); //Once all menus have exited, record the new settings to EEPROM and config file

  recordDeviceSettingsToFile(); //Record the current devices settings to device config file

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars

  //Reset measurements
  measurementCount = 0;
  measurementStartTime = millis();
}
