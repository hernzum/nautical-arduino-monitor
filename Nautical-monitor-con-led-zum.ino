/*
 * Nautical Arduino Monitor - Hernzum
 * Versión: 1.0
 * Descripción:
 * - Monitorea baterías (voltaje, SOC, capacidad restante).
 * - Mide temperatura, humedad y nivel del tanque de agua.
 * - Genera alarmas con LEDs y buzzer en caso de fallas críticas.
 * - Envía datos a SignalK en formato JSON por UART (115200 baud).
 * - Usa un pulsador para cambiar modos y resetear alarmas.
 */

#include <Arduino.h>
#include <DHT.h>
#include <ArduinoJson.h>

// =========================== CONFIGURACIÓN DE PINES ===========================
// Sensores y actuadores
#define DHTPIN 2
#define DHTTYPE DHT11
#define WATER_SENSOR_PIN A0
#define GAS_SENSOR_PIN A5
#define SWITCH_PIN 3
#define BUZZER_PIN 7
#define LED_GREEN 4
#define LED_YELLOW 5
#define LED_RED 6

// Pines de baterías (A1-A3)
#define BATTERY_1_PIN A1
#define BATTERY_2_PIN A2
#define BATTERY_3_PIN A3

// Configuración de resistencias del divisor de voltaje
const float R1 = 680000.0;  // Resistencia superior (680kΩ)
const float R2 = 80000.0;   // Resistencia inferior (80kΩ)
const float VOLTAGE_DIVIDER_RATIO = (R1 + R2) / R2;  // Factor de escala (9.5)

// Factores de calibración individuales
float CALIBRATION_FACTOR_BAT1 = 1.02;
float CALIBRATION_FACTOR_BAT2 = 1.01;
float CALIBRATION_FACTOR_BAT3 = 1.03;

// Umbrales de voltaje de baterías y agua
const float BAT_CRITICAL = 20.0;
const int WATER_CRITICAL = 25;

// Configuración del sensor DHT
DHT dht(DHTPIN, DHTTYPE);

// Variables globales
bool alarmActive = false;
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 5000; // Enviar datos cada 5s

// ========================= FUNCIONES DE LECTURA =========================

// 🔹 Leer voltaje de batería con divisor de voltaje y factor de calibración
float readBatteryVoltage(uint8_t pin, float calibrationFactor) {
  int raw = analogRead(pin);
  float v_out = (raw * 3.3) / 1023.0;  // Convertir ADC a voltaje real en la entrada del Arduino
  float v_in = v_out * VOLTAGE_DIVIDER_RATIO * calibrationFactor;  // Aplicar corrección del divisor y calibración
  return v_in;  // Devolver el voltaje corregido
}

// 🔹 Calcular el estado de carga (SOC) basado en el voltaje
float calculateSOC(float voltage) {
  float minV = 11.8;
  float maxV = 12.6;
  return constrain((voltage - minV) / (maxV - minV) * 100.0, 0.0, 100.0);
}

// 🔹 Leer el nivel del tanque de agua en porcentaje
float readWaterLevel() {
  return map(analogRead(WATER_SENSOR_PIN), 0, 1023, 0, 100) / 100.0;
}

// 🔹 Leer temperatura y humedad del ambiente
void readEnvironment(float &temperature, float &humidity) {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
}

// ========================= MANEJO DE ALARMAS =========================

// 🔹 Verificar estado del sistema y activar alarmas si es necesario
void checkSystemStatus() {
  alarmActive = false;

  // Verificar voltajes de baterías
  float batteryVoltages[] = {
    readBatteryVoltage(BATTERY_1_PIN, CALIBRATION_FACTOR_BAT1),
    readBatteryVoltage(BATTERY_2_PIN, CALIBRATION_FACTOR_BAT2),
    readBatteryVoltage(BATTERY_3_PIN, CALIBRATION_FACTOR_BAT3)
  };

  for (int i = 0; i < 3; i++) {
    if (batteryVoltages[i] < 10.5) {
      alarmActive = true;
    }
  }

  // Verificar nivel de agua
  if (readWaterLevel() < WATER_CRITICAL / 100.0) {
    alarmActive = true;
  }

  // Activar buzzer si hay alarma
  digitalWrite(BUZZER_PIN, alarmActive ? HIGH : LOW);
}

// ========================= FUNCIONES DE ENVÍO DE DATOS =========================

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

  float batteryVoltages[] = {
    readBatteryVoltage(BATTERY_1_PIN, CALIBRATION_FACTOR_BAT1),
    readBatteryVoltage(BATTERY_2_PIN, CALIBRATION_FACTOR_BAT2),
    readBatteryVoltage(BATTERY_3_PIN, CALIBRATION_FACTOR_BAT3)
  };

  for (uint8_t i = 0; i < 3; i++) {
    JsonObject voltObj = values.createNestedObject();
    voltObj["path"] = "electrical.batteries." + String(i) + ".voltage";
    voltObj["value"] = batteryVoltages[i];

    JsonObject socObj = values.createNestedObject();
    socObj["path"] = "electrical.batteries." + String(i) + ".stateOfCharge";
    socObj["value"] = calculateSOC(batteryVoltages[i]) / 100.0;
  }

  serializeJson(doc, Serial);
  Serial.println();
}

// 🔹 Enviar datos ambientales en JSON
void sendEnvironmentData() {
  StaticJsonDocument<256> doc;
  JsonArray updates = doc.createNestedArray("updates");
  JsonObject update = updates.createNestedObject();
  JsonObject source = update.createNestedObject("source");
  
  source["label"] = "arduino-mini";
  source["type"] = "sensor";
  update["timestamp"] = millis();
  JsonArray values = update.createNestedArray("values");

  float temp, hum;
  readEnvironment(temp, hum);

  JsonObject tempObj = values.createNestedObject();
  tempObj["path"] = "environment.inside.temperature";
  tempObj["value"] = temp + 273.15;

  JsonObject humObj = values.createNestedObject();
  humObj["path"] = "environment.inside.relativeHumidity";
  humObj["value"] = hum / 100.0;

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
    sendEnvironmentData();
    delay(100);
    lastUpdate = millis();
  }

  checkSystemStatus();
}
