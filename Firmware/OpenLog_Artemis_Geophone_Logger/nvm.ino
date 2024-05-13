void loadSettings()
{
  //First load any settings from NVM
  //After, we'll load settings from config file if available
  //We'll then re-record settings so that the settings from the file over-rides internal NVM settings

  //Check to see if EEPROM is blank
  uint32_t testRead = 0;
  if (EEPROM.get(0, testRead) == 0xFFFFFFFF)
  {
    recordSystemSettings(); //Record default settings to EEPROM and config file. At power on, settings are in default state
    //Serial.println("Default settings applied");
  }

  //Check that the current settings struct size matches what is stored in EEPROM
  //Misalignment happens when we add a new feature or setting
  int tempSize = 0;
  EEPROM.get(0, tempSize); //Load the sizeOfSettings
  if (tempSize != sizeof(settings))
  {
    Serial.println("Settings wrong size. Default settings applied");
    recordSystemSettings(); //Record default settings to EEPROM and config file. At power on, settings are in default state
  }

  //Check that the olaIdentifier is correct
  //(It is possible for two different versions of the code to have the same sizeOfSettings - which causes problems!)
  int tempIdentifier = 0;
  EEPROM.get(sizeof(int), tempIdentifier); //Load the identifier from the EEPROM location after sizeOfSettings (int)
  if (tempIdentifier != OLA_IDENTIFIER)
  {
    Serial.println("Settings are not valid for this variant of the OLA. Default settings applied");
    recordSystemSettings(); //Record default settings to EEPROM and config file. At power on, settings are in default state
  }

  //Read current settings
  EEPROM.get(0, settings);

  loadSystemSettingsFromFile(); //Load any settings from config file. This will over-write any pre-existing EEPROM settings.
  //Record these new settings to EEPROM and config file to be sure they are the same
  //(do this even if loadSystemSettingsFromFile returned false)
  recordSystemSettings();
}

//Record the current settings struct to EEPROM and then to config file
void recordSystemSettings()
{
  settings.sizeOfSettings = sizeof(settings);
  EEPROM.put(0, settings);
  recordSystemSettingsToFile();
}

//Export the current settings to a config file
void recordSystemSettingsToFile()
{
  if (online.microSD == true)
  {
    if (sd.exists("OLA_Geophone_settings.txt"))
      sd.remove("OLA_Geophone_settings.txt");

    #if SD_FAT_TYPE == 1
    File32 settingsFile;
    #elif SD_FAT_TYPE == 2
    ExFile settingsFile;
    #elif SD_FAT_TYPE == 3
    FsFile settingsFile;
    #else // SD_FAT_TYPE == 0
    File settingsFile;
    #endif  // SD_FAT_TYPE
    if (settingsFile.open("OLA_Geophone_settings.txt", O_CREAT | O_APPEND | O_WRITE) == false)
    {
      Serial.println(F("Failed to create settings file"));
      return;
    }

    settingsFile.println("sizeOfSettings=" + (String)settings.sizeOfSettings);
    settingsFile.println("olaIdentifier=" + (String)settings.olaIdentifier);
    settingsFile.println("nextDataLogNumber=" + (String)settings.nextDataLogNumber);
    settingsFile.println("enableSD=" + (String)settings.enableSD);
    settingsFile.println("enableTerminalOutput=" + (String)settings.enableTerminalOutput);
    settingsFile.println("logDate=" + (String)settings.logDate);
    settingsFile.println("logTime=" + (String)settings.logTime);
    settingsFile.println("logData=" + (String)settings.logData);
    settingsFile.println("logRTC=" + (String)settings.logRTC);
    settingsFile.println("logHertz=" + (String)settings.logHertz);
    settingsFile.println("getRTCfromGPS=" + (String)settings.getRTCfromGPS);
    settingsFile.println("correctForDST=" + (String)settings.correctForDST);
    settingsFile.println("americanDateStyle=" + (String)settings.americanDateStyle);
    settingsFile.println("hour24Style=" + (String)settings.hour24Style);
    settingsFile.println("serialTerminalBaudRate=" + (String)settings.serialTerminalBaudRate);
    settingsFile.print("localUTCOffset="); settingsFile.println(settings.localUTCOffset);
    settingsFile.println("printDebugMessages=" + (String)settings.printDebugMessages);
    settingsFile.println("powerDownQwiicBusBetweenReads=" + (String)settings.powerDownQwiicBusBetweenReads);
    settingsFile.println("qwiicBusMaxSpeed=" + (String)settings.qwiicBusMaxSpeed);
    settingsFile.println("qwiicBusPowerUpDelayMs=" + (String)settings.qwiicBusPowerUpDelayMs);
    settingsFile.println("printMeasurementCount=" + (String)settings.printMeasurementCount);
    settingsFile.println("openNewLogFilesAfter=" + (String)settings.openNewLogFilesAfter);
    settingsFile.print("threshold="); settingsFile.println(settings.threshold);
    settingsFile.println("serialPlotterMode=" + (String)settings.serialPlotterMode);
    settingsFile.println("useGPIO32ForStopLogging=" + (String)settings.useGPIO32ForStopLogging);
    settingsFile.println("enableLowBatteryDetection=" + (String)settings.enableLowBatteryDetection);
    settingsFile.print("lowBatteryThreshold="); settingsFile.println(settings.lowBatteryThreshold);
    
    updateDataFileAccess(&settingsFile); // Update the file access time & date
    settingsFile.close();
  }
}

//If a config file exists on the SD card, load them and overwrite the local settings
//Heavily based on ReadCsvFile from SdFat library
//Returns true if some settings were loaded from a file
//Returns false if a file was not opened/loaded
bool loadSystemSettingsFromFile()
{
  if (online.microSD == true)
  {
    if (sd.exists("OLA_Geophone_settings.txt"))
    {
      #if SD_FAT_TYPE == 1
      File32 settingsFile;
      #elif SD_FAT_TYPE == 2
      ExFile settingsFile;
      #elif SD_FAT_TYPE == 3
      FsFile settingsFile;
      #else // SD_FAT_TYPE == 0
      File settingsFile;
      #endif  // SD_FAT_TYPE

      if (settingsFile.open("OLA_Geophone_settings.txt", O_READ) == false)
      {
        Serial.println(F("Failed to open device settings file"));
        return (false);
      }

      char line[60];
      int lineNumber = 0;

      while (settingsFile.available()) {
        int n = settingsFile.fgets(line, sizeof(line));
        if (n <= 0) {
          if (settings.serialPlotterMode == false) Serial.printf("Failed to read line %d from settings file\n", lineNumber);
        }
        else if (line[n - 1] != '\n' && n == (sizeof(line) - 1)) {
          if (settings.serialPlotterMode == false) Serial.printf("Settings line %d too long\n", lineNumber);
          if (lineNumber == 0)
          {
            //If we can't read the first line of the settings file, give up
            if (settings.serialPlotterMode == false) Serial.println("Giving up on settings file");
            return (false);
          }
        }
        else if (parseLine(line) == false) {
          if (settings.serialPlotterMode == false) Serial.printf("Failed to parse line %d: %s\n", lineNumber, line);
          if (lineNumber == 0)
          {
            //If we can't read the first line of the settings file, give up
            if (settings.serialPlotterMode == false) Serial.println("Giving up on settings file");
            return (false);
          }
        }

        lineNumber++;
      }

      //Serial.println("Config file read complete");
      settingsFile.close();
      return (true);
    }
    else
    {
      if (settings.serialPlotterMode == false) Serial.println("No config file found. Using settings from EEPROM.");
      //The defaults of the struct will be recorded to a file later on.
      return (false);
    }
  }

  if (settings.serialPlotterMode == false) Serial.println("Config file read failed: SD offline");
  return (false); //SD offline
}

// Check for extra characters in field or find minus sign.
char* skipSpace(char* str) {
  while (isspace(*str)) str++;
  return str;
}

//Convert a given line from file into a settingName and value
//Sets the setting if the name is known
bool parseLine(char* str) {
  char* ptr;

  //Debug
  //Serial.printf("Line contents: %s", str);
  //Serial.flush();

  // Set strtok start of line.
  str = strtok(str, "=");
  if (!str) return false;

  //Store this setting name
  char settingName[40];
  sprintf(settingName, "%s", str);

  //Move pointer to end of line
  str = strtok(nullptr, "\n");
  if (!str) return false;

  //Serial.printf("s = %s\n", str);
  //Serial.flush();

  // Convert string to double.
  double d = strtod(str, &ptr);

  if (str == ptr || *skipSpace(ptr)) return false;

  //Serial.printf("d = %lf\n", d);
  //Serial.flush();

  // Get setting name
  if (strcmp(settingName, "sizeOfSettings") == 0)
  {
    //We may want to cause a factory reset from the settings file rather than the menu
    //If user sets sizeOfSettings to -1 in config file, OLA will factory reset
    if (d == -1)
    {
      EEPROM.erase();
      sd.remove("OLA_Geophone_settings.txt");
      Serial.println("OpenLog Artemis has been factory reset. Freezing. Please restart and open terminal at 115200bps.");
      while (1);
    }

    //Check to see if this setting file is compatible with this version of OLA
    if (d != sizeof(settings))
      Serial.printf("Warning: Settings size is %d but current firmware expects %d. Attempting to use settings from file.\n", d, sizeof(settings));

  }
  else if (strcmp(settingName, "olaIdentifier") == 0)
    settings.olaIdentifier = d;
  else if (strcmp(settingName, "nextDataLogNumber") == 0)
    settings.nextDataLogNumber = d;
  else if (strcmp(settingName, "enableSD") == 0)
    settings.enableSD = d;
  else if (strcmp(settingName, "enableTerminalOutput") == 0)
    settings.enableTerminalOutput = d;
  else if (strcmp(settingName, "logDate") == 0)
    settings.logDate = d;
  else if (strcmp(settingName, "logTime") == 0)
    settings.logTime = d;
  else if (strcmp(settingName, "logData") == 0)
    settings.logData = d;
  else if (strcmp(settingName, "logRTC") == 0)
    settings.logRTC = d;
  else if (strcmp(settingName, "logHertz") == 0)
    settings.logHertz = d;
  else if (strcmp(settingName, "getRTCfromGPS") == 0)
    settings.getRTCfromGPS = d;
  else if (strcmp(settingName, "correctForDST") == 0)
    settings.correctForDST = d;
  else if (strcmp(settingName, "americanDateStyle") == 0)
    settings.americanDateStyle = d;
  else if (strcmp(settingName, "hour24Style") == 0)
    settings.hour24Style = d;
  else if (strcmp(settingName, "serialTerminalBaudRate") == 0)
    settings.serialTerminalBaudRate = d;
  else if (strcmp(settingName, "localUTCOffset") == 0)
    settings.localUTCOffset = d;
  else if (strcmp(settingName, "printDebugMessages") == 0)
    settings.printDebugMessages = d;
  else if (strcmp(settingName, "powerDownQwiicBusBetweenReads") == 0)
    settings.powerDownQwiicBusBetweenReads = d;
  else if (strcmp(settingName, "qwiicBusMaxSpeed") == 0)
    settings.qwiicBusMaxSpeed = d;
  else if (strcmp(settingName, "qwiicBusPowerUpDelayMs") == 0)
    settings.qwiicBusPowerUpDelayMs = d;
  else if (strcmp(settingName, "printMeasurementCount") == 0)
    settings.printMeasurementCount = d;
  else if (strcmp(settingName, "openNewLogFilesAfter") == 0)
    settings.openNewLogFilesAfter = d;
  else if (strcmp(settingName, "threshold") == 0)
    settings.threshold = d;
  else if (strcmp(settingName, "serialPlotterMode") == 0)
    settings.serialPlotterMode = d;
  else if (strcmp(settingName, "useGPIO32ForStopLogging") == 0)
    settings.useGPIO32ForStopLogging = d;
  else if (strcmp(settingName, "enableLowBatteryDetection") == 0)
    settings.enableLowBatteryDetection = d;
  else if (strcmp(settingName, "lowBatteryThreshold") == 0)
    settings.lowBatteryThreshold = d;
  else
    Serial.printf("Unknown setting %s on line: %s\n", settingName, str);

  return (true);
}

//Export the current device settings to a config file
void recordDeviceSettingsToFile()
{
  if (online.microSD == true)
  {
    if (sd.exists("OLA_Geophone_deviceSettings.txt"))
      sd.remove("OLA_Geophone_deviceSettings.txt");

    #if SD_FAT_TYPE == 1
    File32 settingsFile;
    #elif SD_FAT_TYPE == 2
    ExFile settingsFile;
    #elif SD_FAT_TYPE == 3
    FsFile settingsFile;
    #else // SD_FAT_TYPE == 0
    File settingsFile;
    #endif  // SD_FAT_TYPE
    if (settingsFile.open("OLA_Geophone_deviceSettings.txt", O_CREAT | O_APPEND | O_WRITE) == false)
    {
      Serial.println(F("Failed to create device settings file"));
      return;
    }

    //Step through the node list, recording each node's settings
    char base[75];
    node *temp = head;
    while (temp != NULL)
    {
      sprintf(base, "%s.%d.%d.", getDeviceName(temp->deviceType), temp->deviceType, temp->address);

      switch (temp->deviceType)
      {
        case DEVICE_GPS_UBLOX:
          {
            struct_uBlox *nodeSetting = (struct_uBlox *)temp->configPtr;
            settingsFile.println((String)base + "log=" + nodeSetting->log);
            settingsFile.println((String)base + "logDate=" + nodeSetting->logDate);
            settingsFile.println((String)base + "logTime=" + nodeSetting->logTime);
            settingsFile.println((String)base + "logPosition=" + nodeSetting->logPosition);
            settingsFile.println((String)base + "logAltitude=" + nodeSetting->logAltitude);
            settingsFile.println((String)base + "logAltitudeMSL=" + nodeSetting->logAltitudeMSL);
            settingsFile.println((String)base + "logSIV=" + nodeSetting->logSIV);
            settingsFile.println((String)base + "logFixType=" + nodeSetting->logFixType);
            settingsFile.println((String)base + "logCarrierSolution=" + nodeSetting->logCarrierSolution);
            settingsFile.println((String)base + "logGroundSpeed=" + nodeSetting->logGroundSpeed);
            settingsFile.println((String)base + "logHeadingOfMotion=" + nodeSetting->logHeadingOfMotion);
            settingsFile.println((String)base + "logpDOP=" + nodeSetting->logpDOP);
            settingsFile.println((String)base + "logiTOW=" + nodeSetting->logiTOW);
            settingsFile.println((String)base + "i2cSpeed=" + nodeSetting->i2cSpeed);
          }
          break;
        case DEVICE_ADC_ADS122C04:
          {
            struct_ADS122C04 *nodeSetting = (struct_ADS122C04 *)temp->configPtr;
            settingsFile.println((String)base + "log=" + nodeSetting->log);
            settingsFile.println((String)base + "gain=" + nodeSetting->gain);
          }
          break;
        case DEVICE_ADC_ADS1015:
          {
            struct_ADS1015 *nodeSetting = (struct_ADS1015 *)temp->configPtr;
            settingsFile.println((String)base + "log=" + nodeSetting->log);
            settingsFile.println((String)base + "gain=" + nodeSetting->gain);
          }
          break;
        case DEVICE_ADC_ADS1219:
          {
            struct_ADS1219 *nodeSetting = (struct_ADS1219 *)temp->configPtr;
            settingsFile.println((String)base + "log=" + nodeSetting->log);
            settingsFile.println((String)base + "gain=" + nodeSetting->gain);
          }
          break;
        default:
          if (settings.serialPlotterMode == false) Serial.printf("recordSettingsToFile Unknown device: %s\n", base);
          //settingsFile.println((String)base + "=UnknownDeviceSettings");
          break;
      }
      temp = temp->next;
    }
    settingsFile.close();
  }
}

//If a device config file exists on the SD card, load them and overwrite the local settings
//Heavily based on ReadCsvFile from SdFat library
//Returns true if some settings were loaded from a file
//Returns false if a file was not opened/loaded
bool loadDeviceSettingsFromFile()
{
  if (online.microSD == true)
  {
    if (sd.exists("OLA_Geophone_deviceSettings.txt"))
    {
      #if SD_FAT_TYPE == 1
      File32 settingsFile;
      #elif SD_FAT_TYPE == 2
      ExFile settingsFile;
      #elif SD_FAT_TYPE == 3
      FsFile settingsFile;
      #else // SD_FAT_TYPE == 0
      File settingsFile;
      #endif  // SD_FAT_TYPE

      if (settingsFile.open("OLA_Geophone_deviceSettings.txt", O_READ) == false)
      {
        if (settings.serialPlotterMode == false) Serial.println("Failed to open device settings file");
        return (false);
      }

      char line[150];
      int lineNumber = 0;

      while (settingsFile.available()) {
        int n = settingsFile.fgets(line, sizeof(line));
        if (n <= 0) {
          if (settings.serialPlotterMode == false) Serial.printf("Failed to read line %d from settings file\n", lineNumber);
        }
        else if (line[n - 1] != '\n' && n == (sizeof(line) - 1)) {
          if (settings.serialPlotterMode == false) Serial.printf("Settings line %d too long\n", lineNumber);
        }
        else if (parseDeviceLine(line) == false) {
          if (settings.serialPlotterMode == false) Serial.printf("Failed to parse line %d: %s\n", lineNumber + 1, line);
        }

        lineNumber++;
      }

      //Serial.println("Device config file read complete");
      settingsFile.close();
      return (true);
    }
    else
    {
      if (settings.serialPlotterMode == false) Serial.println("No device config file found. Creating one with device defaults.");
      recordDeviceSettingsToFile(); //Record the current settings to create the initial file
      return (false);
    }
  }

  if (settings.serialPlotterMode == false) Serial.println("Device config file read failed: SD offline");
  return (false); //SD offline
}

//Convert a given line from device setting file into a settingName and value
//Immediately applies the setting to the appropriate node
bool parseDeviceLine(char* str) {
  char* ptr;

  //Debug
  //Serial.printf("Line contents: %s", str);
  //Serial.flush();

  // Set strtok start of line.
  str = strtok(str, "=");
  if (!str) return false;

  //Store this setting name
  char settingName[150];
  sprintf(settingName, "%s", str);

  //Move pointer to end of line
  str = strtok(nullptr, "\n");
  if (!str) return false;

  //Serial.printf("s = %s\n", str);
  //Serial.flush();

  // Convert string to double.
  double d = strtod(str, &ptr);
  if (str == ptr || *skipSpace(ptr)) return false;

  //Serial.printf("d = %lf\n", d);
  //Serial.flush();

  //Break device setting into its constituent parts
  char deviceSettingName[50];
  deviceType_e deviceType;
  uint8_t address;
  uint8_t count = 0;
  char *split = strtok(settingName, ".");
  while (split != NULL)
  {
    if (count == 0)
      ; //Do nothing. This is merely the human friendly device name
    else if (count == 1)
      deviceType = (deviceType_e)atoi(split);
    else if (count == 2)
      address = atoi(split);
    else if (count == 3)
      sprintf(deviceSettingName, "%s", split);
    split = strtok(NULL, ".");
    count++;
  }

  if (count < 3)
  {
    if (settings.serialPlotterMode == false) Serial.printf("Incomplete setting: %s\n", settingName);
    return false;
  }

  //Find the device in the list that has this device type and address
  void *deviceConfigPtr = getConfigPointer(deviceType, address);
  if (deviceConfigPtr == NULL)
  {
    //Serial.printf("Setting in file found but no matching device on bus is available: %s\n", settingName);
    //Serial.flush();
  }
  else
  {
    switch (deviceType)
    {
      case DEVICE_MULTIPLEXER:
        {
          if (settings.serialPlotterMode == false) Serial.println("There are no known settings for a multiplexer to load.");
        }
        break;
      case DEVICE_GPS_UBLOX:
        {
          struct_uBlox *nodeSetting = (struct_uBlox *)deviceConfigPtr;

          //Apply the appropriate settings
          if (strcmp(deviceSettingName, "log") == 0)
            nodeSetting->log = d;
          else if (strcmp(deviceSettingName, "logDate") == 0)
            nodeSetting->logDate = d;
          else if (strcmp(deviceSettingName, "logTime") == 0)
            nodeSetting->logTime = d;
          else if (strcmp(deviceSettingName, "logPosition") == 0)
            nodeSetting->logPosition = d;
          else if (strcmp(deviceSettingName, "logAltitude") == 0)
            nodeSetting->logAltitude = d;
          else if (strcmp(deviceSettingName, "logAltitudeMSL") == 0)
            nodeSetting->logAltitudeMSL = d;
          else if (strcmp(deviceSettingName, "logSIV") == 0)
            nodeSetting->logSIV = d;
          else if (strcmp(deviceSettingName, "logFixType") == 0)
            nodeSetting->logFixType = d;
          else if (strcmp(deviceSettingName, "logCarrierSolution") == 0)
            nodeSetting->logCarrierSolution = d;
          else if (strcmp(deviceSettingName, "logGroundSpeed") == 0)
            nodeSetting->logGroundSpeed = d;
          else if (strcmp(deviceSettingName, "logHeadingOfMotion") == 0)
            nodeSetting->logHeadingOfMotion = d;
          else if (strcmp(deviceSettingName, "logpDOP") == 0)
            nodeSetting->logpDOP = d;
          else if (strcmp(deviceSettingName, "logiTOW") == 0)
            nodeSetting->logiTOW = d;
          else if (strcmp(deviceSettingName, "i2cSpeed") == 0)
            nodeSetting->i2cSpeed = d;
          else
            if (settings.serialPlotterMode == false) Serial.printf("Unknown device setting: %s\n", deviceSettingName);
        }
        break;
      case DEVICE_ADC_ADS122C04:
        {
          struct_ADS122C04 *nodeSetting = (struct_ADS122C04 *)deviceConfigPtr; //Create a local pointer that points to same spot as node does
          if (strcmp(deviceSettingName, "log") == 0)
            nodeSetting->log = d;
          else if (strcmp(deviceSettingName, "gain") == 0)
            nodeSetting->gain = d;
          else
            if (settings.serialPlotterMode == false) Serial.printf("Unknown device setting: %s\n", deviceSettingName);
        }
        break;
      case DEVICE_ADC_ADS1015:
        {
          struct_ADS1015 *nodeSetting = (struct_ADS1015 *)deviceConfigPtr; //Create a local pointer that points to same spot as node does
          if (strcmp(deviceSettingName, "log") == 0)
            nodeSetting->log = d;
          else if (strcmp(deviceSettingName, "gain") == 0)
            nodeSetting->gain = d;
          else
            if (settings.serialPlotterMode == false) Serial.printf("Unknown device setting: %s\n", deviceSettingName);
        }
        break;
      case DEVICE_ADC_ADS1219:
        {
          struct_ADS1219 *nodeSetting = (struct_ADS1219 *)deviceConfigPtr; //Create a local pointer that points to same spot as node does
          if (strcmp(deviceSettingName, "log") == 0)
            nodeSetting->log = d;
          else if (strcmp(deviceSettingName, "gain") == 0)
            nodeSetting->gain = (ads1219_gain_config_t)d;
          else
            if (settings.serialPlotterMode == false) Serial.printf("Unknown device setting: %s\n", deviceSettingName);
        }
        break;
      default:
        if (settings.serialPlotterMode == false) Serial.printf("Unknown device type: %d\n", deviceType);
        Serial.flush();
        break;
    }
  }
  return (true);
}
