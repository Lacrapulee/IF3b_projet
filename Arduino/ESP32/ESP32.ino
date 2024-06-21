// Bibliotheque :
#include <Wire.h>
#include <cstring>
//capteurs 
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include "DFRobot_CCS811.h"
//actionneurs
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

//Mqtt
#include <WiFi.h>
#include <PubSubClient.h>

//led 
#define LED_PIN 4
#define LED_COUNT 8
// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

//bouton 
const int buttonPin = 27;
int buttonState = 0;

//ventilateur 
int ventilateur = 0; 

// WiFi and MQTT Settings
const char* ssid = "Gauthier";
const char* password = "y3rfajkj";
const char* mqtt_server = "mqtt.ci-ciad.utbm.fr";
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;

// DHT11
#define PIN_DHT11 22 
#define DHTTYPE DHT11
DHT dht(PIN_DHT11, DHTTYPE);

//fan
#define Fan 21    //define driver pins

// BMP280 Sensor
Adafruit_BMP280 bmp;
#define SDA_PIN 33
#define SCL_PIN 32

// OLED Display
#define OLED_RESET 4
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

// Air Quality Sensor
DFRobot_CCS811 CCS811;

//Define buzzerPin
int buzzerPin = 14; 

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
  if (strcmp(topic, "esp32/ventilateur") == 0){
    if (messageTemp == "ON") {
      Serial.println("boutton appuyé");
      if(ventilateur == 4){
        ventilateur = 0; 
      }else{
        ventilateur += 1;
      }
    } 
  
}

// Function to reconnect to MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("espclient")) {
      Serial.println("connected");
      client.subscribe("esp32/projet_qualitee_air");
      clientNOEL.subscribe("esp32/ventilateur");

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

  
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    ^m^!// clock_prescale_set(clock_div_1);
  #endif
  // END of Trinket-specific code.

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)

  //ventilateur 
  pinMode(Fan,OUTPUT);
  
  //bouton
  pinMode(buttonPin, INPUT_PULLUP);

  Wire.begin(SDA_PIN, SCL_PIN);

  pinMode(buzzerPin, OUTPUT); //Set buzzerPin as output

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

  // DHT 11 sensor 
  pinMode(PIN_DHT11, INPUT);
  dht.begin();

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
  // Read DHT sensor data  
  float humidity = dht.readHumidity();  
  if (isnan(humidity) ) {
    Serial.println("Echec reception, d'un ou des capteurs");
    return;
  }
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


  if (CCS811.checkDataReady() == true) {
    display.print("CO2: ");
    display.print(CCS811.getCO2PPM());
    display.print("ppm, TVOC: ");
    display.print(CCS811.getTVOCPPB());
    display.println("ppb");

    if (CCS811.getCO2PPM()> 600){
      
      analogWrite(buzzerPin, 200); //Setting pin to high
      delay(50); //Delaying
      analogWrite(buzzerPin ,0); //Setting pin to LOW 
      Serial.print("buzzer");
    }

    Serial.print("Humidite : ");
    Serial.println(humidity);
    Serial.print("CO2: ");
    Serial.print(CCS811.getCO2PPM());
    Serial.print("ppm, TVOC: ");
    Serial.print(CCS811.getTVOCPPB());
    Serial.println("ppb");


  }
  display.display();

  CCS811.writeBaseLine(0x447B);

  //relatif au ventilateur 
  if (ventilateur == 0){
      strip.clear(); 
      strip.show();
    }else if (ventilateur ==  1){
      strip.setPixelColor(0, strip.Color(50, 50, 50));  
      strip.setPixelColor(1, strip.Color(50, 50, 50));  
      strip.show();
    }else if (ventilateur == 2){ 
      strip.setPixelColor(2, strip.Color(50, 50, 50));  
      strip.setPixelColor(3, strip.Color(50, 50, 50));  
      strip.show();
    }else if (ventilateur == 3){ 
      strip.setPixelColor(4, strip.Color(50, 50, 50));  
      strip.setPixelColor(5, strip.Color(50, 50, 50));  
      strip.show();
    }else if (ventilateur == 4){ 
      strip.setPixelColor(6, strip.Color(50, 50, 50));  
      strip.setPixelColor(7, strip.Color(50, 50, 50));  
      strip.show();
    }
  analogWrite(Fan, ventilateur*64);
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW) {
    if(ventilateur == 4){
      ventilateur = 0; 
    }else{
      ventilateur += 1;
    }
  }

  // MQTT handling
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
    client.publish("esp32/projet_qualitee_air", result);
  }

  // Delay before next loop iteration
  delay(5000);
}