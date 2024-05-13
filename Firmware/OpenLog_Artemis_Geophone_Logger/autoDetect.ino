//Given node number, get a pointer to the node
node *getNodePointer(uint8_t nodeNumber)
{
  //Search the list of nodes
  node *temp = head;

  int counter = 0;
  while (temp != NULL)
  {
    if (nodeNumber == counter)
      return (temp);
    counter++;
    temp = temp->next;
  }

  return (NULL);
}

node *getNodePointer(deviceType_e deviceType, uint8_t address)
{
  //Search the list of nodes
  node *temp = head;
  while (temp != NULL)
  {
    if (temp->address == address)
      if (temp->deviceType == deviceType)
        return (temp);

    temp = temp->next;
  }
  return (NULL);
}

//Given nodenumber, pull out the device type
deviceType_e getDeviceType(uint8_t nodeNumber)
{
  node *temp = getNodePointer(nodeNumber);
  if (temp == NULL) return (DEVICE_UNKNOWN_DEVICE);
  return (temp->deviceType);
}

//Given nodeNumber, return the config pointer
void *getConfigPointer(uint8_t nodeNumber)
{
  //Search the list of nodes
  node *temp = getNodePointer(nodeNumber);
  if (temp == NULL) return (NULL);
  return (temp->configPtr);
}

//Given a bunch of ID'ing info, return the config pointer to a node
void *getConfigPointer(deviceType_e deviceType, uint8_t address)
{
  //Search the list of nodes
  node *temp = getNodePointer(deviceType, address);
  if (temp == NULL) return (NULL);
  return (temp->configPtr);
}

//Add a device to the linked list
//Creates a class but does not begin or configure the device
bool addDevice(deviceType_e deviceType, uint8_t address)
{
  //Ignore devices we've already logged
  if (deviceExists(deviceType, address) == true) return false;

  //Create class instantiation for this device
  //Create logging details for this device

  node *temp = new node; //Create the node in memory

  //Setup this node
  temp->deviceType = deviceType;
  temp->address = address;

  //Instantiate a class and settings struct for this device
  switch (deviceType)
  {
    case DEVICE_GPS_UBLOX:
      {
        temp->classPtr = new SFE_UBLOX_GNSS;
        temp->configPtr = new struct_uBlox;
      }
      break;
    case DEVICE_ADC_ADS122C04:
      {
        temp->classPtr = new SFE_ADS122C04;
        temp->configPtr = new struct_ADS122C04;
      }
      break;
    case DEVICE_ADC_ADS1015:
      {
        temp->classPtr = new ADS1015;
        temp->configPtr = new struct_ADS1015;
      }
      break;
    case DEVICE_ADC_ADS1219:
      {
        temp->classPtr = new SfeADS1219ArdI2C;
        temp->configPtr = new struct_ADS1219;
      }
      break;
    default:
      if (settings.serialPlotterMode == false) Serial.printf("addDevice Device type not found: %d\n", deviceType);
      break;
  }

  //Link to next node
  temp->next = NULL;
  if (head == NULL)
  {
    head = temp;
    tail = temp;
    temp = NULL;
  }
  else
  {
    tail->next = temp;
    tail = temp;
  }

  return true;
}

//Begin()'s all devices in the node list
bool beginQwiicDevices()
{
  bool everythingStarted = true;

  //Step through the list
  node *temp = head;

  if (temp == NULL)
  {
    if (settings.serialPlotterMode == false) Serial.println("beginDevices: No devices detected");
    return (true);
  }

  while (temp != NULL)
  {
    //Attempt to begin the device
    switch (temp->deviceType)
    {
      case DEVICE_GPS_UBLOX:
        {
          setQwiicPullups(0); //Disable pullups for u-blox comms.
          SFE_UBLOX_GNSS *tempDevice = (SFE_UBLOX_GNSS *)temp->classPtr;
          struct_uBlox *nodeSetting = (struct_uBlox *)temp->configPtr; //Create a local pointer that points to same spot as node does
          temp->online = tempDevice->begin(qwiic, temp->address); //Wire port, Address
          setQwiicPullups(QWIIC_PULLUPS); //Re-enable pullups.
        }
        break;
      case DEVICE_ADC_ADS122C04:
        {
          SFE_ADS122C04 *tempDevice = (SFE_ADS122C04 *)temp->classPtr;
          if (tempDevice->begin(temp->address, qwiic) == true) //Address, Wire port. Returns true on success.
            temp->online = true;
        }
        break;
      case DEVICE_ADC_ADS1015:
        {
          ADS1015 *tempDevice = (ADS1015 *)temp->classPtr;
          if (tempDevice->begin(temp->address, qwiic) == true) //Address, Wire port. Returns true on success.
            temp->online = true;
        }
        break;
      case DEVICE_ADC_ADS1219:
        {
          SfeADS1219ArdI2C *tempDevice = (SfeADS1219ArdI2C *)temp->classPtr;
          if (tempDevice->begin(qwiic, temp->address) == true) //Wire port, Address. Returns true on success.
            temp->online = true;
        }
        break;
      default:
        if (settings.serialPlotterMode == false) Serial.printf("addDevice Device type not found: %d\n", temp->deviceType);
        break;
    }

    if (temp->online == false) everythingStarted = false;

    temp = temp->next;
  }

  return everythingStarted;
}

//Pretty print all the online devices
void printOnlineDevice()
{
  //Step through the list
  node *temp = head;

  if (temp == NULL)
  {
    if (settings.serialPlotterMode == false) Serial.println("printOnlineDevice: No devices detected");
    return;
  }

  while (temp != NULL)
  {
    char sensorOnlineText[75];
    if (temp->online)
    {
      sprintf(sensorOnlineText, "%s online at address 0x%02X\n", getDeviceName(temp->deviceType), temp->address);
    }
    else
    {
      sprintf(sensorOnlineText, "%s failed to respond\n", getDeviceName(temp->deviceType));
    }
    if (settings.serialPlotterMode == false) Serial.print(sensorOnlineText);

    temp = temp->next;
  }
}

//Given the node number, apply the node's configuration settings to the device
void configureDevice(uint8_t nodeNumber)
{
  node *temp = getNodePointer(nodeNumber);
  configureDevice(temp);
}

//Given the node pointer, apply the node's configuration settings to the device
void configureDevice(node * temp)
{
  uint8_t deviceType = (uint8_t)temp->deviceType;

  switch (deviceType)
  {
    case DEVICE_GPS_UBLOX:
      {
        setQwiicPullups(0); //Disable pullups for u-blox comms.
        
        SFE_UBLOX_GNSS *sensor = (SFE_UBLOX_GNSS *)temp->classPtr;
        struct_uBlox *sensorSetting = (struct_uBlox *)temp->configPtr;

        sensor->setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)

        //sensor->setAutoPVT(true); //Tell the GPS to "send" each solution
        sensor->setAutoPVT(false); //We will poll the device for PVT solutions
        sensor->setNavigationFrequency(1); //Set output rate to 1Hz

        sensor->saveConfiguration(); //Save the current settings to flash and BBR
        
        setQwiicPullups(QWIIC_PULLUPS); //Re-enable pullups.
      }
      break;
    case DEVICE_ADC_ADS122C04:
      {
        SFE_ADS122C04 *sensor = (SFE_ADS122C04 *)temp->classPtr;
        struct_ADS122C04 *sensorSetting = (struct_ADS122C04 *)temp->configPtr;
      }
      break;
    case DEVICE_ADC_ADS1015:
      {
        ADS1015 *sensor = (ADS1015 *)temp->classPtr;
        struct_ADS1015 *sensorSetting = (struct_ADS1015 *)temp->configPtr;
      }
      break;
    case DEVICE_ADC_ADS1219:
      {
        SfeADS1219ArdI2C *sensor = (SfeADS1219ArdI2C *)temp->classPtr;
        struct_ADS1219 *sensorSetting = (struct_ADS1219 *)temp->configPtr;
      }
      break;
    default:
      if (settings.serialPlotterMode == false) Serial.printf("configureDevice: Unknown device type %d: %s\n", deviceType, getDeviceName((deviceType_e)deviceType));
      break;
  }
}

//Apply device settings to each node
void configureQwiicDevices()
{
  //Step through the list
  node *temp = head;

  while (temp != NULL)
  {
    configureDevice(temp);
    temp = temp->next;
  }
}

//Returns a pointer to the menu function that configures this particular device type
FunctionPointer getConfigFunctionPtr(uint8_t nodeNumber)
{
  FunctionPointer ptr = NULL;

  node *temp = getNodePointer(nodeNumber);
  if (temp == NULL) return (NULL);
  deviceType_e deviceType = temp->deviceType;

  switch (deviceType)
  {
    case DEVICE_GPS_UBLOX:
      ptr = (FunctionPointer)menuConfigure_uBlox;
      break;
    case DEVICE_ADC_ADS122C04:
      ptr = (FunctionPointer)menuConfigure_ADS122C04;
      break;
    case DEVICE_ADC_ADS1015:
      ptr = (FunctionPointer)menuConfigure_ADS1015;
      break;
    case DEVICE_ADC_ADS1219:
      ptr = (FunctionPointer)menuConfigure_ADS1219;
      break;
    default:
      if (settings.serialPlotterMode == false) Serial.println("getConfigFunctionPtr: Unknown device type");
      Serial.flush();
      break;
  }

  return (ptr);
}

//Search the linked list for a given address
//Returns true if this device address already exists in our system
bool deviceExists(deviceType_e deviceType, uint8_t address)
{
  node *temp = head;
  while (temp != NULL)
  {
    if (temp->address == address)
      if (temp->deviceType == deviceType) return (true);

    temp = temp->next;
  }
  return (false);
}

//Bubble sort the given linked list by the device address
//https://www.geeksforgeeks.org/c-program-bubble-sort-linked-list/
void bubbleSortDevices(struct node * start)
{
  int swapped, i;
  struct node *ptr1;
  struct node *lptr = NULL;

  //Checking for empty list
  if (start == NULL) return;

  do
  {
    swapped = 0;
    ptr1 = start;

    while (ptr1->next != lptr)
    {
      if (ptr1->address > ptr1->next->address)
      {
        swap(ptr1, ptr1->next);
        swapped = 1;
      }
      ptr1 = ptr1->next;
    }
    lptr = ptr1;
  }
  while (swapped);
}

//Swap data of two nodes a and b
void swap(struct node * a, struct node * b)
{
  node temp;

  temp.deviceType = a->deviceType;
  temp.address = a->address;
  temp.online = a->online;
  temp.classPtr = a->classPtr;
  temp.configPtr = a->configPtr;

  a->deviceType = b->deviceType;
  a->address = b->address;
  a->online = b->online;
  a->classPtr = b->classPtr;
  a->configPtr = b->configPtr;

  b->deviceType = temp.deviceType;
  b->address = temp.address;
  b->online = temp.online;
  b->classPtr = temp.classPtr;
  b->configPtr = temp.configPtr;
}

//The functions below are specific to the steps of auto-detection rather than node manipulation
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Available Qwiic devices
//We no longer use defines in the search table. These are just here for reference.
#define ADR_UBLOX 0x42 //But can be set to any address
#define ADR_ADS122C04 0x45 //Alternates: 0x44, 0x41 and 0x40
#define ADR_ADS1015 0x48 //Alternates: 0x49, 0x4A and 0x4B
#define ADR_ADS1219 0x40 //Alternates: 0x41 to 0x4F

//Given an address, returns the device type if it responds as we would expect
deviceType_e testDevice(uint8_t i2cAddress)
{
  switch (i2cAddress)
  {
    case 0x40:
      {
        //Confidence: Medium - reads Configuration Register
        SfeADS1219ArdI2C sensor;
        if (sensor.begin(qwiic, i2cAddress) == true) //Wire port, Address
          return (DEVICE_ADC_ADS1219);
      }
      break;
    case 0x42:
      {
        //Confidence: High - Sends/receives CRC checked data response
      
        setQwiicPullups(0); //Disable pullups to minimize CRC issues
        SFE_UBLOX_GNSS sensor;
        if (sensor.begin(qwiic, i2cAddress) == true) //Wire port, address
        {
          setQwiicPullups(QWIIC_PULLUPS); //Re-enable pullups to prevent ghosts at 0x43 onwards
          return (DEVICE_GPS_UBLOX);
        }
        setQwiicPullups(QWIIC_PULLUPS); //Re-enable pullups for normal discovery
      }
      break;
    case 0x45:
      {
        //Confidence: High - Configures ADC mode
        SFE_ADS122C04 sensor;
        if (sensor.begin(i2cAddress, qwiic) == true) //Address, Wire port
          return (DEVICE_ADC_ADS122C04);
      }
      break;
    case 0x48:
      {
        //Confidence: Low - basic beginTransmission test
        ADS1015 sensor;
        if (sensor.begin(i2cAddress, qwiic) == true) //Address, Wire port
          return (DEVICE_ADC_ADS1015);
      }
      break;
    default:
      {
        if (settings.serialPlotterMode == false) Serial.printf("Unknown device at address (0x%02X)\n", i2cAddress);
        return DEVICE_UNKNOWN_DEVICE;
      }
      break;
  }
  if (settings.serialPlotterMode == false) Serial.printf("Known I2C address but device failed identification at address 0x%02X\n", i2cAddress);
  return DEVICE_UNKNOWN_DEVICE;
}

//Given a device number return the string associated with that entry
const char* getDeviceName(deviceType_e deviceNumber)
{
  switch (deviceNumber)
  {
    case DEVICE_GPS_UBLOX:
      return "GPS-ublox";
      break;
    case DEVICE_ADC_ADS122C04:
      return "ADC-ADS122C04";
      break;
    case DEVICE_ADC_ADS1015:
      return "ADC-ADS1015";
      break;
    case DEVICE_ADC_ADS1219:
      return "ADC-ADS1219";
      break;

    case DEVICE_UNKNOWN_DEVICE:
      return "Unknown device";
      break;
    default:
      return "Unknown Status";
      break;
  }
  return "None";
}
