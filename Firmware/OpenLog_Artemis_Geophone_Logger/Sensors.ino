void getDateTime()
{
  dateTime[0] = '\0'; //Clear string contents

  if (settings.logRTC)
  {
    //Decide if we are using the internal RTC or GPS for timestamps
    if (settings.getRTCfromGPS == false)
    {
      myRTC.getTime();

      if (settings.logDate)
      {
        char rtcDate[12]; //10/12/2019,
        if (settings.americanDateStyle == true)
          sprintf(rtcDate, "%02d/%02d/20%02d,", myRTC.month, myRTC.dayOfMonth, myRTC.year);
        else
          sprintf(rtcDate, "%02d/%02d/20%02d,", myRTC.dayOfMonth, myRTC.month, myRTC.year);
        strcat(dateTime, rtcDate);
      }

      if (settings.logTime)
      {
        char rtcTime[13]; //09:14:37.41,
        int adjustedHour = myRTC.hour;
        if (settings.hour24Style == false)
        {
          if (adjustedHour > 12) adjustedHour -= 12;
        }
        sprintf(rtcTime, "%02d:%02d:%02d.%02d,", adjustedHour, myRTC.minute, myRTC.seconds, myRTC.hundredths);
        strcat(dateTime, rtcTime);
      }
    } //end if use RTC for timestamp
    else //Use GPS for timestamp
    {
      if (settings.serialPlotterMode == false) Serial.println("Print GPS Timestamp / not yet implemented");
    }
  }
}

//Read the ADC value as int16_t (2 bytes -32,768 to 32,767)
int16_t gatherADCValue()
{
  node *temp = head;
  while (temp != NULL)
  {
    //If this node successfully begin()'d
    if (temp->online == true)
    {
      openConnection(temp->muxAddress, temp->portNumber); //Connect to this device through muxes as needed

      //Switch on device type to set proper class and setting struct
      switch (temp->deviceType)
      {
        case DEVICE_GPS_UBLOX:
          {
            //SFE_UBLOX_GPS *nodeDevice = (SFE_UBLOX_GPS *)temp->classPtr;
            //struct_uBlox *nodeSetting = (struct_uBlox *)temp->configPtr;
          }
          break;
        case DEVICE_ADC_ADS122C04:
          {
            SFE_ADS122C04 *nodeDevice = (SFE_ADS122C04 *)temp->classPtr;
            struct_ADS122C04 *nodeSetting = (struct_ADS122C04 *)temp->configPtr;
    
            // Union to simplify converting from uint16_t to int16_t
            // without using a cast
            union ADC_conversion_union{
              int16_t INT16;
              uint16_t UINT16;
            } ADC_conversion;
            
            // Read the raw (signed) ADC data
            // The ADC data is returned in the least-significant 24-bits
            uint32_t raw_ADC_data = nodeDevice->readADC();
            nodeDevice->start(); // Start the next conversion
            ADC_conversion.UINT16 = (raw_ADC_data >> 8) & 0xffff; // Truncate to 16-bits (signed)
            return(ADC_conversion.INT16); // Return the signed version
          }
          break;
        default:
          //Serial.printf("printDeviceValue unknown device type: %s\n", getDeviceName(temp->deviceType));
          break;
      }
    }
    temp = temp->next;
  }
  return(0);
}

//Configure the ADC for raw measurements at 600Hz
void configureADC()
{
  node *temp = head;
  while (temp != NULL)
  {
    //If this node successfully begin()'d
    if (temp->online == true)
    {
      openConnection(temp->muxAddress, temp->portNumber); //Connect to this device through muxes as needed

      //Switch on device type to set proper class and setting struct
      switch (temp->deviceType)
      {
        case DEVICE_GPS_UBLOX:
          {
            //SFE_UBLOX_GPS *nodeDevice = (SFE_UBLOX_GPS *)temp->classPtr;
            //struct_uBlox *nodeSetting = (struct_uBlox *)temp->configPtr;
          }
          break;
        case DEVICE_ADC_ADS122C04:
          {
            SFE_ADS122C04 *nodeDevice = (SFE_ADS122C04 *)temp->classPtr;
            struct_ADS122C04 *nodeSetting = (struct_ADS122C04 *)temp->configPtr;
    
            nodeDevice->setInputMultiplexer(ADS122C04_MUX_AIN1_AIN0); // Route AIN1 and AIN0 to AINP and AINN
            if (settings.geophoneGain == 128)
            {
              nodeDevice->setGain(ADS122C04_GAIN_128); // Set the gain to 128
              nodeDevice->enablePGA(ADS122C04_PGA_ENABLED); // Enable the Programmable Gain Amplifier
            }
            else if (settings.geophoneGain == 64)
            {
              nodeDevice->setGain(ADS122C04_GAIN_64); // Set the gain to 64
              nodeDevice->enablePGA(ADS122C04_PGA_ENABLED); // Enable the Programmable Gain Amplifier
            }
            else if (settings.geophoneGain == 32)
            {
              nodeDevice->setGain(ADS122C04_GAIN_32); // Set the gain to 32
              nodeDevice->enablePGA(ADS122C04_PGA_ENABLED); // Enable the Programmable Gain Amplifier
            }
            else if (settings.geophoneGain == 16)
            {
              nodeDevice->setGain(ADS122C04_GAIN_16); // Set the gain to 16
              nodeDevice->enablePGA(ADS122C04_PGA_ENABLED); // Enable the Programmable Gain Amplifier
            }
            else if (settings.geophoneGain == 8)
            {
              nodeDevice->setGain(ADS122C04_GAIN_8); // Set the gain to 8
              nodeDevice->enablePGA(ADS122C04_PGA_ENABLED); // Enable the Programmable Gain Amplifier
            }
            else if (settings.geophoneGain == 4)
            {
              nodeDevice->setGain(ADS122C04_GAIN_4); // Set the gain to 4
              nodeDevice->enablePGA(ADS122C04_PGA_DISABLED); // Disable the Programmable Gain Amplifier
            }
            else if (settings.geophoneGain == 2)
            {
              nodeDevice->setGain(ADS122C04_GAIN_2); // Set the gain to 2
              nodeDevice->enablePGA(ADS122C04_PGA_DISABLED); // Disable the Programmable Gain Amplifier
            }
            else
            {
              nodeDevice->setGain(ADS122C04_GAIN_1); // Set the gain to 1
              nodeDevice->enablePGA(ADS122C04_PGA_DISABLED); // Disable the Programmable Gain Amplifier
            }
            nodeDevice->setDataRate(ADS122C04_DATA_RATE_600SPS); // Set the data rate (samples per second) to 600
            nodeDevice->setOperatingMode(ADS122C04_OP_MODE_NORMAL); // Disable turbo mode
            nodeDevice->setConversionMode(ADS122C04_CONVERSION_MODE_SINGLE_SHOT); // Use single shot mode
            nodeDevice->setVoltageReference(ADS122C04_VREF_INTERNAL); // Use the internal 2.048V reference
            nodeDevice->enableInternalTempSensor(ADS122C04_TEMP_SENSOR_OFF); // Disable the temperature sensor
            nodeDevice->setDataCounter(ADS122C04_DCNT_DISABLE); // Disable the data counter (Note: the library does not currently support the data count)
            nodeDevice->setDataIntegrityCheck(ADS122C04_CRC_DISABLED); // Disable CRC checking (Note: the library does not currently support data integrity checking)
            nodeDevice->setBurnOutCurrent(ADS122C04_BURN_OUT_CURRENT_OFF); // Disable the burn-out current
            nodeDevice->setIDACcurrent(ADS122C04_IDAC_CURRENT_OFF); // Disable the IDAC current
            nodeDevice->setIDAC1mux(ADS122C04_IDAC1_DISABLED); // Disable IDAC1
            nodeDevice->setIDAC2mux(ADS122C04_IDAC2_DISABLED); // Disable IDAC2

            if((settings.printDebugMessages == true) && (settings.serialPlotterMode == false))
            {
              nodeDevice->enableDebugging(Serial); //Enable debug messages on Serial
              nodeDevice->printADS122C04config(); //Print the configuration
              nodeDevice->disableDebugging(); //Enable debug messages on Serial
            }
    
            nodeDevice->start(); // Start the first conversion
          }
          break;
        default:
          Serial.printf("printDeviceValue unknown device type: %s\n", getDeviceName(temp->deviceType));
          break;
      }
    }
    temp = temp->next;
  }
}

//If certain devices are attached, we need to reduce the I2C max speed
void setMaxI2CSpeed()
{
  uint32_t maxSpeed = 400000; //Assume 400kHz - but beware! 400kHz with no pull-ups can cause issues.

  //Search nodes for Ublox modules
  node *temp = head;
  while (temp != NULL)
  {
    if (temp->deviceType == DEVICE_GPS_UBLOX)
    {
      //Check if i2cSpeed is lowered
      struct_uBlox *sensor = (struct_uBlox*)temp->configPtr;
      if (sensor->i2cSpeed == 100000)
        maxSpeed = 100000;
    }

    temp = temp->next;
  }

  //If user wants to limit the I2C bus speed, do it here
  if (maxSpeed > settings.qwiicBusMaxSpeed)
    maxSpeed = settings.qwiicBusMaxSpeed;

  qwiic.setClock(maxSpeed);
}

//Read the VIN voltage
float readVIN()
{
  // Only supported on >= V10 hardware
#if(HARDWARE_VERSION_MAJOR == 0)
  return(0.0); // Return 0.0V on old hardware
#else
  int div3 = analogRead(PIN_VIN_MONITOR); //Read VIN across a 1/3 resistor divider
  float vin = (float)div3 * 3.0 * 2.0 / 16384.0; //Convert 1/3 VIN to VIN (14-bit resolution)
  vin = vin * 1.021; //Correct for divider impedance (determined experimentally)
  return (vin);
#endif
}
