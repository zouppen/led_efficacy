// LED efficacy measurement

// Configurable parameters
const int luxRange = 2; // Choose correct in range of 0..3
const float highVoltage = 2.65; // Must be lower than the rating of your capacitor
const int interval = 100; // Interval between measurements in milliseconds
const float margin = 0.15; // Avoid problems with hysteresis by discharge this much before starting to measurement

// Some precalculated values
const float luxCoeff = (float)(125 << (luxRange << 1))/4096;
const float voltCoeff = 2.44140625e-3; // with 5V supply voltage and two measurements (5/1024/2)
const int stopCharge = highVoltage/voltCoeff;
const int startMeasure = (highVoltage-margin)/voltCoeff;
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
  // Turn on PSU and turn off light
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);
  
  // Initialize serial port
  Serial.begin(9600);
  
  // Configure ISL29023 
  byte init[] = {0,160,luxRange};
  Wire.begin();
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
  int volts = analogRead(A0) + analogRead(A0);

  // Clean line
  Serial.print("\r\x1B[K");

  // Check if saturated
  if (lux == 0xffff) {
    Serial.print("SATURATION DETECTED! Raise lux range.\r\n");
  }

  // If not yet charged.
  if (!gotHighVoltage) {
    if (volts >= stopCharge) {
      gotHighVoltage=true;
      digitalWrite(4,LOW); // Disconnect PSU, turn on the LED
    }
    Serial.print("charging");
    goto out;
  }
  
  // If charged but not yet started.
  if (start==0) {
    if (volts <= startMeasure) {
      start = millis();
      cumulativeLux = lux;
      measurements = 1;
    }
    Serial.print("preparing");
    goto out;
  }
  
  // If charged but bright enough.
  if (lux > darkness) {
    cumulativeLux += lux;
    measurements++;
    Serial.print("discharging");
    goto out;
  }
  
  // Energy has run out.
  Serial.print("energy,measurements: ");
  Serial.print(cumulativeLux);
  Serial.print(',');
  Serial.print(measurements);
  Serial.print("\r\n");
  start = 0;
  gotHighVoltage=false;
  digitalWrite(4,HIGH); // Recharge
  Serial.print("finalizing");
  
out: 

  Serial.print(", ");
  Serial.print(lux*luxCoeff);
  Serial.print(" lx, ");
  Serial.print(volts*voltCoeff);
  Serial.print(" V");
  if (start) {
    Serial.print(", ");
    Serial.print((float)(now-start)/1000);
    Serial.print(" s");
  }
}

