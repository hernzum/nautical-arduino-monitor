#include <Arduino.h>
#include <DHT.h>
#include <ArduinoJson.h> // Incluir la biblioteca ArduinoJson

// =========================== CONFIGURACIÓN DE PINES ===========================
#define DHTPIN 2            // Sensor de temperatura y humedad
#define DHTTYPE DHT11
#define WATER_SENSOR_PIN A0 // Sensor de nivel de agua
#define GAS_SENSOR_PIN A5   // Sensor de gas (MQ-2, MQ-5, etc.)
#define SHUNT_SENSOR_PIN A4 // Medición del voltaje en el shunt
#define SWITCH_PIN 3        // Pulsador para cambiar modos
#define BUZZER_PIN 7        // Buzzer de alerta
#define LED_GREEN 4         // LED verde (estado normal)
#define LED_YELLOW 5        // LED amarillo (advertencia)
#define LED_RED 6           // LED rojo (alarma crítica)

// Pines de baterías
const uint8_t batteryPins[3] = {A1, A2, A3};

// ========================= CONFIGURACIÓN DE BATERÍAS =========================
struct BatteryConfig {
  float min_voltage;
  float max_voltage;
  float calibration;
};

// Configuración individual para cada batería
const BatteryConfig batteries[3] = {
  {11.8, 12.6, 1.02}, // Batería 1 (Plomo-Ácido)
  {12.0, 14.6, 1.01}, // Batería 2 (Litio)
  {10.5, 12.5, 1.03}  // Batería 3 (AGM)
};

// Factor de división del divisor de voltaje utilizado para medir las baterías
const float VOLTAGE_DIVIDER_RATIO = (680000.0 + 80000.0) / 80000.0;

// =========================== CONFIGURACIÓN DEL SHUNT ===========================
const float SHUNT_RESISTANCE = 0.00025; // Resistencia del shunt (300A/75mV → 0.00025Ω)
const float SHUNT_CALIBRATION = 1.0;    // Factor de calibración del shunt

// Umbral crítico para detectar fugas de gas
const int GAS_THRESHOLD = 500;

// Inicialización del sensor DHT
DHT dht(DHTPIN, DHTTYPE);

// Variables globales
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 5000; // Enviar datos cada 5s
bool alarmActive = false;
String displayMode = "Batteries"; // Variable para almacenar el modo de visualización: "Batteries" o "Water"
unsigned long buttonPressTime = 0;

// ========================= FUNCIONES DE LECTURA =========================

float readBatteryVoltage(uint8_t pin, float calibration) {
  int raw = analogRead(pin);
  float v_out = (raw * 3.3) / 1023.0;
  return v_out * VOLTAGE_DIVIDER_RATIO * calibration;
}

float calculateSOC(float voltage, float minV, float maxV) {
  return constrain((voltage - minV) / (maxV - minV) * 100.0, 0.0, 100.0);
}

float readWaterLevel() {
  return map(analogRead(WATER_SENSOR_PIN), 0, 1023, 0, 100) / 100.0;
}

float readCurrent() {
  int raw = analogRead(SHUNT_SENSOR_PIN);
  float v_shunt = (raw * 3.3) / 1023.0;
  return (v_shunt / SHUNT_RESISTANCE) * SHUNT_CALIBRATION;
}

void readEnvironment(float &temperature, float &humidity) {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
}

bool checkGasLeak() {
  int gasLevel = analogRead(GAS_SENSOR_PIN);
  return gasLevel > GAS_THRESHOLD;
}

// ========================= CONTROL DE ALARMAS Y LEDs =========================

void updateLEDs(bool gasLeak, float voltages[], float waterLevel) {
  bool allBatteriesOK = true;
  bool anyBatteryDischarging = false;

  for (uint8_t i = 0; i < 3; i++) {
    if (voltages[i] < batteries[i].min_voltage) {
      allBatteriesOK = false;
    }
    if (readCurrent() < 0) {
      anyBatteryDischarging = true;
    }
  }

  if (displayMode == "Batteries") {
    if (gasLeak || !allBatteriesOK) {
      digitalWrite(LED_RED, HIGH);   // Encender LED Rojo fijo (alarma crítica por batería baja)
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_GREEN, LOW);
      alarmActive = true;
    } else if (anyBatteryDischarging) {
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_YELLOW, HIGH); // Encender LED Amarillo (advertencia)
      digitalWrite(LED_GREEN, LOW);
      alarmActive = false;
    } else {
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_GREEN, HIGH); // Encender LED Verde (estado normal)
      alarmActive = false;
    }
  } else if (displayMode == "Water") {
    if (gasLeak) {
      digitalWrite(LED_RED, HIGH);   // Encender LED Rojo fijo (alarma crítica por fuga de gas)
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_GREEN, LOW);
      alarmActive = true;
    } else if (waterLevel > 0.7) {
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_GREEN, HIGH); // Encender LED Verde (nivel suficiente)
      alarmActive = false;
    } else if (waterLevel > 0.3 && waterLevel <= 0.7) {
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_YELLOW, HIGH); // Encender LED Amarillo (nivel medio)
      digitalWrite(LED_GREEN, LOW);
      alarmActive = false;
    } else {
      static unsigned long lastBlinkTime = 0;
      unsigned long currentTime = millis();
      if (currentTime - lastBlinkTime >= 500) {
        lastBlinkTime = currentTime;
        digitalWrite(LED_RED, digitalRead(LED_RED) == LOW ? HIGH : LOW); // Parpadeo del LED Rojo
      }
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_GREEN, LOW);
      alarmActive = true;
    }
  }

  if (alarmActive && gasLeak) {
    tone(BUZZER_PIN, 1000); // Emitir tono de alarma
  } else {
    noTone(BUZZER_PIN); // Apagar buzzer
  }
}

// ========================= ENVÍO DE DATOS A SIGNALK =========================

void sendAllData() {
  StaticJsonDocument<512> doc;
  JsonArray updates = doc.createNestedArray("updates");
  JsonObject update = updates.createNestedObject();
  update["timestamp"] = millis();

  JsonArray values = update.createNestedArray("values");

  // Leer datos de baterías
  float voltages[3];
  for (uint8_t i = 0; i < 3; i++) {
    voltages[i] = readBatteryVoltage(batteryPins[i], batteries[i].calibration);
    values.createNestedObject()["path"] = "electrical.batteries." + String(i) + ".voltage";
    values[values.size() - 1]["value"] = voltages[i];

    values.createNestedObject()["path"] = "electrical.batteries." + String(i) + ".stateOfCharge";
    values[values.size() - 1]["value"] = calculateSOC(voltages[i], batteries[i].min_voltage, batteries[i].max_voltage) / 100.0;
  }

  // Leer datos del nivel del tanque de agua
  float waterLevel = readWaterLevel();
  values.createNestedObject()["path"] = "tanks.freshWater.0.currentLevel";
  values[values.size() - 1]["value"] = waterLevel;

  // Leer datos del consumo de corriente (shunt)
  float current = readCurrent();
  values.createNestedObject()["path"] = "electrical.batteries.0.current";
  values[values.size() - 1]["value"] = current;

  // Leer datos del sensor DHT (temperatura y humedad)
  float temperatureC, humidity;
  readEnvironment(temperatureC, humidity);

  // Convertir temperatura de Celsius a Kelvin
  float temperatureK = temperatureC + 273.15;

  values.createNestedObject()["path"] = "environment.inside.temperature";
  values[values.size() - 1]["value"] = temperatureK; // Enviar temperatura en Kelvin

  values.createNestedObject()["path"] = "environment.inside.relativeHumidity";
  values[values.size() - 1]["value"] = humidity;

  // Leer datos de fuga de gas
  bool gasLeak = checkGasLeak();
  values.createNestedObject()["path"] = "safety.gasLeak";
  values[values.size() - 1]["value"] = gasLeak;

  // Serializar y enviar el documento JSON al puerto serial
  serializeJson(doc, Serial);
  Serial.println();
}
// ========================= GESTIÓN DEL BOTÓN =========================

void handleButton() {
  if (digitalRead(SWITCH_PIN) == LOW) {
    unsigned long currentTime = millis();

    if (buttonPressTime == 0) {
      buttonPressTime = currentTime;
    } else if (currentTime - buttonPressTime >= 2000) {
      // Cambiar entre "Modo Baterías" y "Modo Agua" (solo afecta la visualización de LEDs)
      displayMode = (displayMode == "Batteries") ? "Water" : "Batteries";
      Serial.println(displayMode == "Batteries" ? "Modo Baterías Activado" : "Modo Agua Activado");
      buttonPressTime = 0;
    }
  } else {
    if (buttonPressTime > 0 && millis() - buttonPressTime < 2000) {
      alarmActive = false;
      digitalWrite(LED_RED, LOW);
      noTone(BUZZER_PIN);
      Serial.println("Alarma Restablecida");
    }
    buttonPressTime = 0;
  }
}

// ========================= LOOP PRINCIPAL =========================

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
}

void loop() {
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    sendAllData();
    lastUpdate = millis();
  }
  handleButton();
}
