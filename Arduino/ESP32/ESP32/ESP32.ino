// Bibliothèques
#include <Wire.h>
#include <cstring>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include "DFRobot_CCS811.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

// Définition des pins
#define LED_PIN 4
#define LED_COUNT 8
#define PIN_DHT11 22
#define DHTTYPE DHT11
#define Fan 21
#define SDA_PIN 33
#define SCL_PIN 32
#define OLED_RESET 4
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define buttonPin 27
#define buzzerPin 14

// Initialisation des objets
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);
DHT dht(PIN_DHT11, DHTTYPE);
Adafruit_BMP280 bmp;
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
DFRobot_CCS811 CCS811;

// Variables globales
int ventilateur = 0;
int ventilateur_auto = 0;
int buttonState = 0;
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
const char* ssid = "Gauthier";
const char* password = "y3rfajkj";
const char* mqtt_server = "mqtt.ci-ciad.utbm.fr";

// Fonction de connexion WiFi
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

// Fonction de rappel MQTT
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
  if (strcmp(topic, "esp32/ventilateur") == 0) {
    if (messageTemp == "ON" && ventilateur_auto != 256) {
      Serial.println("bouton appuyé");
      ventilateur = (ventilateur == 4) ? 0 : ventilateur + 1;
    } 
  }
}

// Fonction de reconnexion MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("espclient")) {
      Serial.println("connected");
      client.subscribe("esp32/projet_qualitee_air");
      client.subscribe("esp32/ventilateur");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  // Initialisation série
  Serial.begin(115200);

  // Initialisation des actionneurs et capteurs
  strip.begin();
  strip.show();
  strip.setBrightness(50);
  pinMode(Fan, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  Wire.begin(SDA_PIN, SCL_PIN);
  pinMode(buzzerPin, OUTPUT);

  // Initialisation des capteurs
  if (!bmp.begin(0x76)) {
    Serial.println(F("Could not find BMP280 sensor. Check wiring."));
    while (1);
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X2, Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::FILTER_X16, Adafruit_BMP280::STANDBY_MS_500);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Could not find OLED display. Check wiring."));
    while (1);
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  dht.begin();

  while (CCS811.begin() != 0) {
    Serial.println("Failed to init CCS811 chip. Please check the connection.");
    delay(1000);
  }

  // Initialisation WiFi et MQTT
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  // Lecture des capteurs
  float humidity = dht.readHumidity();
  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure() / 100.0F; // conversion en hPa
  float altitude = bmp.readAltitude(1013.25); // pression au niveau de la mer en hPa

  if (isnan(humidity) || isnan(temperature) || isnan(pressure) || isnan(altitude)) {
    Serial.println("Echec de reception d'un ou des capteurs");
    return;
  }

  // Affichage sur l'écran OLED
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

  if (CCS811.checkDataReady() == true) {
    display.print("CO2: ");
    display.print(CCS811.getCO2PPM());
    display.print("ppm, TVOC: ");
    display.print(CCS811.getTVOCPPB());
    display.println("ppb");

    if (CCS811.getCO2PPM() > 1000) {
      analogWrite(buzzerPin, 200); // Activation du buzzer
      delay(50);
      analogWrite(buzzerPin, 0); // Désactivation du buzzer
      Serial.print("buzzer");
      ventilateur_auto = 256;
    }
    else{
      ventilateur_auto = 0; 
    }

    Serial.print("Humidité : ");
    Serial.println(humidity);
    Serial.print("CO2: ");
    Serial.print(CCS811.getCO2PPM());
    Serial.print("ppm, TVOC: ");
    Serial.print(CCS811.getTVOCPPB());
    Serial.println("ppb");
  }

  display.display();

  CCS811.writeBaseLine(0x447B);

  // Gestion du ventilateur et des LED
  switch (ventilateur) {
    case 0:
      strip.clear();
      strip.show();
      break;
    case 1:
      strip.setPixelColor(0, strip.Color(50, 50, 50));
      strip.setPixelColor(1, strip.Color(50, 50, 50));
      strip.show();
      break;
    case 2:
      strip.setPixelColor(2, strip.Color(50, 50, 50));
      strip.setPixelColor(3, strip.Color(50, 50, 50));
      strip.show();
      break;
    case 3:
      strip.setPixelColor(4, strip.Color(50, 50, 50));
      strip.setPixelColor(5, strip.Color(50, 50, 50));
      strip.show();
      break;
    case 4:
      strip.setPixelColor(6, strip.Color(50, 50, 50));
      strip.setPixelColor(7, strip.Color(50, 50, 50));
      strip.show();
      break;
  }

  analogWrite(Fan, ventilateur * 64);
  buttonState = digitalRead(buttonPin);
  if (ventilateur_auto == 256){
    analogWrite(Fan, ventilateur_auto);
  }
  if (buttonState == LOW && ventilateur_auto != 256) {
    ventilateur = (ventilateur == 4) ? 0 : ventilateur + 1;
  }

  // Gestion du MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    // Convertir les valeurs float en chaînes de caractères
    char tempString[8];
    char presString[8];
    char altitudeString[8];
    char humidityString[8];
    char CO2String[8];
    char TVOCPPBString[8];

    dtostrf(pressure, 1, 2, presString);
    dtostrf(temperature, 1, 2, tempString);
    dtostrf(altitude, 1, 2, altitudeString);
    dtostrf(humidity, 1, 2, humidityString);
    sprintf(CO2String, "%u", CCS811.getCO2PPM()); // Utilisation de %u pour un uint16_t
    sprintf(TVOCPPBString, "%u", CCS811.getTVOCPPB()); // Utilisation de %u pour un uint16_t

    char result[55]; // Taille de la chaîne de résultat (5 valeurs * 8 caractères chacune + 4 tirets + 1 pour le caractère de fin de chaîne)
    snprintf(result, sizeof(result), "%.2f-%s-%.2f-%.2f-%s-%s", temperature, presString, altitude, humidity, CO2String, TVOCPPBString);

    // Publication des données sur le topic
    client.publish("esp32/projet_qualitee_air", result);
  }
  delay(200);
}
