void menuLogRate()
{
  while (1)
  {
    Serial.println();
    Serial.println("Menu: Configure Terminal Output");

    Serial.print("1) Log to microSD: ");
    if (settings.logData == true) Serial.println("Enabled");
    else Serial.println("Disabled");

    Serial.print("2) Log to Terminal: ");
    if (settings.enableTerminalOutput == true) Serial.println("Enabled");
    else Serial.println("Disabled");

    Serial.print("3) Serial Plotter Mode: ");
    if (settings.serialPlotterMode == true) Serial.println("Enabled");
    else Serial.println("Disabled");

    Serial.print("4) Set Serial Baud Rate: ");
    Serial.print(settings.serialTerminalBaudRate);
    Serial.println(" bps");

    Serial.print("5) Output Measurement Count: ");
    if (settings.printMeasurementCount == true) Serial.println("Enabled");
    else Serial.println("Disabled");

    Serial.print("6) Open New Log Files After (s): ");
    Serial.printf("%d", settings.openNewLogFilesAfter);
    if (settings.openNewLogFilesAfter == 0) Serial.println(" (Never)");
    else Serial.println();

    Serial.print("7) Use pin 32 to Stop Logging: ");
    if (settings.useGPIO32ForStopLogging == true) Serial.println("Yes");
    else Serial.println("No");

    Serial.println("x) Exit");

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming == 1)
    {
      if (settings.logData == true)
      {
        settings.logData = false;
      }
      else
      {
//        Serial.print("\nSD card logging is enabled. Serial plotter mode is disabled.\n");
//        settings.serialPlotterMode = false;
        settings.logData = true;
      }
    }
    else if (incoming == 2)
    {
      if (settings.enableTerminalOutput == true)
      {
        settings.enableTerminalOutput = false;
      }
      else
      {
        settings.enableTerminalOutput = true;
//        if (settings.serialPlotterMode == true)
//        {
//          Serial.print("\nSerial plotter mode enabled. SD card logging is disabled.\n");
//          settings.logData = false;          
//        }
      }
    }
    else if (incoming == 3)
    {
      if (settings.serialPlotterMode == false) // Check if we are trying to enable serial plotter mode
      {
        if (settings.serialTerminalBaudRate < 115200)
        {
          Serial.print("\n*** Serial baud rate is too low. Please set the baud rate to >= 115200 and try again. ***\n");
        }
        else
        {
          settings.serialPlotterMode = true;
//          Serial.print("\nSerial plotter mode enabled. SD card logging is disabled.\n");
//          settings.enableTerminalOutput = true;
//          settings.logData = false;
        }
      }
      else
      {
//        Serial.print("\nSerial plotter mode is disabled. Please enable SD card logging if you need to.\n");
        settings.serialPlotterMode = false;
      }
    }
    else if (incoming == 4)
    {
      Serial.print("Enter baud rate (1200 to 500000): ");
      int newBaud = getNumber(menuTimeout); //Timeout after x seconds
      if (newBaud < 1200 || newBaud > 500000)
      {
        Serial.println("Error: baud rate out of range");
      }
      else
      {
        settings.serialTerminalBaudRate = newBaud;
        recordSystemSettings(); //Normally recorded upon all menu exits
        recordDeviceSettingsToFile(); //Normally recorded upon all menu exits
        Serial.printf("Terminal now set at %dbps. Please reset device and open terminal at new baud rate. Freezing...\n", settings.serialTerminalBaudRate);
        while (1);
      }
    }
    else if (incoming == 5)
      settings.printMeasurementCount ^= 1;
    else if (incoming == 6)
    {
      Serial.println("Open new log files after this many seconds (0 or 10 to 129,600) (0 = Never):");
      int64_t tempSeconds = getNumber(menuTimeout); //Timeout after x seconds
      if ((tempSeconds < 0) || ((tempSeconds > 0) && (tempSeconds < 10)) || (tempSeconds > 129600ULL))
        Serial.println("Error: Invalid interval");
      else
        settings.openNewLogFilesAfter = tempSeconds;
    }
    else if (incoming == 7)
    {
      if (settings.useGPIO32ForStopLogging == true)
      {
        // Disable stop logging
        settings.useGPIO32ForStopLogging = false;
        detachInterrupt(digitalPinToInterrupt(PIN_STOP_LOGGING)); // Disable the interrupt
        pinMode(PIN_STOP_LOGGING, INPUT); // Remove the pull-up
      }
      else
      {
        // Enable stop logging
        settings.useGPIO32ForStopLogging = true;
        pinMode(PIN_STOP_LOGGING, INPUT_PULLUP);
        delay(1); // Let the pin stabilize
        attachInterrupt(digitalPinToInterrupt(PIN_STOP_LOGGING), stopLoggingISR, FALLING); // Enable the interrupt
        stopLoggingSeen = false; // Make sure the flag is clear
      }
    }
    else if (incoming == STATUS_PRESSED_X)
      return;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      return;
    else
      printUnknown(incoming);
  }
}
