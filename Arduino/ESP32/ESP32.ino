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
#define buzzerPin 34

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);
DHT dht(PIN_DHT11, DHTTYPE);
Adafruit_BMP280 bmp;
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
DFRobot_CCS811 CCS811;

const int MAX = 120;
int tvocArray[MAX];
int co2Array[MAX];
int tempArray[MAX]; 
int humArray[MAX];
int ventilateur = 0;
int ecran = 0;
int buttonState = HIGH;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 25;

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
  if (strcmp(topic, "esp32/projet_qualitee_air/Z/ecran") == 0) {
    ecran = atoi(messageTemp.c_str());
    Serial.println("ecran");  
    
  }
  if (strcmp(topic, "esp32/projet_qualitee_air/Z/ventilateur") == 0) {
    if (messageTemp == "ON") {
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
      client.subscribe("esp32/projet_qualitee_air/Z");
      client.subscribe("esp32/projet_qualitee_air/Z/ventilateur");
      client.subscribe("esp32/projet_qualitee_air/Z/ecran");
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
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  float pressure = bmp.readPressure() / 100.0F;
  float altitude = bmp.readAltitude(1013.25);
  int CO2 = 0; 
  int TVOC = 0;
  if (CCS811.checkDataReady() == true) {
    CO2 = CCS811.getCO2PPM(); 
    TVOC = CCS811.getTVOCPPB(); 
  }
  
  storeTemp();

  if (isnan(humidity) || isnan(temperature) || isnan(pressure) || isnan(altitude)) {
    Serial.println("Echec de reception d'un ou des capteurs");
    return;
  }

  // Affichage sur l'écran OLED 
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0); 
  switch (ecran) {
    case 0: 
      display.print("Temperature : ");
      display.print(temperature);
      display.println("*C");
      drawTempGraph();
      break; 
    case 1: 
      display.print("  Pression: ");
      display.print(pressure);
      display.println("hPa");
      break; 
    case 2: 
      display.print("  Altitude: ");
      display.print(altitude);
      display.println("m");
      break; 
    case 3: 
      display.print("  Humiditee : ");
      display.print(humidity);
      display.println("%");
      drawHumGraph();
      break;
    case 4: 
      display.print("  CO2: ");
      display.print(CO2);
      display.println("ppm");
      drawCo2Graph();
      break; 
    case 5: 
      display.print("    TVOC: ");
      display.print(TVOC);
      display.println("ppb");
      drawTvocGraph();
      break; 
  }
  display.display();
  CCS811.writeBaseLine(0x447B);



  if (CO2 > 1000) {
    digitalWrite(buzzerPin, HIGH);
    analogWrite(Fan, 256);
  } else {
    digitalWrite(buzzerPin, LOW);
    analogWrite(Fan, ventilateur * 64);
  }

  
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        ventilateur = (ventilateur == 4) ? 0 : ventilateur + 1;
      }
    }
  }
  lastButtonState = reading;

  switch (ventilateur) {
    case 0:
      strip.clear();
      strip.show();
      break;
    case 1:
      strip.setPixelColor(7, strip.Color(50, 50, 50));
      strip.setPixelColor(6, strip.Color(50, 50, 50));
      strip.show();
      break;
    case 2:
      strip.setPixelColor(5, strip.Color(50, 50, 50));
      strip.setPixelColor(4, strip.Color(50, 50, 50));
      strip.show();
      break;
    case 3:
      strip.setPixelColor(3, strip.Color(50, 50, 50));
      strip.setPixelColor(2, strip.Color(50, 50, 50));
      strip.show();
      break;
    case 4:
      strip.setPixelColor(1, strip.Color(50, 50, 50));
      strip.setPixelColor(0, strip.Color(50, 50, 50));
      strip.show();
      break;
  }



  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 60000) {
    lastMsg = now;
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
    sprintf(CO2String, "%u", CO2);
    sprintf(TVOCPPBString, "%u", TVOC);
    char result[55]; // Taille de la chaîne de résultat (5 valeurs * 8 caractères chacune + 4 tirets + 1 pour le caractère de fin de chaîne)
    snprintf(result, sizeof(result), "%.2f-%s-%.2f-%.2f-%s-%s", temperature, presString, altitude, humidity, CO2String, TVOCPPBString);
    client.publish("esp32/projet_qualitee_air/Z", result);
  }
  delay(200);
}
void storeTemp()
{
  int temp = dht.readTemperature();
  int hum = dht.readHumidity(); 
  int co2 = CCS811.getCO2PPM(); 
  int tvoc = CCS811.getTVOCPPB();
  static int i = 0;
  if ( isnan( ( int ) temp ) )
    if ( i < MAX )
    {
      tempArray[ i ] = 0;
      humArray[ i ] = 0;
      i++;
    }
    else
    {
      for ( int j = 0; j < MAX - 1; j++ )
      {
        tempArray[ j ] = tempArray[ j + 1 ];
        tempArray[ MAX - 1 ] = 0;
        co2Array[j] = co2Array[j + 1];
        co2Array[ MAX - 1 ] = 0;
        tvocArray[j] = tvocArray[j + 1];
        tvocArray[ MAX - 1 ] = 0;
        humArray[ j ] = humArray[ j + 1 ];
        humArray[ MAX - 1 ] = 0; 
      }
    }
  else
  {
    if ( i < MAX )
    {
      tempArray[ i ] = temp;
      co2Array[ i ] = co2;
      humArray[ i ] = hum;
      tvocArray[ i ] = tvoc;
      i++;
    }
    else
    {
      for ( int j = 0; j < MAX - 1; j++ )
      {
        tempArray[ j ] = tempArray[ j + 1 ];
        tempArray[ MAX - 1 ] = temp;
        co2Array[j] = co2Array[j + 1];
        co2Array[ MAX - 1 ] = co2;
        tvocArray[j] = tvocArray[j + 1];
        tvocArray[ MAX - 1 ] = tvoc;
        humArray[ j ] = humArray[ j + 1 ];
        humArray[ MAX - 1 ] = hum; 
      }
    }
  }
}

void drawGraph()
{
  display.drawPixel( 6, 13, WHITE ); 
  display.drawPixel( 6, 23, WHITE ); 
  display.drawPixel( 6, 33, WHITE ); 
  display.drawPixel( 6, 43, WHITE ); 
  display.drawPixel( 6, 53, WHITE ); 
  display.drawPixel( 27, 62, WHITE ); 
  display.drawPixel( 47, 62, WHITE ); 
  display.drawPixel( 67, 62, WHITE ); 
  display.drawPixel( 87, 62, WHITE ); 
  display.drawPixel( 107, 62, WHITE ); 
  display.drawFastVLine( 7, 15, 100, WHITE );
  display.drawFastHLine( 7, 63, 120, WHITE );
}
void drawTempGraph()
{
  drawGraph();
  for (int i = 0; i < MAX; i++ )
    display.drawFastHLine( 135 - MAX * 2 + i * 2, 64 - tempArray[ i ], 2, WHITE  ); 
}
void drawHumGraph()
{
  drawGraph();
  for (int i = 0; i < MAX; i++ )
    display.drawFastHLine( 135 - MAX + i * 2, 64 - humArray[ i ] /2, 2,  WHITE  ); 

}
void drawCo2Graph()
{
  drawGraph();
  for (int i = 0; i < MAX; i++ )
    display.drawFastHLine( 135 - MAX + i * 2, 64 - co2Array[ i ] /15 , 2,  WHITE  ); 

}
void drawTvocGraph()
{
  drawGraph();
  for (int i = 0; i < MAX; i++ )
    display.drawFastHLine( 135 - MAX + i * 2, 64 - tvocArray[ i ]*2, 2,  WHITE  ); 

}

