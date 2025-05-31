// Combined ESP32 code: Send to both ThingSpeak and Blynk
#define BLYNK_TEMPLATE_ID "TMPL6L7tOfI_y"
#define BLYNK_TEMPLATE_NAME "Small Environment Monitor"
#define BLYNK_AUTH_TOKEN "P3fnOizTWwVbSDIakhLFrFc-_4rpvPB0"

#include <Wire.h>
#include <SPI.h>
#include <U8g2lib.h>
#include "Adafruit_CCS811.h"
#include "DHT.h"
#include <WiFi.h>
#include <ThingSpeak.h>
#include <BlynkSimpleEsp32.h>

// === OLED (SPI) ===
U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R2, 5, 17, 16); // CS, DC, RESET

// === CCS811 (Air Quality) ===
Adafruit_CCS811 ccs;

// === DHT Sensor ===
#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// === Wi-Fi credentials ===
const char* ssid = "Mesh_Floor";
const char* password = "thaochi0299";

// === ThingSpeak credentials ===
unsigned long channelID = 2976712;
const char* apiKey = "WF3RG2UW5YNGPUTQ";

// === Variables ===
float temp = 0;
float humd = 0;
uint16_t co2 = 0;
uint16_t tvoc = 0;

WiFiClient client;
unsigned long lastThingSpeakUpdate = 0;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected.");

  dht.begin();
  ccs.begin();
  while (!ccs.available());

  u8g2.begin();

  ThingSpeak.begin(client);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);

  Serial.println("All sensors initialized");
}

void loop() {
  Blynk.run();

  // Read sensors
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temp = t;
  if (!isnan(h)) humd = h;

  if (ccs.available() && !ccs.readData()) {
    co2 = ccs.geteCO2();
    tvoc = ccs.getTVOC();
  }

  // Display on OLED
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.setCursor(0, 12); u8g2.print(" Temp: "); u8g2.print(temp); u8g2.print(" C");
  u8g2.setCursor(0, 26); u8g2.print(" Humd: "); u8g2.print(humd); u8g2.print(" %");
  u8g2.setCursor(0, 40); u8g2.print(" CO2: "); u8g2.print(co2); u8g2.print(" ppm");
  u8g2.setCursor(0, 54); u8g2.print(" VOC: "); u8g2.print(tvoc); u8g2.print(" ppb");
  u8g2.sendBuffer();

  // Send to Blynk every 1s (same as display)
  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, humd);
  Blynk.virtualWrite(V2, co2);
  Blynk.virtualWrite(V3, tvoc);

  // Send to ThingSpeak every 5s
  if (millis() - lastThingSpeakUpdate >= 5000) {
    lastThingSpeakUpdate = millis();
    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, humd);
    ThingSpeak.setField(3, co2);
    ThingSpeak.setField(4, tvoc);

    int httpCode = ThingSpeak.writeFields(channelID, apiKey);
    if (httpCode == 200) {
      Serial.println("Data successfully sent to ThingSpeak!");
    } else {
      Serial.print("Error sending to ThingSpeak: ");
      Serial.println(httpCode);
    }
  }

  delay(1000); // 1-second loop cycle for display and Blynk
}
