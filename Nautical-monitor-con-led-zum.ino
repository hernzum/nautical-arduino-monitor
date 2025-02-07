/*
 * Nautical Arduino Monitor - Versión 1.2
 * Descripción:
 * - Monitorea baterías con divisores de voltaje y calibración individual.
 * - Mide temperatura, humedad, nivel del tanque de agua y fugas de gas.
 * - Mide el consumo de corriente mediante un shunt de 300A/75mV en la batería 1.
 * - Genera alarmas con LEDs y buzzer en caso de fallas críticas.
 * - Pulsador para resetear alarmas y cambiar entre modos.
 * - Envío de datos a SignalK en JSON organizado en bloques.
 */

// Incluimos las bibliotecas necesarias
#include <Arduino.h>       // Biblioteca estándar de Arduino
#include <DHT.h>           // Biblioteca para el sensor DHT (temperatura y humedad)
#include <ArduinoJson.h>   // Biblioteca para manejo de JSON

// =========================== CONFIGURACIÓN DE PINES ===========================
#define DHTPIN 2            // Pin del sensor DHT (temperatura y humedad)
#define DHTTYPE DHT11       // Tipo de sensor DHT (DHT11 en este caso)
#define WATER_SENSOR_PIN A0 // Pin analógico para el sensor de nivel de agua
#define GAS_SENSOR_PIN A5   // Pin analógico para el sensor de gas (MQ-2, MQ-5, etc.)
#define SHUNT_SENSOR_PIN A4 // Pin analógico para medir el voltaje en el shunt
#define SWITCH_PIN 3        // Pin digital para el pulsador de reset y cambio de modo
#define BUZZER_PIN 7        // Pin digital para el buzzer de alerta
#define LED_GREEN 4         // Pin digital para el LED verde (estado normal)
#define LED_YELLOW 5        // Pin digital para el LED amarillo (advertencia)
#define LED_RED 6           // Pin digital para el LED rojo (alarma crítica)

// Configuración de los pines de las baterías
const uint8_t batteryPins[3] = {A1, A2, A3}; // Pins analógicos para las tres baterías

// ========================= CONFIGURACIÓN DE BATERÍAS =========================
// Estructura para almacenar la configuración de cada batería
struct BatteryConfig {
  float min_voltage;    // Voltaje mínimo permitido
  float max_voltage;    // Voltaje máximo permitido
  float calibration;    // Factor de calibración del divisor de voltaje
};
// Configuración de las tres baterías
const BatteryConfig batteries[3] = {
  {11.8, 12.6, 1.02}, // Batería 1 (Plomo-Ácido): Voltaje mínimo = 11.8V, máximo = 12.6V, calibración = 1.02
  {12.0, 14.6, 1.01}, // Batería 2 (Litio): Voltaje mínimo = 12.0V, máximo = 14.6V, calibración = 1.01
  {10.5, 12.5, 1.03}  // Batería 3 (AGM): Voltaje mínimo = 10.5V, máximo = 12.5V, calibración = 1.03
};

// Configuración del divisor de voltaje para las baterías
const float VOLTAGE_DIVIDER_RATIO = (680000.0 + 80000.0) / 80000.0; // Relación del divisor de voltaje (R1 + R2) / R2

// =========================== CONFIGURACIÓN DEL SHUNT ===========================
const float SHUNT_RESISTANCE = 0.00025; // Resistencia del shunt (300A/75mV → 0.00025Ω)
const float SHUNT_CALIBRATION = 1.0;    // Factor de calibración del shunt

// =========================== CONFIGURACIÓN DEL SENSOR DE GAS ===========================
const int GAS_THRESHOLD = 500; // Umbral crítico para detectar fugas de gas

// Configuración del sensor DHT
DHT dht(DHTPIN, DHTTYPE); // Inicialización del sensor DHT

// Variables globales
unsigned long lastUpdate = 0;          // Tiempo de la última actualización de datos
const unsigned long UPDATE_INTERVAL = 5000; // Intervalo de actualización (5 segundos)
bool alarmActive = false;              // Estado de la alarma (activada/desactivada)
bool detailedMode = false;             // Modo detallado activo/inactivo
unsigned long buttonPressTime = 0;     // Tiempo de presión del botón

// ========================= FUNCIONES DE LECTURA =========================

// Función para leer el voltaje de una batería
float readBatteryVoltage(uint8_t pin, float calibration) {
  int raw = analogRead(pin);                     // Leer el valor analógico del pin
  float v_out = (raw * 3.3) / 1023.0;            // Convertir el valor analógico a voltaje (referencia de 3.3V)
  return v_out * VOLTAGE_DIVIDER_RATIO * calibration; // Calcular el voltaje real usando el divisor de voltaje y la calibración
}

// Función para calcular el estado de carga (SOC) de una batería
float calculateSOC(float voltage, float minV, float maxV) {
  return constrain((voltage - minV) / (maxV - minV) * 100.0, 0.0, 100.0); // Calcular SOC como porcentaje entre minV y maxV
}

// Función para leer el nivel del tanque de agua
float readWaterLevel() {
  return map(analogRead(WATER_SENSOR_PIN), 0, 1023, 0, 100) / 100.0; // Mapear el valor analógico a un rango de 0 a 1 (porcentaje)
}

// Función para leer el consumo de corriente mediante el shunt
float readCurrent() {
  int raw = analogRead(SHUNT_SENSOR_PIN);        // Leer el valor analógico del pin del shunt
  float v_shunt = (raw * 3.3) / 1023.0;          // Convertir el valor analógico a voltaje
  return (v_shunt / SHUNT_RESISTANCE) * SHUNT_CALIBRATION; // Calcular la corriente usando Ohm's Law y la calibración
}

// Función para leer temperatura y humedad del sensor DHT
void readEnvironment(float &temperature, float &humidity) {
  temperature = dht.readTemperature();           // Leer la temperatura del sensor DHT
  humidity = dht.readHumidity();                 // Leer la humedad relativa del sensor DHT
}

// Función para verificar si hay una fuga de gas
bool checkGasLeak() {
  int gasLevel = analogRead(GAS_SENSOR_PIN);     // Leer el valor analógico del sensor de gas
  return gasLevel > GAS_THRESHOLD;               // Devolver true si el nivel de gas supera el umbral crítico
}

// ========================= CONTROL DE ALARMAS Y LEDs =========================

// Función para actualizar el estado de los LEDs según las condiciones
void updateLEDs(bool gasLeak, float current, float voltages[]) {
  bool allBatteriesOK = true;                    // Variable para verificar si todas las baterías están bien
  bool anyBatteryDischarging = false;            // Variable para verificar si alguna batería está descargándose

  // Verificar el estado de cada batería
  for (uint8_t i = 0; i < 3; i++) {
    if (voltages[i] < batteries[i].min_voltage) { // Si el voltaje de una batería está por debajo del mínimo
      allBatteriesOK = false;                    // Marcar que no todas las baterías están bien
    }
    if (current < 0) {                           // Si la corriente es negativa (indicando carga)
      anyBatteryDischarging = true;              // Marcar que alguna batería está descargándose
    }
  }

  // Control de los LEDs
  if (gasLeak || !allBatteriesOK) {              // Si hay una fuga de gas o alguna batería está baja
    digitalWrite(LED_RED, HIGH);                 // Encender el LED Rojo
    digitalWrite(LED_YELLOW, LOW);               // Apagar el LED Amarillo
    digitalWrite(LED_GREEN, LOW);                // Apagar el LED Verde
    alarmActive = true;                          // Activar la alarma
  } else if (anyBatteryDischarging) {            // Si alguna batería está descargándose
    digitalWrite(LED_RED, LOW);                  // Apagar el LED Rojo
    digitalWrite(LED_YELLOW, HIGH);              // Encender el LED Amarillo
    digitalWrite(LED_GREEN, LOW);                // Apagar el LED Verde
    alarmActive = false;                         // Desactivar la alarma
  } else {                                       // Si todo está bien
    digitalWrite(LED_RED, LOW);                  // Apagar el LED Rojo
    digitalWrite(LED_YELLOW, LOW);               // Apagar el LED Amarillo
    digitalWrite(LED_GREEN, HIGH);               // Encender el LED Verde
    alarmActive = false;                         // Desactivar la alarma
  }

  // Control del buzzer
  if (alarmActive) {                             // Si la alarma está activa
    tone(BUZZER_PIN, 1000);                      // Emitir un tono de alarma
  } else {
    noTone(BUZZER_PIN);                          // Apagar el buzzer
  }
}

// ========================= ENVÍO DE DATOS A SIGNALK =========================

// Función para enviar todos los datos a SignalK
void sendAllData() {
  StaticJsonDocument<512> doc;                   // Crear un documento JSON para almacenar los datos
  JsonArray updates = doc.createNestedArray("updates"); // Crear un array "updates" en el documento JSON
  JsonObject update = updates.createNestedObject();     // Crear un objeto "update" dentro del array
  JsonObject source = update.createNestedObject("source"); // Crear un objeto "source" dentro del update
  source["label"] = "arduino-mini";              // Asignar etiqueta al origen de los datos
  source["type"] = "sensor";                     // Especificar que el origen es un sensor
  update["timestamp"] = millis();               // Asignar timestamp actual

  JsonArray values = update.createNestedArray("values"); // Crear un array "values" para almacenar los valores

  // Leer y enviar datos de las baterías
  float voltages[3];                            // Array para almacenar los voltajes de las baterías
  for (uint8_t i = 0; i < 3; i++) {
    voltages[i] = readBatteryVoltage(batteryPins[i], batteries[i].calibration); // Leer el voltaje de cada batería
    JsonObject battery = values.createNestedObject(); // Crear un objeto para cada batería
    battery["path"] = "electrical.batteries." + String(i) + ".voltage"; // Definir la ruta del dato
    battery["value"] = voltages[i];                  // Asignar el valor del voltaje

    JsonObject soc = values.createNestedObject();    // Crear un objeto para el SOC de cada batería
    soc["path"] = "electrical.batteries." + String(i) + ".stateOfCharge"; // Definir la ruta del dato
    soc["value"] = calculateSOC(voltages[i], batteries[i].min_voltage, batteries[i].max_voltage) / 100.0; // Asignar el valor del SOC
  }

  // Leer y enviar datos de corriente
  float current = readCurrent();                    // Leer el consumo de corriente
  JsonObject currentObj = values.createNestedObject(); // Crear un objeto para la corriente
  currentObj["path"] = "electrical.batteries.0.current"; // Definir la ruta del dato
  currentObj["value"] = current;                      // Asignar el valor de la corriente

  // Leer y enviar datos del nivel de agua
  JsonObject water = values.createNestedObject();    // Crear un objeto para el nivel de agua
  water["path"] = "tanks.freshWater.0.currentLevel"; // Definir la ruta del dato
  water["value"] = readWaterLevel();                 // Asignar el valor del nivel de agua

  // Leer y enviar datos de temperatura y humedad
  float temperature, humidity;
  readEnvironment(temperature, humidity);            // Leer temperatura y humedad
  JsonObject tempObj = values.createNestedObject();  // Crear un objeto para la temperatura
  tempObj["path"] = "environment.inside.temperature"; // Definir la ruta del dato
  tempObj["value"] = temperature;                    // Asignar el valor de la temperatura
  JsonObject humObj = values.createNestedObject();   // Crear un objeto para la humedad
  humObj["path"] = "environment.inside.relativeHumidity"; // Definir la ruta del dato
  humObj["value"] = humidity / 100.0;                // Asignar el valor de la humedad (en formato 0.0-1.0)

  // Leer y enviar datos de detección de gas
  bool gasLeak = checkGasLeak();                     // Verificar si hay una fuga de gas
  JsonObject gasObj = values.createNestedObject();   // Crear un objeto para la detección de gas
  gasObj["path"] = "safety.gasLeak";                 // Definir la ruta del dato
  gasObj["value"] = gasLeak;                         // Asignar el valor de la detección de gas

  // Actualizar los LEDs según los datos leídos
  updateLEDs(gasLeak, current, voltages);

  // Serializar y enviar el documento JSON por Serial
  serializeJson(doc, Serial);
  Serial.println();
}

// ========================= GESTIÓN DEL BOTÓN =========================

// Función para manejar el botón de reset y cambio de modo
void handleButton() {
  if (digitalRead(SWITCH_PIN) == LOW) { // Si el botón está presionado
    unsigned long currentTime = millis(); // Obtener el tiempo actual
    if (buttonPressTime == 0) {           // Si es la primera vez que se presiona
      buttonPressTime = currentTime;      // Registrar el tiempo inicial
    } else if (currentTime - buttonPressTime >= 2000) { // Si se mantiene presionado durante más de 2 segundos
      detailedMode = !detailedMode;       // Cambiar entre modo normal y detallado
      Serial.println(detailedMode ? "Modo Detallado Activado" : "Modo Normal Activado"); // Imprimir el cambio de modo
      buttonPressTime = 0;                // Reiniciar el temporizador
    }
  } else {                                // Si el botón no está presionado
    if (buttonPressTime > 0 && millis() - buttonPressTime < 2000) { // Si se presionó brevemente
      alarmActive = false;                // Restablecer la alarma
      digitalWrite(LED_RED, LOW);         // Apagar el LED Rojo
      noTone(BUZZER_PIN);                 // Apagar el buzzer
      Serial.println("Alarma Restablecida"); // Imprimir mensaje de restablecimiento
    }
    buttonPressTime = 0;                  // Reiniciar el temporizador
  }
}

// ========================= SETUP =========================

void setup() {
  Serial.begin(115200);                   // Iniciar comunicación serial a 115200 baudios
  dht.begin();                            // Iniciar el sensor DHT
  pinMode(BUZZER_PIN, OUTPUT);            // Configurar el pin del buzzer como salida
  pinMode(LED_GREEN, OUTPUT);             // Configurar el pin del LED Verde como salida
  pinMode(LED_YELLOW, OUTPUT);            // Configurar el pin del LED Amarillo como salida
  pinMode(LED_RED, OUTPUT);               // Configurar el pin del LED Rojo como salida
  pinMode(SWITCH_PIN, INPUT_PULLUP);      // Configurar el pin del botón como entrada con resistencia pull-up
}

// ========================= LOOP PRINCIPAL =========================

void loop() {
  if (millis() - lastUpdate >= UPDATE_INTERVAL) { // Si ha pasado el intervalo de actualización
    sendAllData();                                // Enviar los datos a SignalK
    lastUpdate = millis();                        // Actualizar el tiempo de la última actualización
  }

  handleButton(); // Manejar el botón de reset y cambio de modo
}
