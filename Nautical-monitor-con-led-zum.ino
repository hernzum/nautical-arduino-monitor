/*
 * Proyecto: Nautical Monitor para SignalK
 * Autor: Hernzum
 * Plataforma: Arduino Mini + ROCK 4C+
 * Funcionalidad: Monitoreo de baterías (sensores 0-25V), ambiente, tanque de agua y alarmas.
 */

#include <Arduino.h>
#include <DHT.h>
#include <ArduinoJson.h>

// =========================== CONFIGURACIÓN DE PINES ===========================
#define DHTPIN 2
#define DHTTYPE DHT11
#define GAS_SENSOR_PIN A5         
#define WATER_SENSOR_PIN A0       
#define SWITCH_PIN 3              
#define BUZZER_PIN 7              
#define LED_GREEN 4               
#define LED_YELLOW 5              
#define LED_RED 6                 
#define BATTERY_PINS {A1, A2, A3}  
#define SHUNT_PIN A4              

DHT dht(DHTPIN, DHTTYPE);

// ======================== VARIABLES GLOBALES =========================
const uint8_t batPins[] = BATTERY_PINS;
const unsigned long UPDATE_INTERVAL = 5000;  // Intervalo de actualización (5s)
unsigned long lastUpdate = 0;

// Definición de las baterías
struct BatteryConfig {
  float min_voltage;
  float max_voltage;
  float capacity;
};

BatteryConfig batteries[3] = {
  {11.8, 12.6, 100},  // 🔹 Bloque 0 (Plomo-Ácido)
  {12.0, 14.6, 80},   // 🔹 Bloque 1 (Litio)
  {10.5, 12.5, 60}    // 🔹 Bloque 2 (Otro tipo)
};

// ======================== CONFIGURACIÓN INICIAL =========================
void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
}

// ======================== FUNCIONES DE LECTURA =========================

// 🔹 Lectura de voltaje desde los sensores 0-25V (salida 0-5V)
float readBatteryVoltage(uint8_t index) {
  int raw = analogRead(batPins[index]);
  return (raw * 5.0) / 1023.0 * 5.0;  // Conversión correcta para sensores 0-25V
}

// 🔹 Cálculo del Estado de Carga (SOC) de la batería
float calculateSOC(uint8_t index) {
  float voltage = readBatteryVoltage(index);
  float soc = (voltage - batteries[index].min_voltage) /
              (batteries[index].max_voltage - batteries[index].min_voltage) * 100.0;
  return constrain(soc, 0.0, 100.0);
}

// 🔹 Lectura del nivel de agua (0-100%)
float readWaterLevel() {
  return map(analogRead(WATER_SENSOR_PIN), 0, 1023, 0, 100) / 100.0;
}

// ======================== ENVÍO DE DATOS A SIGNALK =========================

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
    float voltage = readBatteryVoltage(i);
    float soc = calculateSOC(i);
    
    char path[50];

    // 🔹 Voltaje de la batería
    JsonObject voltObj = values.createNestedObject();
    snprintf(path, sizeof(path), "electrical.batteries.%d.voltage", i);
    voltObj["path"] = path;
    voltObj["value"] = voltage;

    // 🔹 Estado de Carga (SOC)
    JsonObject socObj = values.createNestedObject();
    snprintf(path, sizeof(path), "electrical.batteries.%d.stateOfCharge", i);
    socObj["path"] = path;
    socObj["value"] = soc / 100.0; // Normalizado a 0.0-1.0

    // 🔹 Capacidad restante (Coulombs)
    JsonObject capObj = values.createNestedObject();
    snprintf(path, sizeof(path), "electrical.batteries.%d.capacity.remaining", i);
    capObj["path"] = path;
    capObj["value"] = (soc / 100.0) * batteries[i].capacity * 3600.0;
  }

  serializeJson(doc, Serial);
  Serial.println();
}

void sendEnvironmentData() {
  StaticJsonDocument<256> doc;
  JsonArray updates = doc.createNestedArray("updates");
  JsonObject update = updates.createNestedObject();
  
  JsonObject source = update.createNestedObject("source");
  source["label"] = "arduino-mini";
  source["type"] = "sensor";
  
  update["timestamp"] = millis();
  JsonArray values = update.createNestedArray("values");

  float temp = dht.readTemperature();
  if (!isnan(temp)) {
    JsonObject tempObj = values.createNestedObject();
    tempObj["path"] = "environment.inside.temperature";
    tempObj["value"] = temp + 273.15;
  }

  float hum = dht.readHumidity();
  if (!isnan(hum)) {
    JsonObject humObj = values.createNestedObject();
    humObj["path"] = "environment.inside.relativeHumidity";
    humObj["value"] = hum / 100.0;
  }

  serializeJson(doc, Serial);
  Serial.println();
}

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

// ======================== LOOP PRINCIPAL =========================
void loop() {
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    sendBatteryData();
    delay(100);  // 🔹 Pequeña pausa para separar los mensajes
    sendEnvironmentData();
    delay(100);
    sendWaterTankData();
    lastUpdate = millis();
  }
}
