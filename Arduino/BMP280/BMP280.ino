#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Déclaration du capteur BMP280
Adafruit_BMP280 bmp;
#define SDA_PIN 33
#define SCL_PIN 32

// Déclaration de l'écran OLED 
#define OLED_RESET 4
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// Déclaration du moisture sensor 
#define DHpin 13  // Mettre à jour avec une broche disponible sur l'ESP32
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
}

void loop() {
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

  // Pause avant la prochaine lecture
  delay(2000);
}