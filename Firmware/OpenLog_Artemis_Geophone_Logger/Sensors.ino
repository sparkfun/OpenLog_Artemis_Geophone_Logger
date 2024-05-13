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
        char rtcDate[11]; // 10/12/2019
        char rtcDay[3];
        char rtcMonth[3];
        char rtcYear[5];
        if (myRTC.dayOfMonth < 10)
          sprintf(rtcDay, "0%d", myRTC.dayOfMonth);
        else
          sprintf(rtcDay, "%d", myRTC.dayOfMonth);
        if (myRTC.month < 10)
          sprintf(rtcMonth, "0%d", myRTC.month);
        else
          sprintf(rtcMonth, "%d", myRTC.month);
        if (myRTC.year < 10)
          sprintf(rtcYear, "200%d", myRTC.year);
        else
          sprintf(rtcYear, "20%d", myRTC.year);
        if (settings.americanDateStyle == true)
          sprintf(rtcDate, "%s/%s/%s,", rtcMonth, rtcDay, rtcYear);
        else
          sprintf(rtcDate, "%s/%s/%s,", rtcDay, rtcMonth, rtcYear);

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
        char rtcHour[3];
        char rtcMin[3];
        char rtcSec[3];
        char rtcHundredths[3];
        if (adjustedHour < 10)
          sprintf(rtcHour, "0%d", adjustedHour);
        else
          sprintf(rtcHour, "%d", adjustedHour);
        if (myRTC.minute < 10)
          sprintf(rtcMin, "0%d", myRTC.minute);
        else
          sprintf(rtcMin, "%d", myRTC.minute);
        if (myRTC.seconds < 10)
          sprintf(rtcSec, "0%d", myRTC.seconds);
        else
          sprintf(rtcSec, "%d", myRTC.seconds);
        if (myRTC.hundredths < 10)
          sprintf(rtcHundredths, "0%d", myRTC.hundredths);
        else
          sprintf(rtcHundredths, "%d", myRTC.hundredths);
        sprintf(rtcTime, "%s:%s:%s.%s,", rtcHour, rtcMin, rtcSec, rtcHundredths);

        strcat(dateTime, rtcTime);
      }
    } //end if use RTC for timestamp
    else //Use GPS for timestamp
    {
      if (settings.serialPlotterMode == false) Serial.println("Print GPS Timestamp / not yet implemented");
    }
  }
}

//Read the ADC value
int32_t gatherADCValue()
{
  node *temp = head;
  while (temp != NULL)
  {
    //If this node successfully begin()'d
    if (temp->online == true)
    {
      //Switch on device type to set proper class and setting struct
      switch (temp->deviceType)
      {
        case DEVICE_GPS_UBLOX:
          {
            //SFE_UBLOX_GNSS *nodeDevice = (SFE_UBLOX_GNSS *)temp->classPtr;
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
            return((int32_t)ADC_conversion.INT16); // Return the signed version
          }
          break;
        case DEVICE_ADC_ADS1015:
          {
            ADS1015 *nodeDevice = (ADS1015 *)temp->classPtr;
            struct_ADS1015 *nodeSetting = (struct_ADS1015 *)temp->configPtr;
    
            return((int32_t)nodeDevice->getDifferential());
          }
          break;
        case DEVICE_ADC_ADS1219:
          {
            SfeADS1219ArdI2C *nodeDevice = (SfeADS1219ArdI2C *)temp->classPtr;
            struct_ADS122C04 *nodeSetting = (struct_ADS122C04 *)temp->configPtr;
    
            while (nodeDevice->dataReady() == false) // Check if the conversion is complete. This will return true if data is ready.
            {
              delay(1); // The conversion is not complete. Wait a little to avoid pounding the I2C bus.
            }

            nodeDevice->readConversion(); // Read the conversion result from the ADC. Store it internally.
            return(nodeDevice->getConversionRaw());
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
      //Switch on device type to set proper class and setting struct
      switch (temp->deviceType)
      {
        case DEVICE_GPS_UBLOX:
          {
            //SFE_UBLOX_GNSS *nodeDevice = (SFE_UBLOX_GNSS *)temp->classPtr;
            //struct_uBlox *nodeSetting = (struct_uBlox *)temp->configPtr;
          }
          break;
        case DEVICE_ADC_ADS122C04:
          {
            SFE_ADS122C04 *nodeDevice = (SFE_ADS122C04 *)temp->classPtr;
            struct_ADS122C04 *nodeSetting = (struct_ADS122C04 *)temp->configPtr;
    
            nodeDevice->setInputMultiplexer(ADS122C04_MUX_AIN1_AIN0); // Route AIN1 and AIN0 to AINP and AINN

            if (nodeSetting->gain == ADS122C04_GAIN_128)
            {
              nodeDevice->setGain(ADS122C04_GAIN_128); // Set the gain to 128
              nodeDevice->enablePGA(ADS122C04_PGA_ENABLED); // Enable the Programmable Gain Amplifier
            }
            else if (nodeSetting->gain == ADS122C04_GAIN_64)
            {
              nodeDevice->setGain(ADS122C04_GAIN_64); // Set the gain to 64
              nodeDevice->enablePGA(ADS122C04_PGA_ENABLED); // Enable the Programmable Gain Amplifier
            }
            else if (nodeSetting->gain == ADS122C04_GAIN_32)
            {
              nodeDevice->setGain(ADS122C04_GAIN_32); // Set the gain to 32
              nodeDevice->enablePGA(ADS122C04_PGA_ENABLED); // Enable the Programmable Gain Amplifier
            }
            else if (nodeSetting->gain == ADS122C04_GAIN_16)
            {
              nodeDevice->setGain(ADS122C04_GAIN_16); // Set the gain to 16
              nodeDevice->enablePGA(ADS122C04_PGA_ENABLED); // Enable the Programmable Gain Amplifier
            }
            else if (nodeSetting->gain == ADS122C04_GAIN_8)
            {
              nodeDevice->setGain(ADS122C04_GAIN_8); // Set the gain to 8
              nodeDevice->enablePGA(ADS122C04_PGA_ENABLED); // Enable the Programmable Gain Amplifier
            }
            else if (nodeSetting->gain == ADS122C04_GAIN_4)
            {
              nodeDevice->setGain(ADS122C04_GAIN_4); // Set the gain to 4
              nodeDevice->enablePGA(ADS122C04_PGA_DISABLED); // Disable the Programmable Gain Amplifier
            }
            else if (nodeSetting->gain == ADS122C04_GAIN_2)
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
        case DEVICE_ADC_ADS1015:
          {
            ADS1015 *nodeDevice = (ADS1015 *)temp->classPtr;
            struct_ADS1015 *nodeSetting = (struct_ADS1015 *)temp->configPtr;

            if (nodeSetting->gain == ADS1015_CONFIG_PGA_TWOTHIRDS)
            {
              nodeDevice->setGain(ADS1015_CONFIG_PGA_TWOTHIRDS); // Set the gain to 2/3
            }
            else if (nodeSetting->gain == ADS1015_CONFIG_PGA_1)
            {
              nodeDevice->setGain(ADS1015_CONFIG_PGA_1); // Set the gain to 1
            }
            else if (nodeSetting->gain == ADS1015_CONFIG_PGA_2)
            {
              nodeDevice->setGain(ADS1015_CONFIG_PGA_2); // Set the gain to 2
            }
            else if (nodeSetting->gain == ADS1015_CONFIG_PGA_4)
            {
              nodeDevice->setGain(ADS1015_CONFIG_PGA_4); // Set the gain to 4
            }
            else if (nodeSetting->gain == ADS1015_CONFIG_PGA_8)
            {
              nodeDevice->setGain(ADS1015_CONFIG_PGA_8); // Set the gain to 8
            }
            else
            {
              nodeDevice->setGain(ADS1015_CONFIG_PGA_16); // Set the gain to 16
            }

            nodeDevice->setSampleRate(ADS1015_CONFIG_RATE_920HZ); // 490Hz isn't fast enough - use 920Hz
            nodeDevice->useConversionReady(true); // Check the Config Register Operational Status bit
            // Use single shot conversions    
          }
          break;
        case DEVICE_ADC_ADS1219:
          {
            SfeADS1219ArdI2C *nodeDevice = (SfeADS1219ArdI2C *)temp->classPtr;
            struct_ADS1219 *nodeSetting = (struct_ADS1219 *)temp->configPtr;
    
            if (nodeSetting->gain == ADS1219_GAIN_1)
            {
              nodeDevice->setGain(ADS1219_GAIN_1); // Set the gain to 1
            }
            else
            {
              nodeDevice->setGain(ADS1219_GAIN_4); // Set the gain to 4
            }

            nodeDevice->setDataRate(ADS1219_DATA_RATE_1000SPS);
            nodeDevice->setInputMultiplexer(ADS1219_CONFIG_MUX_DIFF_P0_N1);
            nodeDevice->setConversionMode(ADS1219_CONVERSION_SINGLE_SHOT);
            nodeDevice->startSync(); // Start the first conversion
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
