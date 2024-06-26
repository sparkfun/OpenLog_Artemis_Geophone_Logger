// This code is inspired by Ole Wolf's geophone: https://github.com/olewolf/geophone

// Based on Ambiq Micro's stimer.c example:
// https://github.com/sparkfun/AmbiqSuiteSDK/tree/master/boards/apollo3_evb/examples/stimer
// The FFT analysis code is taken from Ambiq's pdm_fft example:
// https://github.com/sparkfun/Arduino_Apollo3/blob/master/libraries/PDM/examples/Example1_MicrophoneOutput/Example1_MicrophoneOutput.ino

#define ARM_MATH_CM4
#include <arm_math.h>

#define GEOPHONE_FFT_SIZE   1024 // 1024 samples at 512Hz
#define FREQ_LIMIT          501  // Frequency limit for plotting / saving (250.5Hz in 0.5Hz bins))

volatile static bool g_bGeophoneDataReady = false;
volatile static bool g_bUsingGeophoneBuffer1 = true;
volatile static uint32_t g_ui32GeophoneDataBufferPointer = 0;
volatile static int32_t g_ui32GeophoneDataBuffer1[GEOPHONE_FFT_SIZE];
volatile static int32_t g_ui32GeophoneDataBuffer2[GEOPHONE_FFT_SIZE];
float g_fGeophoneTimeDomain[GEOPHONE_FFT_SIZE * 2];
float g_fGeophoneFrequencyDomain[GEOPHONE_FFT_SIZE * 2];
float g_fGeophoneMagnitudes[GEOPHONE_FFT_SIZE * 2];
uint32_t g_ui32SampleFreq = 500;  // We'll need to set the ADS122C04 to 600Hz.


/*
 * Interrupt service routine for sampling the geodata.  The geodata analog
 * pin is sampled at each invokation of the ISR.  If the buffer is full, a
 * pointer is passed to the main program and a semaphor is raised to indicate
 * that a new frame of samples is available, and future samples are written
 * to the other buffer.
 *
 * While not a sampling task, we take advantage of the timer interrupt to
 * blink the report LED if enabled.
 */
void sampling_interrupt()
{
  if (samplingEnabled == false)
    return;

  //digitalWrite(PIN_LOGIC_DEBUG, !digitalRead(PIN_LOGIC_DEBUG));

  /* Read a sample and store it in the geodata buffer.  Apply a Hamming
     window as we go along.  It involves a cos operation; the alternative
     is an array that should be fit into program memory. */

  /* Read the geodata sample from the ADS122C04. */
#if defined(TEST_PERIOD_1) && defined(TEST_AMPLITUDE_1) && defined(TEST_PERIOD_2) && defined(TEST_AMPLITUDE_2) // If TEST_PERIOD 1+2 are defined then fake the data
  int32_t geodata_sample = (int32_t)((TEST_AMPLITUDE_1 * sin(2.0 * M_PI * ((double)(micros() % TEST_PERIOD_1)) / ((double)TEST_PERIOD_1)))
    + (TEST_AMPLITUDE_2 * cos(2.0 * M_PI * ((double)(micros() % TEST_PERIOD_2)) / ((double)TEST_PERIOD_2))));
#elif (defined(TEST_PERIOD_1) && defined(TEST_AMPLITUDE_1)) // If TEST_PERIOD 1 is defined then fake the data
  int32_t geodata_sample = TEST_AMPLITUDE_1 * sin(2.0 * M_PI * ((double)(micros() % TEST_PERIOD_1)) / ((double)TEST_PERIOD_1));
#else  
  int32_t geodata_sample = gatherADCValue(); // Take a real sample from the ADC
#endif

  if (g_bUsingGeophoneBuffer1) // Write the geophone sample into the appropriate buffer
  {
    g_ui32GeophoneDataBuffer1[g_ui32GeophoneDataBufferPointer] = geodata_sample;
  }
  else
  {
    g_ui32GeophoneDataBuffer2[g_ui32GeophoneDataBufferPointer] = geodata_sample;
  }
  g_ui32GeophoneDataBufferPointer++; // Increment the pointer
  if (g_ui32GeophoneDataBufferPointer == GEOPHONE_FFT_SIZE) // Have we reached the end of the buffer?
  {
    g_bUsingGeophoneBuffer1 ^= 1; // Swap to the other buffer
    g_ui32GeophoneDataBufferPointer = 0; // Reset the pointer
    g_bGeophoneDataReady = true; // Set the flag
  }
}

/**
 * Main program loop which performs a frequency analysis each time the
 * geophone sample buffer has been filled and creates a report with the
 * frequency/amplitude data.
 */
bool geophone_loop()
{
  /* Analyze the geophone data once it's available. */
  if ( g_bGeophoneDataReady == true )
  {
    float fMaxValue;
    uint32_t ui32MaxIndex;
    int32_t *pi32GeophoneData;
    uint32_t ui32LoudestFrequency;

    geophoneData[0] = '\0'; //Clear string contents
    geophoneDataSerial[0] = '\0';
    peakFreq[0] = '\0';
    char tempData[50];

    g_bGeophoneDataReady = false; // Clear the flag
    
    if (g_bUsingGeophoneBuffer1)
    {
      pi32GeophoneData = (int32_t *) g_ui32GeophoneDataBuffer2; // Point to the full buffer
    }
    else
    {
      pi32GeophoneData = (int32_t *) g_ui32GeophoneDataBuffer1; // Point to the full buffer
    }
      
    //
    // Convert the samples to floats, and arrange them in the format
    // required by the FFT function.
    //
    for (uint32_t i = 0; i < GEOPHONE_FFT_SIZE; i++)
    {
      g_fGeophoneTimeDomain[2 * i] = pi32GeophoneData[i] / 1.0;
      g_fGeophoneTimeDomain[2 * i + 1] = 0.0;
    }

    //
    // Perform the FFT.
    //
    arm_cfft_radix4_instance_f32 S;
    arm_cfft_radix4_init_f32(&S, GEOPHONE_FFT_SIZE, 0, 1);
    arm_cfft_radix4_f32(&S, g_fGeophoneTimeDomain);
    arm_cmplx_mag_f32(g_fGeophoneTimeDomain, g_fGeophoneMagnitudes, GEOPHONE_FFT_SIZE);

    //
    // Find the frequency bin with the largest magnitude.
    //
    arm_max_f32(g_fGeophoneMagnitudes, GEOPHONE_FFT_SIZE / 2, &fMaxValue, &ui32MaxIndex);

    ui32LoudestFrequency = (g_ui32SampleFreq * ui32MaxIndex) / GEOPHONE_FFT_SIZE;

    if (fMaxValue >= (float)settings.threshold) // Check if the threshold has been exceeded
    {
      for (uint32_t i = 1; i < FREQ_LIMIT; i++)
      {
        char tempStr[16];
        olaftoa(g_fGeophoneMagnitudes[i], tempStr, 2, sizeof(tempStr) / sizeof(char));
        sprintf(tempData, "%s,", tempStr);
        strcat(geophoneData, tempData);
        sprintf(tempData, "%s\n", tempStr);
        strcat(geophoneDataSerial, tempData);
      }
  
      sprintf(tempData, "%d", ui32LoudestFrequency);
      strcat(geophoneData, tempData);
      strcat(peakFreq, tempData);

      char tempStr[16];
      olaftoa(fMaxValue, tempStr, 2, sizeof(tempStr) / sizeof(char));
      sprintf(tempData, ",%s", tempStr);
      strcat(peakFreq, tempData);
      
      if (settings.printMeasurementCount)
      {
        sprintf(tempData, ",%d" , measurementCount);
        strcat(geophoneData, tempData);
        strcat(peakFreq, tempData);
      }
      
      sprintf(tempData, "\n"); // Add a LF to the end of the geophoneData
      strcat(geophoneData, tempData);
      
      return(true);
    }
  }
  return(false);
}
