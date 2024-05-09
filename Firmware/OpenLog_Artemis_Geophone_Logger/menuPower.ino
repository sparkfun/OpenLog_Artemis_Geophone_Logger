void menuPower()
{
  while (1)
  {
    Serial.println();
    Serial.println("Menu: Configure Power Options");

    //Keep Qwiic bus powered on during powerDown if user desires it - but only to let X04 avoid a brown-out
    Serial.print("1) Turn off Qwiic bus power during powerDown: ");
    if (settings.powerDownQwiicBusBetweenReads == true) Serial.println("Yes");
    else Serial.println("No");

#if(HARDWARE_VERSION_MAJOR >= 1)
    Serial.print(F("2) Low Battery Voltage Detection: "));
    if (settings.enableLowBatteryDetection == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("3) Low Battery Threshold (V): "));
    char tempStr[16];
    olaftoa(settings.lowBatteryThreshold, tempStr, 2, sizeof(tempStr) / sizeof(char));
    Serial.printf("%s\r\n", tempStr);
#endif

    Serial.println("x) Exit");

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
    {
      settings.powerDownQwiicBusBetweenReads ^= 1;
    }
#if(HARDWARE_VERSION_MAJOR >= 1)
    else if (incoming == '2')
    {
      settings.enableLowBatteryDetection ^= 1;
    }
    else if (incoming == '3')
    {
      Serial.println(F("Please enter the new low battery threshold:"));
      float tempBT = (float)getDouble(menuTimeout); //Timeout after x seconds
      if ((tempBT < 3.0) || (tempBT > 6.0))
        Serial.println(F("Error: Threshold out of range"));
      else
        settings.lowBatteryThreshold = tempBT;
    }
#endif
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }
}
