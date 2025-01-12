/**
 * This sketch uses an arduino Nano a TM1637 display and a DSM501a dust sensor to implement a crude Air quality monitor
 * It is based on https://www.makerguides.com/dust-sensor-dsm501a-with-arduino/ and implements sliding window to smooth out the output
 */

#include <TM1637Display.h>
TM1637Display display(5,6);

const uint8_t SEG_BOOT[] = {0x7C, 0x3F, 0x3F, 0x78};
const uint8_t SEG_BEST[] = {0x7C, 0x79, 0x6D, 0x78};
const uint8_t SEG_GOOD[] = {0x3D, 0x3F, 0x3F, 0x5E};
const uint8_t SEG_ACPT[] = {0x77, 0x39, 0x73, 0x78};
const uint8_t SEG_HEAV[] = {0x76, 0x79, 0x77, 0x3E};
const uint8_t SEG_HZRD[] = {0x76, 0x5B, 0x50, 0x5E};

const int pinPM25 = 8;
const int pinPM1 = 7;
const unsigned long sampleTime = 5000;  // mSec   -> 5..30 sec
const int numSamples = 12;                // Number of samples for 1 minute at 5 seconds

float pm25Samples[numSamples] = {0};
float pm1Samples[numSamples] = {0};
int sampleIndex = 0;
bool displayWhichOne = false;

float calc_low_ratio(float lowPulse) {
  return lowPulse / sampleTime * 100.0;  // low ratio in %
}

float calc_c_ugm3(float lowPulse) {
  float r = calc_low_ratio(lowPulse);
  float c_mgm3 = 0.00258425 * pow(r, 2) + 0.0858521 * r - 0.01345549;
  return max(0, c_mgm3) * 1000;
}

void setup() {
  Serial.begin(115200);
  display.setBrightness(0x0f);
  pinMode(pinPM25, INPUT);
  pinMode(pinPM1, INPUT);
  Serial.println("Warming up...");
  display.setSegments(SEG_BOOT);
  delay(60000);  // 1 minute warm-up
  
}

static unsigned long t_start = millis();
static float lowPM25 = 0.0;
static float lowPM1 = 0.0;

void loop() {
    
    // Read and accumulate pulse duration
    lowPM25 += pulseIn(pinPM25, LOW) / 1000.0; // Low pulse in seconds
    lowPM1 += pulseIn(pinPM1, LOW) / 1000.0;   // Low pulse in seconds

    if ((millis() - t_start) >= sampleTime) {

      // Calculate concentrations for PM2.5 and PM1
      float c_ugm3_pm25 = calc_c_ugm3(lowPM25);
      float c_ugm3_pm1 = calc_c_ugm3(lowPM1);
      
      // Display current values in the serial monitor
      Serial.println("Current Concentration PM2.5: " + String(c_ugm3_pm25) + " ug/m3");
      Serial.println("Current Concentration PM1: " + String(c_ugm3_pm1) + " ug/m3");
  
      // Store the latest concentrations in arrays for sliding window average
      pm25Samples[sampleIndex] = c_ugm3_pm25;
      pm1Samples[sampleIndex] = c_ugm3_pm1;
  
      sampleIndex = (sampleIndex + 1) % numSamples; // Circular buffer
  
      // Calculate averages for the last 5 minutes
      float avg_pm25 = 0;
      float avg_pm1 = 0;
  
      for (int i = 0; i < numSamples; i++) {
          avg_pm25 += pm25Samples[i];
          avg_pm1 += pm1Samples[i];
      }
  
      avg_pm25 /= numSamples; // Average PM2.5 concentration
      avg_pm1 /= numSamples;   // Average PM1 concentration
      Serial.println("AVG Concentration PM2.5: " + String(avg_pm25) + " ug/m3");
      Serial.println("AVG Concentration PM1: " + String(avg_pm1) + " ug/m3");
      display.clear();
      float avg_c_ugm3 = 999;
      if(displayWhichOne){
        avg_c_ugm3= avg_pm1;
        display.showNumberDecEx(1.0,0b01000000,false,2,0);
        displayWhichOne = !displayWhichOne;
      }else{
        avg_c_ugm3= avg_pm25;
        display.showNumberDecEx(2.5,0b01000000,false,2,0);
        displayWhichOne = !displayWhichOne;
      }
      delay(1000);
  
      // Update the display based on the 1-minute average concentration
      if (avg_c_ugm3 <= 25) {
          display.setSegments(SEG_BEST);
      } else if (avg_c_ugm3 > 25 && avg_c_ugm3 <= 50) {
          display.setSegments(SEG_GOOD);
      } else if (avg_c_ugm3 > 50 && avg_c_ugm3 <= 100) {
          display.setSegments(SEG_ACPT);
      } else if (avg_c_ugm3 > 100 && avg_c_ugm3 <= 300) {
          display.setSegments(SEG_HEAV);
      } else {
          display.setSegments(SEG_HZRD);
      }
      t_start = millis();
      lowPM25 = 0.0;
      lowPM1 = 0.0;
      
    }
}
