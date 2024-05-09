//Let's see what's on the I2C bus
//Creates a linked list of devices
//Creates appropriate classes for each device
//Begin()s each device in list
//Returns true if devices detected > 0
bool detectQwiicDevices()
{
  bool somethingDetected = false;

  qwiic.setClock(100000); //During detection, go slow

  setQwiicPullups(QWIIC_PULLUPS); //Set pullups. (Redundant. beginQwiic has done this too.) If we don't have pullups, detectQwiicDevices() takes ~900ms to complete. We'll disable pullups if something is detected.

  //24k causes a bunch of unknown devices to be falsely detected.
  //setQwiicPullups(24); //Set pullups to 24k. If we don't have pullups, detectQwiicDevices() takes ~900ms to complete. We'll disable pullups if something is detected.

  //Do a prelim scan to see if anything is out there
  for (uint8_t address = 1 ; address < 127 ; address++)
  {
    qwiic.beginTransmission(address);
    if (qwiic.endTransmission() == 0)
    {
      somethingDetected = true;
      break;
    }
  }
  if (somethingDetected == false) return (false);

  if (settings.serialPlotterMode == false) Serial.println("Identifying Qwiic Devices...");

  //Depending on what hardware is configured, the Qwiic bus may have only been turned on a few ms ago
  //Give sensors, specifically those with a low I2C address, time to turn on
  for (int i = 0; i < 100; i++) //SCD30 required >50ms to turn on
  {
    delay(1);
  }

  //Scan the main branch for all devices
  //Serial.println("Scanning main bus");
  for (uint8_t address = 1 ; address < 127 ; address++)
  {
    qwiic.beginTransmission(address);
    if (qwiic.endTransmission() == 0)
    {
      deviceType_e foundType = testDevice(address);
      if (foundType != DEVICE_UNKNOWN_DEVICE)
      {
        if (addDevice(foundType, address) == true) //Records this device. //Returns false if device was already recorded.
        {
          if (settings.printDebugMessages == true)
            Serial.printf("Added %s at address 0x%02X\n", getDeviceName(foundType), address);
        }
      }
    }
  }

  bubbleSortDevices(head);

  //*** PaulZC commented this. Let's leave pull-ups set to 1k and only disable them when taking to a u-blox device ***
  //setQwiicPullups(0); //We've detected something on the bus so disable pullups.

  setMaxI2CSpeed(); //Try for 400kHz but reduce to 100kHz or low if certain devices are attached

  if (settings.serialPlotterMode == false) Serial.println("Autodetect complete");

  return (true);
}

void menuAttachedDevices()
{
  while (1)
  {
    Serial.println(F(""));
    Serial.println(F("Menu: Configure Attached Devices"));

    int availableDevices = 0;

    //Step through node list
    node *temp = head;

    if (temp == NULL)
      Serial.println(F("**No devices detected on Qwiic bus**"));

    while (temp != NULL)
    {
      char strAddress[50];
      sprintf(strAddress, "(0x%02X)", temp->address);

      char strDeviceMenu[10];
      sprintf(strDeviceMenu, "%d)", availableDevices++ + 1);

      switch (temp->deviceType)
      {
        case DEVICE_GPS_UBLOX:
          Serial.printf("%s u-blox GPS Receiver %s\r\n", strDeviceMenu, strAddress);
          break;
        case DEVICE_ADC_ADS122C04:
          Serial.printf("%s ADS122C04 ADC (Qwiic PT100) %s\r\n", strDeviceMenu, strAddress);
          break;
        case DEVICE_ADC_ADS1015:
          Serial.printf("%s ADS1015 ADC %s\r\n", strDeviceMenu, strAddress);
          break;
        case DEVICE_ADC_ADS1219:
          Serial.printf("%s ADS1219 ADC %s\r\n", strDeviceMenu, strAddress);
          break;
        default:
          Serial.printf("Unknown device type %d in menuAttachedDevices\r\n", temp->deviceType);
          break;
      }

      temp = temp->next;
    }

    availableDevices++;
    Serial.printf("%d) Configure Qwiic Settings\r\n", availableDevices);
    availableDevices++;

    Serial.println(F("x) Exit"));

    int nodeNumber = getNumber(menuTimeout); //Timeout after x seconds
    if (nodeNumber > 0 && nodeNumber < availableDevices - 1)
    {
      //Lookup the function we need to call based the node number
      FunctionPointer functionPointer = getConfigFunctionPtr(nodeNumber - 1);

      //Get the configPtr for this given node
      void *deviceConfigPtr = getConfigPointer(nodeNumber - 1);
      functionPointer(deviceConfigPtr); //Call the appropriate config menu with a pointer to this node's configPtr

      configureDevice(nodeNumber - 1); //Reconfigure this device with the new settings
    }
    else if (nodeNumber == availableDevices - 1)
    {
      menuConfigure_QwiicBus();
    }
    else if (nodeNumber == STATUS_PRESSED_X)
      break;
    else if (nodeNumber == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(nodeNumber);
  }
}

void menuConfigure_QwiicBus()
{
  while (1)
  {
    Serial.println();
    Serial.println("Menu: Configure Qwiic Bus");

    Serial.printf("1) Set Max Qwiic Bus Speed: %d Hz\n", settings.qwiicBusMaxSpeed);

    Serial.printf("2) Set Qwiic bus power up delay: %d ms\n", settings.qwiicBusPowerUpDelayMs);

    Serial.println("x) Exit");

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
    {
      Serial.print("Enter max frequency to run Qwiic bus: (100000 to 400000): ");
      int amt = getNumber(menuTimeout);
      if (amt >= 100000 && amt <= 400000)
        settings.qwiicBusMaxSpeed = amt;
      else
        Serial.println("Error: Out of range");
    }
    else if (incoming == '2')
    {
      Serial.print("Enter number of milliseconds to wait for Qwiic VCC to stabilize before communication: (1 to 1000): ");
      int amt = getNumber(menuTimeout);
      if (amt >= 1 && amt <= 1000)
        settings.qwiicBusPowerUpDelayMs = amt;
      else
        Serial.println("Error: Out of range");
    }
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }
}

void menuConfigure_uBlox(void *configPtr)
{
  //struct_uBlox *sensorSetting = (struct_uBlox*)configPtr;

  Serial.println();
  Serial.println("Menu: Configure uBlox GPS Receiver");

  Serial.println("There are currently no configurable options for this device.");
  for (int i = 0; i < 500; i++)
  {
    delay(1);
  }
}

bool isUbloxAttached()
{
  //Step through node list
  node *temp = head;

  while (temp != NULL)
  {
    switch (temp->deviceType)
    {
      case DEVICE_GPS_UBLOX:
        return (true);
    }
    temp = temp->next;
  }

  return(false);
}

void getUbloxDateTime(int &year, int &month, int &day, int &hour, int &minute, int &second, int &millisecond, bool &dateValid, bool &timeValid)
{
  //Step through node list
  node *temp = head;

  while (temp != NULL)
  {
    switch (temp->deviceType)
    {
      case DEVICE_GPS_UBLOX:
      {
        setQwiicPullups(0); //Disable pullups to minimize CRC issues

        SFE_UBLOX_GNSS *nodeDevice = (SFE_UBLOX_GNSS *)temp->classPtr;
        struct_uBlox *nodeSetting = (struct_uBlox *)temp->configPtr;

        //Get latested date/time from GPS
        //These will be extracted from a single PVT packet
        year = nodeDevice->getYear();
        month = nodeDevice->getMonth();
        day = nodeDevice->getDay();
        hour = nodeDevice->getHour();
        minute = nodeDevice->getMinute();
        second = nodeDevice->getSecond();
        dateValid = nodeDevice->getDateValid();
        timeValid = nodeDevice->getTimeValid();
        millisecond = nodeDevice->getMillisecond();

        setQwiicPullups(QWIIC_PULLUPS); //Re-enable pullups
      }
    }
    temp = temp->next;
  }
}

void menuConfigure_ADS122C04(void *configPtr)
{
  struct_ADS122C04 *sensorSetting = (struct_ADS122C04*)configPtr;

  while (1)
  {
  Serial.println();
  Serial.println("Menu: Configure ADS122C04 ADC (Qwiic PT100)");

    Serial.print("1) Gain: ");

    switch (sensorSetting->gain)
    {
      case ADS122C04_GAIN_128:
        Serial.println("128");
        break;
      case ADS122C04_GAIN_64:
        Serial.println("64");
        break;
      case ADS122C04_GAIN_32:
        Serial.println("32");
        break;
      case ADS122C04_GAIN_16:
        Serial.println("16");
        break;
      case ADS122C04_GAIN_8:
        Serial.println("8");
        break;
      case ADS122C04_GAIN_4:
        Serial.println("4");
        break;
      case ADS122C04_GAIN_2:
        Serial.println("2");
        break;
      case ADS122C04_GAIN_1:
        Serial.println("1");
        break;
    }

    Serial.println("x) Exit");

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
    {
      if (sensorSetting->gain == ADS122C04_GAIN_1)
        sensorSetting->gain = ADS122C04_GAIN_128;
      else
        sensorSetting->gain -= 1;
    }
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }
}

void menuConfigure_ADS1015(void *configPtr)
{
  struct_ADS1015 *sensorSetting = (struct_ADS1015*)configPtr;

  while (1)
  {
  Serial.println();
  Serial.println("Menu: Configure ADS1015 ADC");

    Serial.print("1) Gain: ");

    switch (sensorSetting->gain)
    {
      case ADS1015_CONFIG_PGA_TWOTHIRDS:
        Serial.println("2/3");
        break;
      case ADS1015_CONFIG_PGA_1:
        Serial.println("1");
        break;
      case ADS1015_CONFIG_PGA_2:
        Serial.println("2");
        break;
      case ADS1015_CONFIG_PGA_4:
        Serial.println("4");
        break;
      case ADS1015_CONFIG_PGA_8:
        Serial.println("8");
        break;
      case ADS1015_CONFIG_PGA_16:
        Serial.println("16");
        break;
    }

    Serial.println("x) Exit");

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
    {
      if (sensorSetting->gain == ADS1015_CONFIG_PGA_TWOTHIRDS)
        sensorSetting->gain = ADS1015_CONFIG_PGA_16;
      else if (sensorSetting->gain == ADS1015_CONFIG_PGA_1)
        sensorSetting->gain = ADS1015_CONFIG_PGA_TWOTHIRDS;
      else if (sensorSetting->gain == ADS1015_CONFIG_PGA_2)
        sensorSetting->gain = ADS1015_CONFIG_PGA_1;
      else if (sensorSetting->gain == ADS1015_CONFIG_PGA_4)
        sensorSetting->gain = ADS1015_CONFIG_PGA_2;
      else if (sensorSetting->gain == ADS1015_CONFIG_PGA_8)
        sensorSetting->gain = ADS1015_CONFIG_PGA_4;
      else if (sensorSetting->gain == ADS1015_CONFIG_PGA_16)
        sensorSetting->gain = ADS1015_CONFIG_PGA_8;
    }
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }
}

void menuConfigure_ADS1219(void *configPtr)
{
  struct_ADS1219 *sensorSetting = (struct_ADS1219*)configPtr;

  while (1)
  {
  Serial.println();
  Serial.println("Menu: Configure ADS1219 ADC");

    Serial.print("1) Gain: ");

    switch (sensorSetting->gain)
    {
      case ADS1219_GAIN_1:
        Serial.println("1");
        break;
      case ADS1219_GAIN_4:
        Serial.println("4");
        break;
    }

    Serial.println("x) Exit");

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
    {
      if (sensorSetting->gain == ADS1219_GAIN_1)
        sensorSetting->gain = ADS1219_GAIN_4;
      else if (sensorSetting->gain == ADS1219_GAIN_4)
        sensorSetting->gain = ADS1219_GAIN_1;
    }
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }
}
