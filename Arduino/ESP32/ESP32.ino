#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "DFRobot_CCS811.h"
#include <string>

// WiFi and MQTT Settings
const char* ssid = "Gauthier";
const char* password = "y3rfajkj";
const char* mqtt_server = "mqtt.ci-ciad.utbm.fr";
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;

// BMP280 Sensor
Adafruit_BMP280 bmp;
#define SDA_PIN 33
#define SCL_PIN 32

// OLED Display
#define OLED_RESET 4
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

// Moisture Sensor
#define DHpin 22 
byte dat[5];

// Air Quality Sensor
DFRobot_CCS811 CCS811;

// Function to read data from the moisture sensor
byte read_data() {
  byte data = 0;
  for (int i = 0; i < 8; i++) {
    if (digitalRead(DHpin) == LOW) {
      while (digitalRead(DHpin) == LOW);
      delayMicroseconds(30);
      if (digitalRead(DHpin) == HIGH) {
        data |= (1 << (7 - i));
      }
      while (digitalRead(DHpin) == HIGH);
    }
  }
  return data;
}

// Function to initialize the moisture sensor
void start_test() {
  digitalWrite(DHpin, LOW);
  delay(30);
  digitalWrite(DHpin, HIGH);
  delayMicroseconds(40);
  pinMode(DHpin, INPUT);
  while (digitalRead(DHpin) == HIGH);
  delayMicroseconds(80);
  if (digitalRead(DHpin) == LOW);
  delayMicroseconds(80);
  for (int i = 0; i < 4; i++) {
    dat[i] = read_data();
    pinMode(DHpin, OUTPUT);
    digitalWrite(DHpin, HIGH);
  }
}

// Function to connect to WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// MQTT callback function
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}

// Function to reconnect to MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("espclient")) {
      Serial.println("connected");
      client.subscribe("esp32/projet_qualitee_air");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  // Serial communication
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  // Moisture sensor
  pinMode(DHpin, OUTPUT);

  // BMP280 sensor
  if (!bmp.begin(0x76)) {
    Serial.println(F("Could not find BMP280 sensor. Check wiring."));
    while (1);
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);

  // OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Could not find OLED display. Check wiring."));
    while (1);
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  // CCS811 sensor
  while (CCS811.begin() != 0) {
    Serial.println("Failed to init CCS811 chip. Please check the connection.");
    delay(1000);
  }

  // WiFi and MQTT
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  start_test();

  // Read BMP280 sensor data
  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure() / 100.0F; // convert to hPa
  float altitude = bmp.readAltitude(1013.25); // sea level pressure in hPa

  // Display data on OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print(F("Temperature: "));
  display.print(temperature);
  display.println(" *C");
  display.print(F("Pressure: "));
  display.print(pressure);
  display.println(" hPa");
  display.print(F("Altitude: "));
  display.print(altitude);
  display.println(" m");
  display.print("Current humidity =");
  display.print(dat[0], DEC);
  display.print('.');
  display.print(dat[1], DEC);
  display.println('%');
  display.print("Current temperature =");
  display.print(dat[2], DEC);
  display.print('.');
  display.print(dat[3], DEC);
  display.println('C');

  if (CCS811.checkDataReady() == true) {
    display.print("CO2: ");
    display.print(CCS811.getCO2PPM());
    display.print("ppm, TVOC: ");
    display.print(CCS811.getTVOCPPB());
    display.println("ppb");
  }
  display.display();

  // Print data to Serial Monitor
  Serial.print(F("Temperature = "));
  Serial.print(temperature);
  Serial.println(" *C");
  Serial.print(F("Pressure = "));
  Serial.print(pressure);
  Serial.println(" hPa");
  Serial.print(F("Approx. Altitude = "));
  Serial.print(altitude);
  Serial.println(" m");
  Serial.print("Current humidity =");
  Serial.print(dat[0], DEC);
  Serial.print('.');
  Serial.print(dat[1], DEC);
  Serial.println('%');
  Serial.print("Current temperature =");
  Serial.print(dat[2], DEC);
  Serial.print('.');
  Serial.print(dat[3], DEC);
  Serial.println('C');

  if (CCS811.checkDataReady() == true) {
    Serial.print("CO2: ");
    Serial.print(CCS811.getCO2PPM());
    Serial.print("ppm, TVOC: ");
    Serial.print(CCS811.getTVOCPPB());
    Serial.println("ppb");
  }
  CCS811.writeBaseLine(0x447B);

  // MQTT handling
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();
  if (now - lastMsg > 6000) {
    lastMsg = now;
    client.publish("esp32/projet_qualitee_air", "caca");
  }

  // Delay before next loop iteration
  delay(2000);
}
