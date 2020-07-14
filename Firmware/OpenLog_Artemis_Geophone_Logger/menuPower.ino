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

    Serial.println("x) Exit");

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
    {
      settings.powerDownQwiicBusBetweenReads ^= 1;
    }
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }
}
