// LED efficacy measurement

// Configurable parameters
const int luxRange = 2; // Choose correct in range of 0..3
const float highVoltage = 2.6; // Must be lower than the rating of your capacitor

// Some precalculated values
const float luxCoeff = (float)(125 << (luxRange << 1))/4096;
const float voltCoeff = 4.8828125e-3; // with 5V supply voltage
const int stopCharge = highVoltage/voltCoeff;
const int startMeasure = (highVoltage-0.1)/voltCoeff; // Little smaller to avoid problems with hysteresis 
const int darkness = 5; // Any small number between 2..100 will probably do

#include <Wire.h>

word getLux() {
  Wire.beginTransmission(0x44);
  Wire.write((byte)2);
  Wire.endTransmission();
  Wire.requestFrom(0x44,2);
  byte l = Wire.read();
  byte h = Wire.read();
  return word(h,l);
}

bool gotHighVoltage = false;
unsigned long start = 0;
unsigned long cumulativeLux;
int measurements;
unsigned long targetTime = 0;

void setup()
{
  byte init[] = {0,160,luxRange};
  Wire.begin(); // join i2c bus (address optional for master)
  Serial.begin(9600);
  
  // Configure ISL29023  
  Wire.beginTransmission(0x44);
  Wire.write(init,sizeof(init));
  Wire.endTransmission();
}

void loop()
{
  unsigned long now = millis();
  long sleep = targetTime-now;
  if (sleep > 0) {
    Serial.print("sleeping ");
    Serial.println(sleep);
    delay(sleep);
  } else {
    Serial.println("TIMING ERROR");
  }
  targetTime += 1000;

  word lux = getLux();
  int volts = analogRead(A0);

  Serial.print(lux*luxCoeff);
  Serial.print(" lx, ");
  Serial.print(volts*voltCoeff);
  Serial.print(" V    \r");

  // If not yet charged.
  if (!gotHighVoltage) {
    if (volts >= stopCharge) {
      Serial.println("Fully charged.");
      gotHighVoltage=true;
    }
    return;
  }
  
  // If charged but not yet started.
  if (start==0) {
    if (volts <= startMeasure) {
      Serial.println("Measurement started.");
      start = millis();
      cumulativeLux = lux;
      measurements = 1;
    }
    return;
  }
  
  // If charged but bright enough.
  if (lux > darkness) {
    cumulativeLux += lux;
    measurements++;
    return;
  }
  
  // Energy has run out.
  Serial.println("Measurement ready.");
  Serial.print("cumulative,measurements,duration: ");
  Serial.print(cumulativeLux);
  Serial.print(',');
  Serial.print(measurements);
  Serial.print(',');
  Serial.println(now-start);
  start = 0;
  gotHighVoltage=false;
 }

