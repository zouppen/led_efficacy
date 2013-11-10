// LED efficacy measurement

// Configurable parameters
const int luxRange = 2; // Choose correct in range of 0..3
const float highVoltage = 2.6; // Must be lower than the rating of your capacitor
const int interval = 250; // Interval between measurements in milliseconds

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
  // Constant intervals
  unsigned long now = millis();
  if (targetTime > now) {
    delay(targetTime-now);
  } else {
    Serial.print("TIMING ERROR! Check I2C connection.\r\n");
  }
  targetTime += interval;

  word lux = getLux();
  int volts = analogRead(A0);

  // Clean line
  Serial.print("\r\x1B[K");

  // Check if saturated
  if (lux == 0xffff) {
    Serial.print("SATURATION DETECTED! Raise lux range.\r\n");
  }

  // If not yet charged.
  if (!gotHighVoltage) {
    if (volts >= stopCharge) {
      Serial.print("Fully charged.\r\n");
      gotHighVoltage=true;
    }
    goto out;
  }
  
  // If charged but not yet started.
  if (start==0) {
    if (volts <= startMeasure) {
      Serial.print("Measurement started.\r\n");
      start = millis();
      cumulativeLux = lux;
      measurements = 1;
    }
    goto out;
  }
  
  // If charged but bright enough.
  if (lux > darkness) {
    cumulativeLux += lux;
    measurements++;
    goto out;
  }
  
  // Energy has run out.
  Serial.print("Measurement ready.\r\n");
  Serial.print("cumulative,measurements,duration: ");
  Serial.print(cumulativeLux);
  Serial.print(',');
  Serial.print(measurements);
  Serial.print(',');
  Serial.print(now-start);
  Serial.print("\r\n");
  start = 0;
  gotHighVoltage=false;
  
out: 

  Serial.print(lux*luxCoeff);
  Serial.print(" lx, ");
  Serial.print(volts*voltCoeff);
  Serial.print(" V");    
}

