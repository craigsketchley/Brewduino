#include <OneWire.h>
#include <LiquidCrystal.h>

#define ONE_MIN 60000
#define SENSOR_PIN 44
#define COMPRESSOR_PIN 22
#define HEATER_PIN 23
#define FAN_PIN 24

#define COMPRESSOR_MIN_ON_TIME 300000 // 5 mins
#define HEATER_MIN_ON_TIME 300000 // 5 mins

byte BREW_TEMP_ADDR[8] = {0x28, 0xE1, 0x65, 0x67, 0x05, 0x00, 0x00, 0xE0};
byte FRID_TEMP_ADDR[8] = {0x28, 0x5A, 0xCC, 0x44, 0x05, 0x00, 0x00, 0x27};
byte OUTS_TEMP_ADDR[8] = {0x28, 0x37, 0x06, 0x68, 0x05, 0x00, 0x00, 0x53};

OneWire  ds(SENSOR_PIN);

LiquidCrystal lcd(35, 34, 33, 32, 31, 30);

bool compressorOn = false;
bool heaterOn = false;

// TODO: Maybe use to monitor on times. Maybe use timer/counter??
unsigned long compressorOnTime;
unsigned long heaterOnTime;

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 4);
  pinMode(COMPRESSOR_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  
  turnOffAll();
}

void loop() {
  // Get temperatures
  float brewTemp = getTemp(BREW_TEMP_ADDR);
  float fridTemp = getTemp(FRID_TEMP_ADDR);
  float outsTemp = getTemp(OUTS_TEMP_ADDR);

  // Display temperatures on LCD
  displayTemps(brewTemp, fridTemp, outsTemp);
  
  if (brewTemp > 28) {
    turnOnCompressor();
    delay(COMPRESSOR_MIN_ON_TIME);
  } else if (brewTemp < 15) {
    turnOnHeater();
    delay(HEATER_MIN_ON_TIME);
  } else {
    turnOffAll();
  }  
}

void turnOnCompressor() {
    digitalWrite(COMPRESSOR_PIN, HIGH);
    digitalWrite(HEATER_PIN, LOW);
    digitalWrite(FAN_PIN, LOW);
    compressorOn = true;
    heaterOn = false;
}

void turnOnHeater() {
    digitalWrite(COMPRESSOR_PIN, LOW);
    digitalWrite(HEATER_PIN, HIGH);
    digitalWrite(FAN_PIN, HIGH);
    compressorOn = false;
    heaterOn = true;
}

void turnOffAll() {
    digitalWrite(COMPRESSOR_PIN, LOW);
    digitalWrite(HEATER_PIN, LOW);
    digitalWrite(FAN_PIN, LOW);
    compressorOn = false;
    heaterOn = false;
}

void serialPrintTemps(float brewTemp, float fridTemp, float outsTemp) {
  Serial.print("Brew Temp = ");
  Serial.print(brewTemp);
  Serial.print((char)223);
  Serial.println("C");
  Serial.print("Fridge Temp = ");
  Serial.print(fridTemp);
  Serial.print((char)223);
  Serial.println("C");
  Serial.print("Outside Temp = ");
  Serial.print(outsTemp);
  Serial.print((char)223);
  Serial.println("C");
}

void displayTemps(float brewTemp, float fridTemp, float outsTemp) {
  lcd.clear();
  lcd.setCursor(0,0);
  displayTemp("Brew", brewTemp);
  lcd.setCursor(0,1);
  displayTemp("Fridge", fridTemp);
  lcd.setCursor(0,2);
  displayTemp("Outside", outsTemp);
}

void displayTemp(String tempName, float temp) {
  lcd.print(tempName);
  lcd.print(": ");
  lcd.print(temp);
  lcd.print((char)223);
  lcd.print("C");
}

float getTemp(byte* addr) {
  byte i;
  byte present = 0;
  byte type_s = 0;
  byte data[12];
  float celsius;
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
}
