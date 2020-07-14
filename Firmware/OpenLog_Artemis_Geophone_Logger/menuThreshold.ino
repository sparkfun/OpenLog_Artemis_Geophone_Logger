void menuThreshold()
{
  while (1)
  {
    Serial.println();
    Serial.println("Menu: Configure Geophone Gain and Threshold");

    Serial.print("1) Gain: ");
    Serial.printf("%d\n", settings.geophoneGain);
    Serial.print("2) Threshold: ");
    Serial.printf("%f\n", settings.threshold);

    Serial.println("x) Exit");

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming == 1)
    {
      Serial.println("Enter the new gain (1,2,4,8,16,32,64,128):");
      int tempGain = getNumber(menuTimeout); //Timeout after x seconds
      if ((tempGain != 1) && (tempGain != 2) && (tempGain != 4) && (tempGain != 8) && 
        (tempGain != 16) && (tempGain != 32) && (tempGain != 64) && (tempGain != 128))
        Serial.println("Error: Invalid gain");
      else
        settings.geophoneGain = tempGain;
    }
    else if (incoming == 2)
    {
      Serial.println("Enter the new threshold:");
      double tempThreshold = getDouble(menuTimeout); //Timeout after x seconds
      if ((tempThreshold < 0.0) || (tempThreshold > 100000000.0))
        Serial.println("Error: Threshold out of range");
      else
        settings.threshold = tempThreshold;
    }
    else if (incoming == STATUS_PRESSED_X)
    {
      configureADC(); // Reconfigure ADC
      return;
    }
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      return;
    else
      printUnknown(incoming);
  }
}
