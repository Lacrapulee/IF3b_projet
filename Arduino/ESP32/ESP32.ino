#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <iostream>
#include <string>
#include <cstring> 

// Déclaration des variables relative à la DB ; 

const String Object_id = "6661de566a8872a50807db59"
const char* ssid = "Gauthier";
const char* password = "y3rfajkj";
const char* mqtt_server = "mqtt.ci-ciad.utbm.fr";

WiFiClient espclient;
PubSubClient client(espclient);
long lastMsg = 0;

// Déclaration du capteur BMP280
Adafruit_BMP280 bmp;
#define SDA_PIN 33
#define SCL_PIN 32

// Déclaration de l'écran OLED 
#define OLED_RESET 4
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// Déclaration du moisture sensor 
#define DHpin 22  // Mettre à jour avec une broche disponible sur l'ESP32
byte dat[5];

byte read_data() {
  byte data = 0;
  for (int i = 0; i < 8; i++) {
    if (digitalRead(DHpin) == LOW) {
      while (digitalRead(DHpin) == LOW);
      delayMicroseconds(30);
      if (digitalRead(DHpin) == HIGH)
        data |= (1 << (7 - i));
      while (digitalRead(DHpin) == HIGH);
    }
  }
  return data;
}

void start_test () {
digitalWrite (DHpin, LOW);
delay (30);
digitalWrite (DHpin, HIGH);
delayMicroseconds (40);
pinMode (DHpin, INPUT);
while (digitalRead (DHpin) == HIGH);
delayMicroseconds (80);
  if (digitalRead (DHpin) == LOW);
    delayMicroseconds (80);
for (int i = 0; i < 4; i ++){
    dat[i] = read_data ();
    pinMode (DHpin, OUTPUT);
    digitalWrite (DHpin, HIGH);
}
}

// Initialisation de l'écran led 
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

const uint8_t digits[10][5] = {
  {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
  {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
  {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
  {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
  {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
  {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
  {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
  {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
  {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
  {0x06, 0x49, 0x49, 0x29, 0x1E}  // 9
};

void setup() {

  // Initialisation de la communication série
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialisation du Moisture sensor
  pinMode(DHpin, OUTPUT);

  // Initialisation du capteur BMP280
  if (!bmp.begin(0x76)) {
    Serial.println(F("Impossible de trouver le capteur BMP280. Vérifiez les connexions."));
    while (1);
  }

  // Configuration du capteur
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,   // oversampling de la température
                  Adafruit_BMP280::SAMPLING_X16,  // oversampling de la pression
                  Adafruit_BMP280::FILTER_X16,    // filtrage
                  Adafruit_BMP280::STANDBY_MS_500); // temps de veille

  // Initialisation de l'écran OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Adresse I2C de l'écran OLED
    Serial.println(F("Impossible de trouver l'écran OLED. Vérifiez les connexions."));
    while (1);
  }
  display.display();
  delay(2000); // Pause pour permettre à l'écran de s'initialiser
  display.clearDisplay();

  // Initialisation de 
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

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

void loop() {
  
  // Initialisation de la puce ST052 (humidité)
  start_test ();

  // Lecture de la température
  float temperature = bmp.readTemperature();
  // Lecture de la pression
  float pressure = bmp.readPressure() / 100.0F; // conversion en hPa
  // Lecture de l'altitude estimée
  float altitude = bmp.readAltitude(1013.25); // pression atmosphérique au niveau de la mer en hPa

  // Affichage des valeurs sur l'écran OLED
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
  display.display();
  
  // Affichage des valeurs lues sur le moniteur série
  Serial.print(F("Température = "));
  Serial.print(temperature);
  Serial.println(" *C");
  Serial.print(F("Pression = "));
  Serial.print(pressure);
  Serial.println(" hPa");
  Serial.print(F("Altitude approximative = "));
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


  // Code relatif au mqtt 

  //reconnection automatique 
  if (!client.connected()) {
    reconnect();
  }

  client.loop();
  long now = millis();

  std::String msg = Object_id + "-" + 
  std::to_string(temperature) + "-" + 
  std::to_string(dat[0]) + "." + 
  std::to_string(dat[1]) + "-" + 
  std::to_string(pressure) + "-" 
   ;
  
  const int length = msg.length(); 
  char* msg_char = new char[length + 1]; 
  strcpy(msg_char, msg.c_str()); 

  if (now - lastMsg > 6000) {
    lastMsg = now;
    client.publish("esp32/projet_qualitee_air", msg_char);
  }
  // Pause avant la prochaine lecture
  delay(2000);
}