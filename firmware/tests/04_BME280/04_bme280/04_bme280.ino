#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// =========================
// I2C
// =========================

#define SDA_PIN 8
#define SCL_PIN 9

// =========================
// OLED
// =========================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// =========================
// BME280
// =========================

Adafruit_BME280 bme;


void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("==============================");
  Serial.println("ESP32-S3 + BME280 + OLED");
  Serial.println("==============================");


  // =========================
  // I2C
  // =========================

  Wire.begin(SDA_PIN, SCL_PIN);


  // =========================
  // OLED
  // =========================

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C
      )) {

    Serial.println("OLED ERROR!");

    while (1);
  }

  Serial.println("OLED OK!");


  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);

  display.setCursor(0, 0);

  display.println("BME280");

  display.setTextSize(1);

  display.setCursor(0, 30);

  display.println("Starting...");

  display.display();

  delay(1000);


  // =========================
  // BME280
  // =========================

  bool status = bme.begin(
    0x76,
    &Wire
  );

  // Một số module dùng địa chỉ 0x77
  if (!status) {

    Serial.println("BME280 0x76 not found!");

    status = bme.begin(
      0x77,
      &Wire
    );
  }


  if (!status) {

    Serial.println("BME280 ERROR!");

    display.clearDisplay();

    display.setTextSize(1);

    display.setCursor(0, 0);

    display.println("BME280 ERROR");

    display.println();

    display.println("Check wiring:");

    display.println("SDA -> GPIO 8");

    display.println("SCL -> GPIO 9");

    display.println("VCC -> 3V3");

    display.println("GND -> GND");

    display.display();

    while (1);
  }


  Serial.println("BME280 OK!");

  Serial.println();
  Serial.println("Start measuring...");


  // =========================
  // OLED READY
  // =========================

  display.clearDisplay();

  display.setTextSize(2);

  display.setCursor(0, 0);

  display.println("READY");

  display.setTextSize(1);

  display.setCursor(0, 30);

  display.println("BME280 OK");

  display.setCursor(0, 44);

  display.println("Measuring...");

  display.display();

  delay(1500);
}


void loop() {

  // =========================
  // Read BME280
  // =========================

  float temperature =
    bme.readTemperature();

  float humidity =
    bme.readHumidity();

  float pressure =
    bme.readPressure() / 100.0F;


  // =========================
  // Serial Monitor
  // =========================

  Serial.print("Temperature: ");

  Serial.print(temperature);

  Serial.println(" °C");


  Serial.print("Humidity: ");

  Serial.print(humidity);

  Serial.println(" %");


  Serial.print("Pressure: ");

  Serial.print(pressure);

  Serial.println(" hPa");


  Serial.println("--------------------------");


  // =========================
  // OLED
  // =========================

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);


  // Temperature

  display.setTextSize(1);

  display.setCursor(0, 0);

  display.print("Temp: ");

  display.setTextSize(2);

  display.print(temperature, 1);

  display.println(" C");


  // Humidity

  display.setTextSize(1);

  display.setCursor(0, 25);

  display.print("Humidity: ");

  display.print(humidity, 1);

  display.println(" %");


  // Pressure

  display.setCursor(0, 40);

  display.print("Pressure: ");

  display.print(pressure, 1);

  display.println(" hPa");


  display.setCursor(0, 55);

  display.println("BME280");


  display.display();


  delay(1000);
}