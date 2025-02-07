/*
 * Nautical Arduino Monitor - Hernzum
 * Versión: 1.3
 * Descripción:
 * - Monitorea baterías con divisores de voltaje y calibración individual.
 * - Cada batería tiene su propio voltaje máximo y mínimo para calcular el SOC.
 * - Mide temperatura, humedad y nivel del tanque de agua.
 * - Genera alarmas con LEDs y buzzer en caso de fallas críticas.
 * - Envía datos a SignalK en formato JSON por UART (115200 baud).
 */

#include <Arduino.h>
#include <DHT.h>
#include <ArduinoJson.h>

// =========================== CONFIGURACIÓN DE PINES ===========================
#define DHTPIN 2           // Sensor de temperatura y humedad
#define DHTTYPE DHT11
#define WATER_SENSOR_PIN A0 // Sensor de nivel de agua
#define SWITCH_PIN 3        // Pulsador para reset de alarmas
#define BUZZER_PIN 7        // Buzzer de alerta
#define LED_GREEN 4         // LED verde (estado normal)
#define LED_YELLOW 5        // LED amarillo (advertencia)
#define LED_RED 6           // LED rojo (alarma crítica)

// Pines de baterías
#define BATTERY_1_PIN A1
#define BATTERY_2_PIN A2
#define BATTERY_3_PIN A3

// ========================= CONFIGURACIÓN DE BATERÍAS =========================
struct BatteryConfig {
  uint8_t pin;
  float min_voltage;
  float max_voltage;
  float calibration;
};

BatteryConfig batteries[3] = {
  {BATTERY_1_PIN, 11.8, 12.6, 1.02},  // Batería 1 (Plomo-Ácido)
  {BATTERY_2_PIN, 12.0, 14.6, 1.01},  // Batería 2 (Litio)
  {BATTERY_3_PIN, 10.5, 12.5, 1.03}   // Batería 3 (AGM)
};

// Configuración del divisor de voltaje
const float R1 = 680000.0;
const float R2 = 80000.0;
const float VOLTAGE_DIVIDER_RATIO = (R1 + R2) / R2;

// Umbrales del sistema
const int WATER_CRITICAL = 25;
const float BAT_CRITICAL_VOLTAGE = 10.5;

// Configuración del sensor DHT
DHT dht(DHTPIN, DHTTYPE);

// Variables globales
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 5000; // Enviar datos cada 5s

// ========================= FUNCIONES DE LECTURA =========================

// 🔹 Leer voltaje de batería con calibración
float readBatteryVoltage(BatteryConfig battery) {
  int raw = analogRead(battery.pin);
  float v_out = (raw * 3.3) / 1023.0;
  float v_in = v_out * VOLTAGE_DIVIDER_RATIO * battery.calibration;
  return v_in;
}

// 🔹 Calcular el estado de carga (SOC)
float calculateSOC(float voltage, float minV, float maxV) {
  return constrain((voltage - minV) / (maxV - minV) * 100.0, 0.0, 100.0);
}

// 🔹 Leer nivel del tanque de agua (%)
float readWaterLevel() {
  return map(analogRead(WATER_SENSOR_PIN), 0, 1023, 0, 100) / 100.0;
}

// 🔹 Leer temperatura y humedad
void readEnvironment(float &temperature, float &humidity) {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
}

// ========================= ENVÍO DE DATOS A SIGNALK =========================

// 🔹 Enviar datos de baterías en JSON
void sendBatteryData() {
  StaticJsonDocument<512> doc;
  JsonArray updates = doc.createNestedArray("updates");
  JsonObject update = updates.createNestedObject();
  JsonObject source = update.createNestedObject("source");

  source["label"] = "arduino-mini";
  source["type"] = "sensor";
  update["timestamp"] = millis();
  JsonArray values = update.createNestedArray("values");

  for (uint8_t i = 0; i < 3; i++) {
    float voltage = readBatteryVoltage(batteries[i]);

    JsonObject voltObj = values.createNestedObject();
    voltObj["path"] = "electrical.batteries." + String(i) + ".voltage";
    voltObj["value"] = voltage;

    JsonObject socObj = values.createNestedObject();
    socObj["path"] = "electrical.batteries." + String(i) + ".stateOfCharge";
    socObj["value"] = calculateSOC(voltage, batteries[i].min_voltage, batteries[i].max_voltage) / 100.0;
  }

  serializeJson(doc, Serial);
  Serial.println();
}

// 🔹 Enviar datos del tanque de agua en JSON
void sendWaterTankData() {
  StaticJsonDocument<256> doc;
  JsonArray updates = doc.createNestedArray("updates");
  JsonObject update = updates.createNestedObject();
  JsonObject source = update.createNestedObject("source");

  source["label"] = "arduino-mini";
  source["type"] = "sensor";
  update["timestamp"] = millis();
  JsonArray values = update.createNestedArray("values");

  JsonObject waterObj = values.createNestedObject();
  waterObj["path"] = "tanks.freshWater.0.currentLevel";
  waterObj["value"] = readWaterLevel();

  serializeJson(doc, Serial);
  Serial.println();
}

// 🔹 Enviar datos de temperatura y humedad en JSON
void sendEnvironmentData() {
  StaticJsonDocument<256> doc;
  JsonArray updates = doc.createNestedArray("updates");
  JsonObject update = updates.createNestedObject();
  JsonObject source = update.createNestedObject("source");

  source["label"] = "arduino-mini";
  source["type"] = "sensor";
  update["timestamp"] = millis();
  JsonArray values = update.createNestedArray("values");

  float temperature, humidity;
  readEnvironment(temperature, humidity);

  if (!isnan(temperature)) {
    JsonObject tempObj = values.createNestedObject();
    tempObj["path"] = "environment.inside.temperature";
    tempObj["value"] = temperature + 273.15; // Convertir a Kelvin
  }

  if (!isnan(humidity)) {
    JsonObject humObj = values.createNestedObject();
    humObj["path"] = "environment.inside.relativeHumidity";
    humObj["value"] = humidity / 100.0; // Convertir a fracción
  }

  serializeJson(doc, Serial);
  Serial.println();
}

// ========================= LOOP PRINCIPAL =========================
void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    sendBatteryData();
    delay(100);
    sendWaterTankData();
    delay(100);
    sendEnvironmentData();
    delay(100);
    lastUpdate = millis();
  }
}
