//Print a message both to terminal and to log
void msg(const char * message)
{
  if (settings.serialPlotterMode == false) Serial.println(message);
  if (online.microSD)
    sensorDataFile.println(message);
}

//Returns next available log file name
//Checks the spots in EEPROM for the next available LOG# file name
//Updates EEPROM and then appends to the new log file.
char* findNextAvailableLog(int &newFileNumber, const char *fileLeader)
{
  //This will contain the file for SD writing
  #if SD_FAT_TYPE == 1
  File32 newFile;
  #elif SD_FAT_TYPE == 2
  ExFile newFile;
  #elif SD_FAT_TYPE == 3
  FsFile newFile;
  #else // SD_FAT_TYPE == 0
  File newFile;
  #endif  // SD_FAT_TYPE

  if (newFileNumber < 2) //If the settings have been reset, let's warn the user that this could take a while!
  {
    if (settings.serialPlotterMode == false) Serial.println(F("Finding the next available log file."));
    if (settings.serialPlotterMode == false) Serial.println(F("This could take a long time if the SD card contains many existing log files."));
  }

  if (newFileNumber > 0)
    newFileNumber--; //Check if last log file was empty. Reuse it if it is.

  //Search for next available log spot
  static char newFileName[40];
  while (1)
  {
    sprintf(newFileName, "%s%05u.TXT", fileLeader, newFileNumber); //Splice the new file number into this file name. Max no. is 99999.

    if (sd.exists(newFileName) == false) break; //File name not found so we will use it.

    //File exists so open and see if it is empty. If so, use it.
    newFile.open(newFileName, O_READ);
    if (newFile.fileSize() == 0) break; // File is empty so we will use it. Note: we need to make the user aware that this can happen!

    newFile.close(); // Close this existing file we just opened.

    newFileNumber++; //Try the next number
    if (newFileNumber >= 100000) break; // Have we hit the maximum number of files?
  }
  
  newFile.close(); //Close this new file we just opened

  newFileNumber++; //Increment so the next power up uses the next file #

  if (newFileNumber >= 100000) // Have we hit the maximum number of files?
  {
    if (settings.serialPlotterMode == false) Serial.print(F("***** WARNING! File number limit reached! (Overwriting "));
    if (settings.serialPlotterMode == false) Serial.print(newFileName);
    if (settings.serialPlotterMode == false) Serial.println(F(") *****"));
    newFileNumber = 100000; // This will overwrite Log99999.TXT next time thanks to the newFileNumber-- above
  }
  else
  {
    if (settings.serialPlotterMode == false) Serial.print(F("Logging to: "));
    if (settings.serialPlotterMode == false) Serial.println(newFileName);    
  }

  //Record new file number to EEPROM and to config file
  //This works because newFileNumber is a pointer to settings.newFileNumber
  recordSystemSettings();

  return (newFileName);
}
