// Incluir librerías necesarias
#include <DHT.h>          // Librería para el sensor de temperatura y humedad DHT
#include <ArduinoJson.h>  // Librería para crear mensajes JSON

// ======================== CONFIGURACIÓN GENERAL DEL SISTEMA ========================
// ¡AJUSTA ESTAS VARIABLES SEGÚN TU PLACA ARDUINO!

// Selecciona tu placa (descomenta la que corresponda)
//#define ARDUINO_UNO       // Arduino Uno, Nano, etc. (5V)
//#define ARDUINO_MEGA     // Arduino Mega (5V)
//#define ARDUINO_MINI_PRO // Arduino Mini Pro 5V (5V)
#define ARDUINO_MINI_PRO_3V3 // Arduino Mini Pro 3.3V (3.3V)
//#define ESP8266          // NodeMCU, Wemos D1, etc. (3.3V)
//#define ESP32            // ESP32 (3.3V)

// Configuración automática según la placa seleccionada
#if defined(ARDUINO_UNO) || defined(ARDUINO_MEGA) || defined(ARDUINO_MINI_PRO)
  #define VOLTAGE_REFERENCE 5.0   // Placas de 5V
  #define BOARD_NAME "Arduino 5V"
#elif defined(ARDUINO_MINI_PRO_3V3) || defined(ESP8266) || defined(ESP32)
  #define VOLTAGE_REFERENCE 3.3   // Placas de 3.3V
  #define BOARD_NAME "Arduino 3.3V"
#else
  #error "Debes seleccionar una placa en la configuración"
#endif

// ======================== CONFIGURACIÓN DE PINES ========================
#define DHTPIN 2            // Pin digital 2 para el sensor DHT11
#define DHTTYPE DHT11       // Tipo de sensor DHT que estamos usando
#define WATER_SENSOR_PIN A0 // Pin analógico A0 para el sensor de nivel de agua
#define SHUNT_SENSOR_PIN A4 // Pin analógico A4 para el shunt de medición de corriente
#define SWITCH_PIN 3        // Pin digital 3 para el botón de cambio de modo
#define BUZZER_PIN 7        // Pin digital 7 para el buzzer de alarma
#define LED_GREEN 4         // Pin digital 4 para LED verde (estado normal)
#define LED_YELLOW 5        // Pin digital 5 para LED amarillo (advertencia)
#define LED_RED 6           // Pin digital 6 para LED rojo (alarma)

// Definir los pines analógicos para las 3 baterías
const uint8_t batteryPins[3] = {A1, A2, A3}; 

// ======================= CONFIGURACIÓN DE BATERÍAS ======================
// Estructura para almacenar configuración específica de cada batería
struct BatteryConfig {
  float min_voltage;  // Voltaje mínimo (por debajo de este se considera batería baja)
  float max_voltage;  // Voltaje máximo (para cálculo de SOC)
  float calibration;  // Factor de calibración para ajustar mediciones
};

// Configuración para cada una de las 3 baterías
const BatteryConfig batteries[3] = {
  {11.8, 12.6, 0.961}, // Batería 1: valores típicos para batería de 12V
  {12.0, 14.6, 0.961}, // Batería 2: valores típicos para batería de motor
  {10.5, 12.5, 0.961},  // Batería 3: valores para batería de respaldo
};

// Relación del divisor de voltaje (resistencias en el circuito)
const float VOLTAGE_DIVIDER_RATIO = (680000.0 + 80000.0) / 80000.0;

// Configuración del shunt para medición de corriente
const float SHUNT_RESISTANCE = 0.00025; // 300A/75mV → 0.00025Ω
const float SHUNT_CALIBRATION = 1.024;   // Factor de calibración para ajustar mediciones

// Crear objeto para el sensor DHT
DHT dht(DHTPIN, DHTTYPE);

// Variables globales para el control del sistema
unsigned long lastUpdate = 0;              // Último momento en que se actualizaron los datos
const unsigned long UPDATE_INTERVAL = 5000; // Intervalo de actualización (5 segundos)
bool alarmActive = false;                  // Estado actual de la alarma
String displayMode = "Batteries";          // Modo de visualización actual
unsigned long buttonPressTime = 0;         // Tiempo en que se presionó el botón

// ======================== FUNCIONES DE LECTURA =========================
// Función para leer el voltaje de una batería
float readBatteryVoltage(uint8_t pin, float calibration) {
  // Usa VOLTAGE_REFERENCE definido según la placa seleccionada
  return analogRead(pin) * (VOLTAGE_REFERENCE / 1023.0) * VOLTAGE_DIVIDER_RATIO * calibration;
}

// Función para calcular el Estado de Carga (SOC) de una batería
float calculateSOC(float voltage, float minV, float maxV) {
  // Calcula el SOC como porcentaje entre minV y maxV
  // constrain() asegura que el valor esté entre 0.0 y 100.0
  return constrain((voltage - minV) / (maxV - minV) * 100.0, 0.0, 100.0);
}

// Función para leer el nivel de agua
float readWaterLevel() {
  // Mapea el valor analógico (0-1023) a un rango 0-100 y convierte a decimal
  return map(analogRead(WATER_SENSOR_PIN), 0, 1023, 0, 100) / 100.0;
}

// Función para leer la corriente
float readCurrent() {
  // Usa VOLTAGE_REFERENCE definido según la placa seleccionada
  return (analogRead(SHUNT_SENSOR_PIN) * (VOLTAGE_REFERENCE / 1023.0) / SHUNT_RESISTANCE) * SHUNT_CALIBRATION;
}

// Función para leer temperatura y humedad del DHT
void readEnvironment(float &temperature, float &humidity) {
  temperature = dht.readTemperature(); // Lee temperatura en °C
  humidity = dht.readHumidity();       // Lee humedad relativa en %
}

// ========================= CONTROL DE ALARMAS Y LEDs =========================
// Función para actualizar los LEDs según el estado del sistema
void updateLEDs(float voltages[], float waterLevel) {
  bool allBatteriesOK = true;        // Estado de todas las baterías
  bool anyBatteryDischarging = false; // ¿Alguna batería está descargando?

  // Verificar el estado de todas las baterías
  for (uint8_t i = 0; i < 3; i++) {
    if (voltages[i] < batteries[i].min_voltage) allBatteriesOK = false;
    if (readCurrent() < 0) anyBatteryDischarging = true; // Corriente negativa = descarga
  }

  // Control de LEDs según el modo de visualización actual
  if (displayMode == "Batteries") {
    // Modo baterías: mostrar estado de las baterías
    if (!allBatteriesOK) {
      // Estado de alarma: batería baja
      digitalWrite(LED_RED, HIGH);   // LED rojo encendido
      digitalWrite(LED_YELLOW, LOW); // LED amarillo apagado
      digitalWrite(LED_GREEN, LOW);  // LED verde apagado
      alarmActive = true;
    } else if (anyBatteryDischarging) {
      // Estado de advertencia: descarga de batería
      digitalWrite(LED_RED, LOW);    // LED rojo apagado
      digitalWrite(LED_YELLOW, HIGH);// LED amarillo encendido
      digitalWrite(LED_GREEN, LOW);  // LED verde apagado
      alarmActive = false;
    } else {
      // Estado normal
      digitalWrite(LED_RED, LOW);    // LED rojo apagado
      digitalWrite(LED_YELLOW, LOW); // LED amarillo apagado
      digitalWrite(LED_GREEN, HIGH); // LED verde encendido
      alarmActive = false;
    }
  } else if (displayMode == "Water") {
    // Modo agua: mostrar estado del tanque
    if (waterLevel > 0.7) {
      // Nivel suficiente
      digitalWrite(LED_RED, LOW);    // LED rojo apagado
      digitalWrite(LED_YELLOW, LOW); // LED amarillo apagado
      digitalWrite(LED_GREEN, HIGH); // LED verde encendido
      alarmActive = false;
    } else if (waterLevel > 0.3 && waterLevel <= 0.7) {
      // Nivel medio
      digitalWrite(LED_RED, LOW);    // LED rojo apagado
      digitalWrite(LED_YELLOW, HIGH);// LED amarillo encendido
      digitalWrite(LED_GREEN, LOW);  // LED verde apagado
      alarmActive = false;
    } else {
      // Nivel críticamente bajo - LED rojo parpadea
      static unsigned long lastBlinkTime = 0;
      if (millis() - lastBlinkTime >= 500) { // Parpadeo cada 500ms
        lastBlinkTime = millis();
        digitalWrite(LED_RED, !digitalRead(LED_RED)); // Cambiar estado
      }
      digitalWrite(LED_YELLOW, LOW); // LED amarillo apagado
      digitalWrite(LED_GREEN, LOW);  // LED verde apagado
      alarmActive = true;
    }
  }

  // Control del buzzer según el estado de la alarma
  if (alarmActive) tone(BUZZER_PIN, 1000); // Encender buzzer con tono de 1000Hz
  else noTone(BUZZER_PIN);                 // Apagar buzzer
}

// ========================= ENVÍO DE DATOS A SIGNALK =========================
// Función auxiliar para crear objetos JSON para SignalK
void createJsonObject(JsonArray values, const char* path, float value) {
  // Crea un objeto JSON con la ruta y el valor proporcionados
  JsonObject obj = values.createNestedObject();
  obj["path"] = path;   // Ruta del dato en la estructura SignalK
  obj["value"] = value; // Valor del dato
}

// Función principal para enviar todos los datos a SignalK
void sendAllData() {
  // Crear documento JSON con capacidad para 512 bytes
  StaticJsonDocument<512> doc;
  
  // Crear array de actualizaciones
  JsonArray updates = doc.createNestedArray("updates");
  JsonObject update = updates.createNestedObject();
  
  // Usamos millis() para el timestamp - SignalK lo sobrescribe según tu indicación
  update["timestamp"] = millis();
  
  // Crear array de valores a enviar
  JsonArray values = update.createNestedArray("values");

  // Leer y enviar datos de las 3 baterías
  float voltages[3];
  for (uint8_t i = 0; i < 3; i++) {
    // Leer voltaje de la batería
    voltages[i] = readBatteryVoltage(batteryPins[i], batteries[i].calibration);
    
    // Crear ruta para el voltaje de la batería
    String voltagePath = "electrical.batteries." + String(i) + ".voltage";
    // Crear ruta para el SOC (State of Charge) de la batería
    String socPath = "electrical.batteries." + String(i) + ".stateOfCharge";
    
    // Añadir voltaje al mensaje
    createJsonObject(values, voltagePath.c_str(), voltages[i]);
    // Añadir SOC al mensaje (normalizado a 0.0-1.0)
    createJsonObject(values, socPath.c_str(), calculateSOC(voltages[i], batteries[i].min_voltage, batteries[i].max_voltage) / 100.0);
  }

  // Leer y enviar nivel del tanque de agua
  float waterLevel = readWaterLevel();
  createJsonObject(values, "tanks.freshWater.0.currentLevel", waterLevel);

  // Leer y enviar consumo de corriente
  float current = readCurrent();
  createJsonObject(values, "electrical.batteries.0.current", current);

  // Leer temperatura y humedad
  float temperatureC, humidity;
  readEnvironment(temperatureC, humidity);
  
  // CORRECCIÓN INTENCIONAL para el DHT11 (error de 2°C)
  // Como mencionaste, usamos 271.15 en lugar de 273.15
  float temperatureK = temperatureC + 271.15;
  
  // Enviar temperatura y humedad
  createJsonObject(values, "environment.inside.temperature", temperatureK);
  createJsonObject(values, "environment.inside.relativeHumidity", humidity);

  // Actualizar LEDs y buzzer según los valores leídos
  updateLEDs(voltages, waterLevel);

  // Enviar datos JSON al puerto serial
  serializeJson(doc, Serial);
  Serial.println(); // Salto de línea necesario para el parser de SignalK
}

// ========================= GESTIÓN DEL BOTÓN =========================
// Función para manejar las acciones del botón
void handleButton() {
  // Si el botón está presionado (LOW porque INPUT_PULLUP)
  if (digitalRead(SWITCH_PIN) == LOW) {
    unsigned long currentTime = millis();
    // Si es la primera vez que se presiona
    if (buttonPressTime == 0) {
      buttonPressTime = currentTime;
    } 
    // Si se mantiene presionado durante 2 segundos
    else if (currentTime - buttonPressTime >= 2000) {
      // Cambiar entre modos
      displayMode = (displayMode == "Batteries") ? "Water" : "Batteries";
      // Enviar mensaje de confirmación por serial
      Serial.println(displayMode == "Batteries" ? "Modo Baterías Activado" : "Modo Agua Activado");
      buttonPressTime = 0; // Resetear temporizador
    }
  } 
  // Si el botón se suelta
  else {
    // Si se presionó brevemente (menos de 2 segundos)
    if (buttonPressTime > 0 && millis() - buttonPressTime < 2000) {
      // Restablecer alarma
      alarmActive = false;
      digitalWrite(LED_RED, LOW);
      noTone(BUZZER_PIN);
      Serial.println("Alarma Restablecida");
    }
    buttonPressTime = 0; // Resetear temporizador
  }
}

// ========================= SETUP =========================
// Función de inicialización (se ejecuta una vez al inicio)
void setup() {
  // Inicializar comunicación serial a 115200 baudios
  Serial.begin(38400);
  
  // Esperar brevemente para que se estabilice la conexión serial
  delay(100);
  
  // Inicializar sensor DHT
  dht.begin();
  
  // Configurar pines de salida para los LEDs y el buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  
  // Configurar pin del botón como entrada con resistencia pull-up
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // Mensaje de inicio con información de la placa
  Serial.print("..::: Nautical Arduino Monitor Ver: 4.0 (");
  Serial.print(BOARD_NAME);
  Serial.println("):::.. ");
  Serial.println("Sistema listo - Enviando datos a SignalK...");
}

// ========================= LOOP PRINCIPAL =========================
// Función principal del programa (se ejecuta continuamente)
void loop() {
  // Verificar si es hora de actualizar los datos
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    sendAllData();       // Enviar todos los datos a SignalK
    lastUpdate = millis(); // Actualizar el último momento de actualización
  }
  
  // Manejar eventos del botón
  handleButton();
}
