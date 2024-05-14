void menuThreshold()
{
  while (1)
  {
    Serial.println();
    Serial.println("Menu: Configure Geophone Threshold");

    Serial.print("1) Threshold: ");
    char tempStr[16];
    olaftoa(settings.threshold, tempStr, 3, sizeof(tempStr) / sizeof(char));
    Serial.printf("%s\r\n", tempStr);

    Serial.println("x) Exit");

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming == 1)
    {
      Serial.println("Enter the new threshold:");
      double tempThreshold = getDouble(menuTimeout); //Timeout after x seconds
      if ((tempThreshold < 0.0) || (tempThreshold > 100000000.0))
        Serial.println("Error: Threshold out of range");
      else
        settings.threshold = tempThreshold;
    }
    else if (incoming == STATUS_PRESSED_X)
      return;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      return;
    else
      printUnknown(incoming);
  }
}
