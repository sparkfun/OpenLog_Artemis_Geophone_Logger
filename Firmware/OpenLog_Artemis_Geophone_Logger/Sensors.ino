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
    
            // Union to simplify converting from uint32_t to int32_t
            // without using a cast
            union ADC_conversion_union{
              int32_t INT32;
              uint32_t UINT32;
            } ADC_conversion;
            
            // Read the raw (signed) ADC data
            // The ADC data is returned in the least-significant 24-bits
            ADC_conversion.UINT32 = nodeDevice->readADC();
            nodeDevice->start(); // Start the next conversion
            if ((ADC_conversion.UINT32 & 0x00800000) == 0x00800000) // preserve the two's complement
              ADC_conversion.UINT32 |= 0xFF000000;
            return(ADC_conversion.INT32); // Return the signed version
          }
          break;
        case DEVICE_ADC_ADS1015:
          {
            ADS1015 *nodeDevice = (ADS1015 *)temp->classPtr;
            struct_ADS1015 *nodeSetting = (struct_ADS1015 *)temp->configPtr;
    
            int32_t result = (int32_t)nodeDevice->getLastConversionResults(); // Read the conversionm result

            uint16_t config = ADS1015_CONFIG_OS_SINGLE |
                              ADS1015_CONFIG_MUX_DIFF_P0_N1 |
                              nodeSetting->gain |
                              ADS1015_CONFIG_MODE_SINGLE |
                              ADS1015_CONFIG_RATE_920HZ |
                              ADS1015_CONFIG_CQUE_NONE;

            nodeDevice->writeRegister(ADS1015_POINTER_CONFIG, config); // Start the next conversion

            return(result);
          }
          break;
        case DEVICE_ADC_ADS1219:
          {
            SfeADS1219ArdI2C *nodeDevice = (SfeADS1219ArdI2C *)temp->classPtr;
            struct_ADS122C04 *nodeSetting = (struct_ADS122C04 *)temp->configPtr;
    
            /*
            while (nodeDevice->dataReady() == false) // Check if the conversion is complete. This will return true if data is ready.
            {
              delay(1); // The conversion is not complete. Wait a little to avoid pounding the I2C bus.
            }
            */

            nodeDevice->readConversion(); // Read the conversion result from the ADC. Store it internally.
            int32_t result = nodeDevice->getConversionRaw();

            nodeDevice->startSync(); // Start the next conversion

            return(result);
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
  vin = vin * 1.47; //Correct for divider impedance (determined experimentally)
  return (vin);
#endif
}
