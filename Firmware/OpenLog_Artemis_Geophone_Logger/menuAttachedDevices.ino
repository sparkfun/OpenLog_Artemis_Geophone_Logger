/*
  To add a new sensor to the system:

  Add the library in OpenLog_Artemis
  Add DEVICE_ name to settings.h
  Add struct_MCP9600 to settings.h - This will define what settings for the sensor we will control

  Add gathering of data to gatherDeviceValues() in Sensors
  Add helper text to printHelperText() in Sensors

  Add class creation to addDevice() in autoDetect
  Add begin functions to beginQwiicDevices() in autoDetect
  Add configuration functions to configureDevice() in autoDetect
  Add pointer to configuration menu name to getConfigFunctionPtr() in autodetect
  Add test case to testDevice() in autoDetect
  Add pretty print device name to getDeviceName() in autoDetect

  Add menu title to menuAttachedDevices() list in menuAttachedDevices
  Create a menuConfigure_LPS25HB() function in menuAttachedDevices

  Add settings to the save/load device file settings in nvm
*/


//Let's see what's on the I2C bus
//Scan I2C bus including sub-branches of multiplexers
//Creates a linked list of devices
//Creates appropriate classes for each device
//Begin()s each device in list
//Returns true if devices detected > 0
bool detectQwiicDevices()
{
  bool somethingDetected = false;

  qwiic.setClock(100000); //During detection, go slow

  qwiic.setPullups(QWIIC_PULLUPS); //Set pullups. (Redundant. beginQwiic has done this too.) If we don't have pullups, detectQwiicDevices() takes ~900ms to complete. We'll disable pullups if something is detected.

  //24k causes a bunch of unknown devices to be falsely detected.
  //qwiic.setPullups(24); //Set pullups to 24k. If we don't have pullups, detectQwiicDevices() takes ~900ms to complete. We'll disable pullups if something is detected.

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

  //First scan for Muxes. Valid addresses are 0x70 to 0x77.
  //If any are found, they will be begin()'d causing their ports to turn off
  //Serial.println("Scanning for multiplexers");
  uint8_t muxCount = 0;
  for (uint8_t address = 0x70 ; address < 0x78 ; address++)
  {
    qwiic.beginTransmission(address);
    if (qwiic.endTransmission() == 0)
    {
      deviceType_e foundType = testDevice(address, 0, 0); //No mux or port numbers for this test
      if (foundType == DEVICE_MULTIPLEXER)
      {
        addDevice(foundType, address, 0, 0); //Add this device to our map
        muxCount++;
      }
    }
  }

  if (muxCount > 0) beginQwiicDevices(); //Because we are about to use a multiplexer, begin() the muxes.

  //Before going into sub branches, complete the scan of the main branch for all devices
  //Serial.println("Scanning main bus");
  for (uint8_t address = 1 ; address < 127 ; address++)
  {
    qwiic.beginTransmission(address);
    if (qwiic.endTransmission() == 0)
    {
      deviceType_e foundType = testDevice(address, 0, 0); //No mux or port numbers for this test
      if (foundType != DEVICE_UNKNOWN_DEVICE)
      {
        if (addDevice(foundType, address, 0, 0) == true) //Records this device. //Returns false if device was already recorded.
        {
          if (settings.printDebugMessages == true)
            Serial.printf("Added %s at address 0x%02X\n", getDeviceName(foundType), address);
        }
      }
    }
  }

  //If we have muxes, begin scanning their sub nets
  if (muxCount > 0)
  {
    if (settings.serialPlotterMode == false) Serial.println("Multiplexers found. Scanning sub nets...");

    //Step into first mux and begin stepping through ports
    for (int muxNumber = 0 ; muxNumber < muxCount ; muxNumber++)
    {
      //The node tree starts with muxes so we can align node numbers
      node *muxNode = getNodePointer(muxNumber);
      QWIICMUX *myMux = (QWIICMUX *)muxNode->classPtr;

      for (int portNumber = 0 ; portNumber < 7 ; portNumber++)
      {
        myMux->setPort(portNumber);
        
        //Scan this new bus for new addresses
        for (uint8_t address = 1 ; address < 127 ; address++)
        {
          qwiic.beginTransmission(address);
          if (qwiic.endTransmission() == 0)
          {
            somethingDetected = true;

            deviceType_e foundType = testDevice(address, muxNode->address, portNumber);
            if (foundType != DEVICE_UNKNOWN_DEVICE)
            {
              if (addDevice(foundType, address, muxNode->address, portNumber) == true) //Record this device, with mux port specifics.
              {
                //Serial.printf("-Added %s at address 0x%02X.0x%02X.%d\n", getDeviceName(foundType), address, muxNode->address, portNumber);
              }
            }
          } //End I2c check
        } //End I2C scanning
      } //End mux port stepping
    } //End mux stepping
  } //End mux > 0

  bubbleSortDevices(head); //This may destroy mux alignment to node 0.

  //*** PaulZC commented this. Let's leave pull-ups set to 1k and only disable them when taking to a u-blox device ***
  //qwiic.setPullups(0); //We've detected something on the bus so disable pullups.

  setMaxI2CSpeed(); //Try for 400kHz but reduce to 100kHz or low if certain devices are attached

  if (settings.serialPlotterMode == false) Serial.println("Autodetect complete");

  return (true);
}

//void menuAttachedDevices()
//{
//  while (1)
//  {
//    Serial.println();
//    Serial.println("Menu: Configure Attached Devices");
//
//    int availableDevices = 0;
//
//    //Step through node list
//    node *temp = head;
//
//    if (temp == NULL)
//      Serial.println("**No devices detected on Qwiic bus**");
//
//    while (temp != NULL)
//    {
//      //Exclude multiplexers from the list
//      if (temp->deviceType != DEVICE_MULTIPLEXER)
//      {
//        char strAddress[50];
//        if (temp->muxAddress == 0)
//          sprintf(strAddress, "(0x%02X)", temp->address);
//        else
//          sprintf(strAddress, "(0x%02X)(Mux:0x%02X Port:%d)", temp->address, temp->muxAddress, temp->portNumber);
//
//        char strDeviceMenu[10];
//        sprintf(strDeviceMenu, "%d)", availableDevices++ + 1);
//
//        switch (temp->deviceType)
//        {
//          case DEVICE_MULTIPLEXER:
//            //Serial.printf("%s Multiplexer %s\n", strDeviceMenu, strAddress);
//            break;
//          case DEVICE_GPS_UBLOX:
//            Serial.printf("%s u-blox GPS Receiver %s\n", strDeviceMenu, strAddress);
//            break;
//          case DEVICE_ADC_ADS122C04:
//            Serial.printf("%s ADS122C04 ADC (Qwiic PT100) %s\n", strDeviceMenu, strAddress);
//            break;
//          default:
//            Serial.printf("Unknown device type %d in menuAttachedDevices\n", temp->deviceType);
//            break;
//        }
//      }
//
//      temp = temp->next;
//    }
//
//    Serial.printf("%d) Configure Qwiic Settings\n", availableDevices++ + 1);
//
//    Serial.println("x) Exit");
//
//    int nodeNumber = getNumber(menuTimeout); //Timeout after x seconds
//    if (nodeNumber > 0 && nodeNumber < availableDevices)
//    {
//      //Lookup the function we need to call based the node number
//      FunctionPointer functionPointer = getConfigFunctionPtr(nodeNumber - 1);
//
//      //Get the configPtr for this given node
//      void *deviceConfigPtr = getConfigPointer(nodeNumber - 1);
//      functionPointer(deviceConfigPtr); //Call the appropriate config menu with a pointer to this node's configPtr
//
//      configureDevice(nodeNumber - 1); //Reconfigure this device with the new settings
//    }
//    else if (nodeNumber == availableDevices)
//    {
//      menuConfigure_QwiicBus();
//    }
//    else if (nodeNumber == STATUS_PRESSED_X)
//      break;
//    else if (nodeNumber == STATUS_GETNUMBER_TIMEOUT)
//      break;
//    else
//      printUnknown(nodeNumber);
//  }
//}

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

void menuConfigure_Multiplexer(void *configPtr)
{
  //struct_multiplexer *sensor = (struct_multiplexer*)configPtr;

  Serial.println();
  Serial.println("Menu: Configure Multiplexer");

  Serial.println("There are currently no configurable options for this device.");
  for (int i = 0; i < 500; i++)
  {
    delay(1);
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
        qwiic.setPullups(0); //Disable pullups to minimize CRC issues

        SFE_UBLOX_GPS *nodeDevice = (SFE_UBLOX_GPS *)temp->classPtr;
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

        qwiic.setPullups(QWIIC_PULLUPS); //Re-enable pullups
      }
    }
    temp = temp->next;
  }
}

void menuConfigure_ADS122C04(void *configPtr)
{
  //struct_ADS122C04 *sensorSetting = (struct_ADS122C04*)configPtr;

  Serial.println();
  Serial.println("Menu: Configure ADS122C04 ADC (Qwiic PT100)");

  Serial.println("There are currently no configurable options for this device.");
  for (int i = 0; i < 500; i++)
  {
    delay(1);
  }
}
