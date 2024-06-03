#include <Wire.h>
#include <Adafruit_BMP280.h>

#define SDA_PIN 21
#define SCL_PIN 22

// Use default I2C address 0x76 or 0x77
Adafruit_BMP280 bmp;

void setup() {
    Serial.begin(9600);
    Serial.println(F("BMP280 test"));
   
    // Initialiser l'interface I2C avec les broches GPIO spécifiées
    Wire.begin(SDA_PIN, SCL_PIN);

    // Initialize the BMP280 sensor
    if (!bmp.begin(0x77)) {  // Use the appropriate I2C address
        Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
        while (1);
    }
}

void loop() {
    Serial.print(F("Temperature = "));
    Serial.print(bmp.readTemperature());
    Serial.println(" *C");
    
    Serial.print(F("Pressure = "));
    Serial.print(bmp.readPressure());
    Serial.println(" Pa");
    
    Serial.print(F("Approx altitude = "));
    Serial.print(bmp.readAltitude(1013.25)); // this should be adjusted to your local forecast
    Serial.println(" m");
    
    Serial.println();
    delay(2000);
}
