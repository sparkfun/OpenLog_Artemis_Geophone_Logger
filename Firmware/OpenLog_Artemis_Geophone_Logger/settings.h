typedef enum
{
  DEVICE_MULTIPLEXER = 0,
  DEVICE_GPS_UBLOX,
  DEVICE_ADC_ADS122C04,
  DEVICE_ADC_ADS1015,
  DEVICE_ADC_ADS1219,

  DEVICE_TOTAL_DEVICES, //Marks the end, used to iterate loops
  DEVICE_UNKNOWN_DEVICE,
} deviceType_e;

struct node
{
  deviceType_e deviceType;

  uint8_t address = 0;
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
  int gain = ADS122C04_GAIN_128;
};

struct struct_ADS1015 {
  bool log = true;
  int gain = ADS1015_CONFIG_PGA_16;
};

struct struct_ADS1219 {
  bool log = true;
  ads1219_gain_config_t gain = ADS1219_GAIN_4;
};

//This is all the settings that can be set on OpenLog. It's recorded to NVM and the config file.
struct struct_settings {
  int sizeOfSettings = 0; //sizeOfSettings **must** be the first entry and must be int
  int olaIdentifier = OLA_IDENTIFIER; // olaIdentifier **must** be the second entry
  int nextDataLogNumber = 1;
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
  int  serialTerminalBaudRate = 115200;
  float localUTCOffset = 0; //Default to UTC because we should
  bool printDebugMessages = false;
  bool powerDownQwiicBusBetweenReads = true; // 29 chars!
  int  qwiicBusMaxSpeed = 100000; // 400kHz with no pull-ups can cause issues. Default to 100kHz. User can change to 400 if required.
  int  qwiicBusPowerUpDelayMs = 250;
  bool printMeasurementCount = false;
  unsigned long openNewLogFilesAfter = 0; //Default to 0 (Never) seconds
  double threshold = 0.0; // Default geophone signal threshold
  bool serialPlotterMode = false; // If true, only output Serial Plotter-compatible data
  bool useGPIO32ForStopLogging = false; //If true, use GPIO as a stop logging button
  bool enableLowBatteryDetection = false; // Low battery detection
  float lowBatteryThreshold = 3.4; // Low battery voltage threshold (Volts)
} settings;

//These are the devices on board OpenLog that may be on or offline.
struct struct_online {
  bool microSD = false;
  bool dataLogging = false;
} online;
