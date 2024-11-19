// OPEN A NEW SKETCH WINDOW IN ARDUINO
// CLICK IN THIS BOX, CTL-A, CTL-C (Copy code from text box.)
// CLICK IN SKETCH, CTL-A, CTL-V (Paste code into sketch.)

// Breathing Rate Detection System -- Final Integration
//
// Pieced together from code created by: Clark Hochgraf and David Orlicki Oct 18, 2018
// Modified by: Mark Thompson April 2020 to integrate MATLAB read and Write 
//              and integrate the system

#include <MsTimer2.h>
#include <SPI.h>


const int TSAMP_MSEC = 100;
const int NUM_SAMPLES = 3600;  // 3600;
const int NUM_SUBSAMPLES = 160;
const int DAC0 = 3, DAC1 = 4, DAC2 = 5, LM61 = A0, VDITH = A1;
const int V_REF = 5.0;
const int SPKR = 12; // d12  PB4

// MFILT is the kernel length.  Must be an odd number
const int MFILT = 31; // MFILT points from 0...MFILT-1

// FC_BPM is the corner frequency of the filter in breaths per minute
// MOD_BPM is not used
const int FC_BPM = 70, MOD_BPM = 0;

//FC is the normalized corner frequency.  Normalized to the sample rate
const float FC = FC_BPM/600.0; // fs = 10 Hz -> 600 bpm

//FM is the normalized MOD frequency.  Normalized to the sample rate
const float FM = MOD_BPM/600.0;

//HFXPT is the scale value to convert to a fixed point kernel
const long HFXPT = 10000;


//  Storage for the kernel values
int h[MFILT] = {0};
int hFilt[MFILT] = {0};

volatile boolean sampleFlag = false;

const long DATA_FXPT = 1000; // Scale value to convert from float to fixed
const float INV_FXPT = 1.0 / DATA_FXPT; // division slow: precalculate


int nSmpl = 1, sample;

float xv, yv, yLF, yMF, yHF, stdLF, stdMF, stdHF;
float printArray[9];
int numValues = 0;


int loopTick = 0;
bool statsReset;
bool isToneEn = false;

unsigned long startUsec, endUsec, execUsec;

//  Define a structure to hold statistics values for each filter band
struct stats_t
{
  int tick = 1;
  float mean, var, stdev;
} statsLF, statsMF, statsHF;

//**********************************************************************
void setup()
{

  configureArduino();
  Serial.begin(115200);delay(5);

   //Handshake with MATLAB 
  Serial.println(F("%Arduino Ready"));
  while (Serial.read() != 'g'); // spin

  MsTimer2::set(TSAMP_MSEC, ISR_Sample); // Set sample msec, ISR name
  MsTimer2::start(); // start running the Timer  

}


////**********************************************************************
void loop()
{

  // syncSample();  // Wait for the interupt when actually reading ADC data

  
  // Breathing Rate Detection

  // Declare variables

  float readValue, floatOutput;  //  Input data from ADC after dither averaging or from MATLAB
  long fxdInputValue, lpfInput, lpfOutput;  
  long eqOutput;  //  Equalizer output
  int alarmCode;  //  Alarm code


  // ******************************************************************
  //  When finding the impulse responses of the filters use this as an input
  //  Create a Delta function in time with the first sample a 1 and all others 0
  //xv = (loopTick == 0) ? 1.0 : 0.0; // impulse test input

  // ******************************************************************
  //  Use this when the test vector generator is used as an input
    xv = testVector();


  // ******************************************************************
  //  Read input value in ADC counts  -- Get simulated data from MATLAB
  //readValue = ReadFromMATLAB();

  // ******************************************************************
  //  Read input value from ADC using Dithering, and averaging
  readValue = analogReadDitherAve();


  //  Convert the floating point number to a fixed point value.  First
  //  scale the floating point value by a number to increase its resolution
  //  (use DATA_FXPT).  Then round the value and truncate to a fixed point
  //  INT datatype

  fxdInputValue = long(DATA_FXPT * readValue + 0.5);

  //  Execute the equalizer
  //eqOutput = EqualizerFIR( fxdInputValue, loopTick );
  
  //  Execute the noise filter.  
//  eqOutput = NoiseFilter( eqOutput, loopTick );

  //  Convert the output of the equalizer by scaling floating point
  xv = float(eqOutput) * INV_FXPT;


  //*******************************************************************
  // Uncomment this when measuring execution times
  //startUsec = micros();

  // ******************************************************************
  //  Compute the output of the filter using the cascaded SOS sections
   yLF = IIR_Low(xv); // second order systems cascade  
   yMF = IIR_Middle(xv);
   yHF = IIR_High(xv);

  //  Compute the latest output of the running stats for the output of the filters.
  //  Pass the entire set of output values, the latest stats structure and the reset flag

  
  statsReset = (statsLF.tick%100 == 0);
  getStats( yLF, statsLF, statsReset);
  stdLF = statsLF.stdev;

  statsReset = (statsMF.tick%100 == 0);
  getStats( yMF, statsMF, statsReset);
  stdMF = statsMF.stdev;

  statsReset = (statsHF.tick%100 == 0);
  getStats( yHF, statsHF, statsReset);
  stdHF = statsHF.stdev;


  //*******************************************************************
  // Uncomment this when measuring execution times
  // endUsec = micros();
  // execUsec = execUsec + (endUsec-startUsec);

  //  Call the alarm check function to determine what breathing range 
  //  alarmCode = AlarmCheck( stdLF, stdMF, stdHF );

  //  Call the alarm function to turn on or off the tone
  //setAlarm(alarmCode, isToneEn );

  
 // To print data to the serial port, use the WriteToSerial function.  
 //
 //  This is a generic way to print out variable number of values
 //
 //  There are two input arguments to the function:
 //  printArray -- An array of values that are to be printed starting with the first column
 //  numValues -- An integer indicating the number of values in the array.  
 
   printArray[0] = loopTick;  //  The sample number -- always print this
   printArray[1] = xv;        //  Column 2
   
//   printArray[2] = yLF;       //  Column 3
//   printArray[3] = yMF;       //  Column 4, etc...
//   printArray[4] = yHF;
//   printArray[5] = stdLF;
//   printArray[6] = stdMF;
//   printArray[7] = stdHF;
//   printArray[8] = float(alarmCode);

   numValues = 2;  // The number of columns to be sent to the serial monitor (or MATLAB)

 WriteToSerial( numValues, printArray );  //  Write to the serial monitor (or MATLAB)

  if (++loopTick >= NUM_SAMPLES){
    //Serial.print("Average execution time (uSec) = ");Serial.println( float(execUsec)/NUM_SAMPLES );
    while(true); // spin forever
  }

} // loop()

//******************************************************************* Noise Filter
void NoiseFilter(float Fc, int h[])
//
// This function computes a Windowed SINC lowpass filter.
//  
//  The code operates by first building a floating point 
//  version of the filter and while doing so summing
//  all the kernel values.  Then the filter is created again
//  using floating point numbers, but the floating point values are
//  converted to fixed point at the end by dividing by the sum
//  of the kernel values to set the filter gain to 1.0 and then
//  scaling by HFXPT to convert the numbers to fixed point.
//
//  Windowed Sinc Lowpass Filter Kernel, M odd
{
 
  float hv, accumRaw = 0.0;

  // This part of the code builds the filter once and finds the filter gain
  //
  //  Iterate from i = 0 to MFILT-1.  MFILT/2 is the center point
   
  for (int i=0; i < MFILT; i++)
  {

    //  hv is the argument of the SINC function.  See Page 285 in Smith
    //  It is a function of the normalized corner frequency of the filter.
     
    hv = TWO_PI*Fc*(i-MFILT/2);

    // if the point is the center point then the argument hv will be 0 and
    // dividing by zero is a problem, so correct for this point.  In the limit
    // sin(0)/0 will be 1.0 so set the point to 1.0
    
    if (i == (MFILT/2)) hv = 1.0; // L'Hopital point repair for the center point

    // Otherwise compute SINC normally.  SINC(hv) = sin(hv)/hv
    
    else hv = sin(hv)/hv;

    //  Now compute the value of the Hamming Window a this point and
    //  multiply the kernel value by this window value
    
    hv *= 0.54-0.46*cos(TWO_PI*i/(MFILT-1)); 

    //  Add upt the values of each point in the kernel to find the gain
    //  of the filter.  This will be used later to scale the kernel values
    
    accumRaw += hv; // floating point sinc DC gain normalizer
  }

  //  Now build the filter again, but scale each value in the kernel by the gain
  //  and the value of HFXPT and convert the values to fixed point.  See comments above
  //  to determine what each line below is doing.
  
  for (int i=0; i < MFILT; i++)
  {
    hv = TWO_PI*Fc*(i-MFILT/2);
    if (i == (MFILT/2)) hv = 1.0; // L'Hopital point repair
    else hv = sin(hv)/hv; // sinc
    hv *= 0.54-0.46*cos(TWO_PI*i/(MFILT-1)); // window

    //  Normalize by the filter gain and scale for fixed point
    
    h[i] = int(HFXPT*hv/accumRaw + ((hv > 0.0) ? 0.5 : -0.5)); // round, fix point
  }
}

//****************************************************************** Alarm Check
int AlarmCheck( float stdLF, float stdMF, float stdHF)
{


//  Your alarm check logic code will go here.

  
//return alarmCode;

}  // end AlarmCheck



//****************************************************************** Equalizer for Input

//long EqualizerFIR(long xInput )
//{
//
//  int i;
//  long yN=0; //  Current output
//  const int equalizerLength = 4;
//  static long xN[equalizerLength] = {0};
//  long h[] = {1, 1, -1, -1};  // Impulse response of the equalizer
//
//  //  Update the xN array
//
//  for ( i = equalizerLength-1 ; i >= 1; i-- )
//  {
//    xN[i] = xN[i - 1];
//  }
//
//  xN[0] = xInput;
//
//  //  Convolve the input with the impulse response
//
//  for ( i = 0; i <= equalizerLength-1 ; i++)
//  {
//    yN += h[i] * xN[i];
//  }
//
//  if (tick < equalizerLength)
//  {
//    return 0;
//  }
//  else
//  {
//   return yN;
//  }
//
//}

//*******************************************************************************
float IIR_Low(float xv)
{  


  //  ***  Copy variable declarations from MATLAB generator to here  ****

//Filter specific variable declarations
const int numStages = 3;
static float G[numStages];
static float b[numStages][3];
static float a[numStages][3];
//  *** Stop copying MATLAB variable declarations here
  
  int stage;
  int i;
  static float xM0[numStages] = {0.0}, xM1[numStages] = {0.0}, xM2[numStages] = {0.0};
  static float yM0[numStages] = {0.0}, yM1[numStages] = {0.0}, yM2[numStages] = {0.0};
  
  float yv = 0.0;
  unsigned long startTime;



//  ***  Copy variable initialization code from MATLAB generator to here  ****

//CHEBY low, order 5, R = 0.5, 12 BPM

G[0] = 0.0054630;
b[0][0] = 1.0000000; b[0][1] = 0.9990472; b[0][2]= 0.0000000;
a[0][0] = 1.0000000; a[0][1] =  -0.9554256; a[0][2] =  0.0000000;
G[1] = 0.0054630;
b[1][0] = 1.0000000; b[1][1] = 2.0015407; b[1][2]= 1.0015416;
a[1][0] = 1.0000000; a[1][1] =  -1.9217194; a[1][2] =  0.9289864;
G[2] = 0.0054630;
b[2][0] = 1.0000000; b[2][1] = 1.9994122; b[2][2]= 0.9994131;
a[2][0] = 1.0000000; a[2][1] =  -1.9562202; a[2][2] =  0.9723269;


//  **** Stop copying MATLAB code here  ****



  //  Iterate over each second order stage.  For each stage shift the input data
  //  buffer ( x[kk] ) by one and the output data buffer by one ( y[k] ).  Then bring in 
  //  a new sample xv into the buffer;
  //
  //  Then execute the recusive filter on the buffer
  //
  //  y[k] = -a[2]*y[k-2] + -a[1]*y[k-1] + g*b[0]*x[k] + b[1]*x[k-1] + b[2]*x[k-2] 
  //
  //  Pass the output from this stage to the next stage by setting the input
  //  variable to the next stage x to the output of the current stage y
  //  
  //  Repeat this for each second order stage of the filter

  
  for (i =0; i<numStages; i++)
    {
      yM2[i] = yM1[i]; yM1[i] = yM0[i];  xM2[i] = xM1[i]; xM1[i] = xM0[i], xM0[i] = G[i]*xv;
      yv = -a[i][2]*yM2[i] - a[i][1]*yM1[i] + b[i][2]*xM2[i] + b[i][1]*xM1[i] + b[i][0]*xM0[i];
      yM0[i] = yv;
      xv = yv;
    }
//
//  execUsec += micros()-startTime;
  
  return yv;
}

//*******************************************************************************
float IIR_Middle(float xv)
{  


  //  ***  Copy variable declarations from MATLAB generator to here  ****

//Filter specific variable declarations
const int numStages = 5;
static float G[numStages];
static float b[numStages][3];
static float a[numStages][3];

//  *** Stop copying MATLAB variable declarations here
  
  int stage;
  int i;
  static float xM0[numStages] = {0.0}, xM1[numStages] = {0.0}, xM2[numStages] = {0.0};
  static float yM0[numStages] = {0.0}, yM1[numStages] = {0.0}, yM2[numStages] = {0.0};
  
  float yv = 0.0;
  unsigned long startTime;



//  ***  Copy variable initialization code from MATLAB generator to here  ****

//CHEBY bandpass, order 5, R= 0.5, [12 40] BPM
G[0] = 0.1005883;
b[0][0] = 1.0000000; b[0][1] = -0.0007646; b[0][2]= -0.9996017;
a[0][0] = 1.0000000; a[0][1] =  -1.7797494; a[0][2] =  0.8889605;
G[1] = 0.1005883;
b[1][0] = 1.0000000; b[1][1] = 2.0009417; b[1][2]= 1.0009420;
a[1][0] = 1.0000000; a[1][1] =  -1.8483239; a[1][2] =  0.8984289;
G[2] = 0.1005883;
b[2][0] = 1.0000000; b[2][1] = 1.9996397; b[2][2]= 0.9996400;
a[2][0] = 1.0000000; a[2][1] =  -1.9241024; a[2][2] =  0.9473605;
G[3] = 0.1005883;
b[3][0] = 1.0000000; b[3][1] = -1.9997055; b[3][2]= 0.9997056;
a[3][0] = 1.0000000; a[3][1] =  -1.7805039; a[3][2] =  0.9515924;
G[4] = 0.1005883;
b[4][0] = 1.0000000; b[4][1] = -2.0001113; b[4][2]= 1.0001113;
a[4][0] = 1.0000000; a[4][1] =  -1.9696093; a[4][2] =  0.9850367;

//  **** Stop copying MATLAB code here  ****
  
  for (i =0; i<numStages; i++)
    {
      yM2[i] = yM1[i]; yM1[i] = yM0[i];  xM2[i] = xM1[i]; xM1[i] = xM0[i], xM0[i] = G[i]*xv;
      yv = -a[i][2]*yM2[i] - a[i][1]*yM1[i] + b[i][2]*xM2[i] + b[i][1]*xM1[i] + b[i][0]*xM0[i];
      yM0[i] = yv;
      xv = yv;
    }
//
//  execUsec += micros()-startTime;
  
  return yv;
}

//*******************************************************************************
float IIR_High(float xv)
{  

  //  ***  Copy variable declarations from MATLAB generator to here  ****

//Filter specific variable declarations
const int numStages = 3;
static float G[numStages];
static float b[numStages][3];
static float a[numStages][3];

//  *** Stop copying MATLAB variable declarations here
  
  int stage;
  int i;
  static float xM0[numStages] = {0.0}, xM1[numStages] = {0.0}, xM2[numStages] = {0.0};
  static float yM0[numStages] = {0.0}, yM1[numStages] = {0.0}, yM2[numStages] = {0.0};
  
  float yv = 0.0;
  unsigned long startTime;


//  ***  Copy variable initialization code from MATLAB generator to here  ****

//CHEBY high, order 5, R = 0.5, 40 BPM

G[0] = 0.7527548;
b[0][0] = 1.0000000; b[0][1] = -1.0012325; b[0][2]= 0.0000000;
a[0][0] = 1.0000000; a[0][1] =  -0.2605136; a[0][2] =  0.0000000;
G[1] = 0.7527548;
b[1][0] = 1.0000000; b[1][1] = -1.9980085; b[1][2]= 0.9980100;
a[1][0] = 1.0000000; a[1][1] =  -1.3350294; a[1][2] =  0.6145423;
G[2] = 0.7527548;
b[2][0] = 1.0000000; b[2][1] = -2.0007590; b[2][2]= 1.0007605;
a[2][0] = 1.0000000; a[2][1] =  -1.7555162; a[2][2] =  0.9156503;

//  **** Stop copying MATLAB code here  ****
  
  for (i =0; i<numStages; i++)
    {
      yM2[i] = yM1[i]; yM1[i] = yM0[i];  xM2[i] = xM1[i]; xM1[i] = xM0[i], xM0[i] = G[i]*xv;
      yv = -a[i][2]*yM2[i] - a[i][1]*yM1[i] + b[i][2]*xM2[i] + b[i][1]*xM1[i] + b[i][0]*xM0[i];
      yM0[i] = yv;
      xv = yv;
    }
//
//  execUsec += micros()-startTime;
  
  return yv;
}



//*******************************************************************
void getStats(float xv, stats_t &s, bool reset)
{
  float oldMean, oldVar;
  
  if (reset == true)
  {
    s.stdev = sqrt(s.var/s.tick);
    s.tick = 1;
    s.mean = xv;
    s.var = 0.0;  
  }
  else
  {
    oldMean = s.mean;
    s.mean = oldMean + (xv - oldMean)/(s.tick+1);
    oldVar = s.var; 
    s.var = oldVar + (xv - oldMean)*(xv - s.mean);      
  }
  s.tick++;  
}

//*******************************************************************
float analogReadDitherAve(void)
{ 
 
float sum = 0.0;
int index;
  for (int i = 0; i < NUM_SUBSAMPLES; i++)
  {
    index = i;
    digitalWrite(DAC0, (index & B00000001)); // LSB bit mask
    digitalWrite(DAC1, (index & B00000010));
    digitalWrite(DAC2, (index & B00000100)); // MSB bit mask
    sum += analogRead(LM61);
  }
  return sum/NUM_SUBSAMPLES; // averaged subsamples 

}

//*********************************************************************
void setAlarm(int aCode, boolean isToneEn)
{

// Your alarm code goes here

    
} // setBreathRateAlarm()

//*************************************************************
float testVector(void)
{
  // Variable rate sinusoidal input
  // Specify segment frequencies in bpm.
  // Test each frequency for nominally 60 seconds.
  // Adjust segment intervals for nearest integer cycle count.
    
  const int NUM_BAND = 6;
  const float CAL_FBPM = 10.0, CAL_AMP = 2.0; 
  
  const float FBPM[NUM_BAND] = {5.0, 10.0, 15.0, 20.0, 30.0, 70.0}; // LPF test
  static float bandAmp[NUM_BAND] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0};

  //  Determine the number of samples (around 600 ) that will give you an even number
  //  of full cycles of the sinewave.  This is done to avoid a large discontinuity 
  //  between bands.  This forces the sinewave in each band to end near a value of zer
  
  static int bandTick = int(int(FBPM[0]+0.5)*(600/FBPM[0]));
  static int simTick = 0, band = 0;
  static float Fc = FBPM[0]/600, cycleAmp = bandAmp[0];

  //for (int i = 0; i < NUM_BAND; i++) bandAmp[i] = CAL_AMP*(CAL_FBPM/FBPM[i]);  

  //  Check to see if the simulation tick has exceeded the number of tick in each band.
  //  If it has then switch to the next frequency (band) again computing how many
  //  ticks to go through to end up at the end of a cycle.
  
  if ((simTick >= bandTick) && (FBPM[band] > 0.0))
  {

    //  The simTick got to the end of the band cycle.  Go to the next frequency
    simTick = 0;
    band++;
    Fc = FBPM[band]/600.0;
    cycleAmp = bandAmp[band];
    bandTick = int(int(FBPM[band]+0.5)*(600/FBPM[band]));
  }
 
  float degC = 0.0; // DC offset
  degC += cycleAmp*sin(TWO_PI*Fc*simTick++);  
  //degC += 1.0*(tick/100.0); // drift: degC / 10sec
  //degC += 0.1*((random(0,101)-50.0)/29.0); // stdev scaled from 1.0
  return degC;
}

//*******************************************************************
void configureArduino(void)
{
  pinMode(DAC0,OUTPUT); digitalWrite(DAC0,LOW);
  pinMode(DAC1,OUTPUT); digitalWrite(DAC1,LOW);
  pinMode(DAC2,OUTPUT); digitalWrite(DAC2,LOW);

  pinMode(SPKR, OUTPUT); digitalWrite(SPKR,LOW);


  analogReference(INTERNAL); // DEFAULT, INTERNAL
  analogRead(LM61); // read and discard to prime ADC registers
  Serial.begin(115200); // 11 char/msec 
}


//**********************************************************************
void WriteToSerial( int numValues, float dataArray[] )
{

  int index=0; 
  for (index = 0; index < numValues; index++)
  {
    if (index >0)
    {
      Serial.print('\t');
    }
      Serial.print(dataArray[index], DEC);
  }

  Serial.print('\n');
  delay(20);

}  // end WriteToMATLAB

////**********************************************************************
float ReadFromMATLAB()
{
  int charCount;
  bool readComplete = false;
  char inputString[80], inChar;


  // Wait for the serial port

  readComplete = false;
  charCount = 0;
  while ( !readComplete )
  {
    while ( Serial.available() <= 0);
    inChar = Serial.read();

    if ( inChar == '\n' )
    {
      readComplete = true;
    }
    else
    {
      inputString[charCount++] = inChar;
    }
  }
  inputString[charCount] = 0;
  return atof(inputString);

} // end ReadFromMATLAB

//*******************************************************************
void syncSample(void)
{
  while (sampleFlag == false); // spin until ISR trigger
  sampleFlag = false;          // disarm flag: enforce dwell  
}

//**********************************************************************
void ISR_Sample()
{
  sampleFlag = true;
}