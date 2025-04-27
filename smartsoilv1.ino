#include <LedControl.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// A SmartSoil V1 valós idejű környezeti megfigyelőrendszert mutat be, 
// amely beltéri növények állapotának nyomon követésére és a levegő minőségének figyelemmel kísérésére szolgál. 
// A projektet Štefan Müller készítette a Robotika (KINF/AIdb/ROB/22) tantárgy szemesztrális munka részeként.
// Štefan Müller, UJS - GIK, Alkalmazott Informatika. 2025/4/27

//  MAX7219 DOT LED 8X8 MATRIX definíció
LedControl lc = LedControl(11, 13, 10, 1); // DIN, CLK, CS

// BME280 definíció
#define BME280_I2C_ADDRESS 0x76 // Try 0x77 if 0x76 doesn't work
Adafruit_BME280 bme;
#define SEA_LEVEL_PRESSURE_HPA 1014.0

// Gáz-szenzor pin definíciók
#define SOIL_PIN   A0
#define MQ135_PIN  A1
#define MQ9_PIN    A2

float R0mq135 = 5.9; // Kalibrált R0 érték - MQ135
float R0mq9 = 3.8; // Kalibrált R0 érték - MQ9

const int dryValue = 980; // Kalibrált száraz-érték
const int wetValue = 320; // Kalibrált nedves-érték

//  MAX7219 DOT LED 8X8 MATRIX hangulatjelek
byte happyface[8] = {
  B00111100, B01000010, B10100101, B10000001,
  B10100101, B10011001, B01000010, B00111100
};

byte sadface[8] = {
  B00111100, B01000010, B10100101, B10000001,
  B10011001, B10100101, B01000010, B00111100
};

byte seriousface[8] = {
  B00111100, B01000010, B10100101, B10000001,
  B10111101, B10000001, B01000010, B00111100
};

byte alertsign[8] = {
  B00111100, B00111100, B00111100, B00111100,
  B00111100, B00000000, B00111100, B00111100
};

byte welcometext[8] = {
  B00000000, B01001010, B01001000, B01111010,
  B01001010, B01001010, B00000000, B00000000
};

// Hangulatjelek felrajzolása a MAX7219 DOT LED 8X8 MATRIX kijelzőre
void showFace(byte face[8]) {
  for (int row = 0; row < 8; row++) {
    lc.setRow(0, row, face[row]);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // BME280 komponens inicializálása - ha nem található a cím, újra kell indítani a rendszert
  if (!bme.begin(BME280_I2C_ADDRESS)) {
    Serial.println("BME280 not found! Please check the connections.");
    showFace(alertsign);
    while (1);
  }

  Serial.println("SmartSoil Components Initialized!");

  // MAX7219 DOT LED 8X8 MATRIX kijelző iniciálizálása
  lc.shutdown(0, false);
  lc.setIntensity(0, 5);
  lc.clearDisplay(0);
  showFace(welcometext);
  delay(2000);
}

void loop() {
  // BME280 Szenzor adatainak beolvasása
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;
  float altitude = bme.readAltitude(SEA_LEVEL_PRESSURE_HPA);

  // Értékek beolvasása a táptalaj-nedvesség szenzorból
  int soilRaw = analogRead(SOIL_PIN);
  float soilPercent = map(soilRaw, dryValue, wetValue, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  // Értékek beolvasása a gáz-szenzorokból
  int mq135Value = analogRead(MQ135_PIN);
  int mq9Value = analogRead(MQ9_PIN);

  float voltage135 = mq135Value * (5.0 / 1023.0); 
  float Rs135 = (5.0 - voltage135) / voltage135;

  float voltage9 = mq9Value * (5.0 / 1023.0); 
  float Rs9 = (5.0 - voltage9) / voltage9;

  float ratiomq135 = Rs135 / R0mq135;
  float ratiomq9 = Rs9 / R0mq9;

  // Formula MQ-135 Air Quality (levegő minőség és füst) PPM detektálására
  float ppm_mq135_air = pow(10, (log10(ratiomq135) - 0.68) / -0.42); //AQ
  float ppm_mq135_smoke = pow(10, (log10(ratiomq135) - 0.74) / -0.42); // Smoke

  // Formula MQ-9 CO, NH3 (szén-monoxid és metán) PPM detektálására
  float ppm_mq9_co = pow(10, (log10(ratiomq9) - 0.35) / -0.48);
  float ppm_mq9_ch4 = pow(10, (log10(ratiomq9) - 0.38) / -0.38);    // CH4 (Methane)

  // SERIAL - KONZOL KIMENETEK
  Serial.println("--- SmartSoil Readings ---");
  Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");
  Serial.print("Pressure: "); Serial.print(pressure); Serial.println(" hPa");
  Serial.print("Altitude: "); Serial.print(altitude); Serial.println(" m");
  Serial.print("Soil Moisture: "); Serial.print(soilRaw); Serial.print(" | "); Serial.print(soilPercent); Serial.println(" %");

  // MQ-135 SZENZOR
  Serial.println("--- MQ-135 Readings ---");
  Serial.print("Raw ADC Value: "); Serial.println(mq135Value);
  Serial.print("RS Value: "); Serial.println(Rs135);
  Serial.print("Air Quality (ppm): "); Serial.println(ppm_mq135_air);
  Serial.print("Smoke (ppm): "); Serial.println(ppm_mq135_smoke);

  // MQ-9 SZENZOR
  Serial.println("--- MQ-9 Readings ---");
  Serial.print("Raw ADC Value: "); Serial.println(mq9Value);
  Serial.print("RS Value: "); Serial.println(Rs9);
  Serial.print("CO (ppm): "); Serial.println(ppm_mq9_co);
  Serial.print("Methane (CH4) (ppm): "); Serial.println(ppm_mq9_ch4);
  Serial.println("-----------------------------");


  // Mérések kimutatása a MAX7219 DOT LED 8X8 MATRIX kijelzőn

  // GÁZOK ÉS LEVEGŐ MINŐSÉG SZENZOROK
  if (ppm_mq9_co > 50) {
      showFace(alertsign); // Kritikus szén-monoxid szint - nagyobb mint 50ppm
  } 
  else if (ppm_mq9_co > 30 || ppm_mq9_ch4 > 500 || ppm_mq135_smoke > 150 || ppm_mq135_air > 100) {
      showFace(sadface); // Nem kedvező levegő minőség - magas szint
  }
  else if (ppm_mq9_co > 10 || ppm_mq9_ch4 > 200 || ppm_mq135_smoke > 80 || ppm_mq135_air > 50) {
      showFace(seriousface); // Nem kedvező levegő minőség - közepes szint
  }

  // TÁPTALAJ NEDVESSÉG SZENZOR
  else if (soilPercent < 20) {
      showFace(sadface); // Kiszáradt táptalaj
  }
  else if (soilPercent > 90) {
      showFace(alertsign); // Túl-öntözött táptalaj
  }

  // HŐMÉRSÉKLET ÉS PÁRATARTALOM SZENZOR
  else if (temperature > 40 || humidity > 80) {
      showFace(alertsign); // Kritikus hőmérséklet vagy kritikus páratartalom
  }
  else if (temperature > 30 || humidity > 49) {
      showFace(sadface); // Túl meleg vagy túl párás a levegő
  }
  else if (temperature < 22.2 || humidity < 10) {
      showFace(seriousface); // Hideg vagy túl száraz a levegő
  }

  // OPTIMÁLIS ÉRTÉKEK
  else {
      showFace(happyface); // Optimális értékekek
  }


  delay(2000); // 2 másodpercenként frissít
}
