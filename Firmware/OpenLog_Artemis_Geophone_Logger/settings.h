//Needed for the MS8607 struct below
#include "SparkFun_PHT_MS8607_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_PHT_MS8607

typedef enum
{
  DEVICE_MULTIPLEXER = 0,
  DEVICE_GPS_UBLOX,
  DEVICE_ADC_ADS122C04,

  DEVICE_TOTAL_DEVICES, //Marks the end, used to iterate loops
  DEVICE_UNKNOWN_DEVICE,
} deviceType_e;

struct node
{
  deviceType_e deviceType;

  uint8_t address = 0;
  uint8_t portNumber = 0;
  uint8_t muxAddress = 0;
  bool online = false; //Goes true once successfully begin()'d

  void *classPtr; //Pointer to this devices' class instantiation
  void *configPtr; //The struct containing this devices logging options
  node *next;
};

node *head = NULL;
node *tail = NULL;

typedef void (*FunctionPointer)(void*); //Used for pointing to device config menus

//Begin specific sensor config structs
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

struct struct_multiplexer {
  //There is nothing about a multiplexer that we want to configure
  //Ignore certain ports at detection step?
};

//Add the new sensor settings below
struct struct_uBlox {
  bool log = true;
  bool logDate = true;
  bool logTime = true;
  bool logPosition = true;
  bool logAltitude = true;
  bool logAltitudeMSL = false;
  bool logSIV = true;
  bool logFixType = true;
  bool logCarrierSolution = false;
  bool logGroundSpeed = true;
  bool logHeadingOfMotion = true;
  bool logpDOP = true;
  bool logiTOW = false;
  int i2cSpeed = 400000;
};

struct struct_ADS122C04 {
  bool log = true;
  bool logCentigrade = true;
  bool logFahrenheit = false;
  bool logInternalTemperature = true;
  bool logRawVoltage = false;
  bool useFourWireMode = true;
  bool useThreeWireMode = false;
  bool useTwoWireMode = false;
  bool useFourWireHighTemperatureMode = false;
  bool useThreeWireHighTemperatureMode = false;
  bool useTwoWireHighTemperatureMode = false;
};

//This is all the settings that can be set on OpenLog. It's recorded to NVM and the config file.
struct struct_settings {
  int sizeOfSettings = 0; //sizeOfSettings **must** be the first entry and must be int
  int olaIdentifier = OLA_IDENTIFIER; // olaIdentifier **must** be the second entry
  int nextDataLogNumber = 1;
  //uint32_t: Largest is 4,294,967,295 or 4,294s or 71 minutes between readings.
  //uint64_t: Largest is 9,223,372,036,854,775,807 or 9,223,372,036,854s or 292,471 years between readings.
  uint64_t usBetweenReadings = 1000000ULL; //Default to 1000,000us = 1000ms = 1 reading per second.
  //100,000 / 1000 = 100ms. 1 / 100ms = 10Hz
  //recordPerSecond (Hz) = 1 / ((usBetweenReadings / 1000UL) / 1000UL)
  //recordPerSecond (Hz) = 1,000,000 / usBetweenReadings
  bool enableRTC = true;
  bool enableSD = true;
  bool enableTerminalOutput = true;
  bool logDate = true;
  bool logTime = true;
  bool logData = true;
  bool logRTC = true;
  bool logHertz = true;
  bool getRTCfromGPS = false;
  bool correctForDST = false;
  bool americanDateStyle = true;
  bool hour24Style = true;
  int serialTerminalBaudRate = 115200;
  int serialLogBaudRate = 9600;
  int localUTCOffset = 0; //Default to UTC because we should
  bool printDebugMessages = false;
  bool powerDownQwiicBusBetweenReads = true;
  int qwiicBusMaxSpeed = 400000;
  int qwiicBusPowerUpDelayMs = 250;
  bool printMeasurementCount = false;
  bool enablePwrLedDuringSleep = true;
  bool logVIN = false;
  unsigned long openNewLogFilesAfter = 0; //Default to 0 (Never) seconds
  double threshold = 0.0; // Default geophone signal threshold
  int geophoneGain = 16; // ADS122C04 Gain: 1,2,4,8,16,32,64,128
  bool serialPlotterMode = false; // If true, only output Serial Plotter-compatible data
} settings;

//These are the devices on board OpenLog that may be on or offline.
struct struct_online {
  bool microSD = false;
  bool dataLogging = false;
} online;
