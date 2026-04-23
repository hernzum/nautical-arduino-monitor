# 🚢 Nautical Arduino Monitor

Sistema completo de monitoreo náutico basado en Arduino que permite supervisar baterías, nivel de agua, temperatura, humedad y consumo de corriente en embarcaciones. Los datos se integran con SignalK™ y pueden visualizarse mediante una interfaz web o recibir alertas a través de Telegram.

## 📋 Índice

- [Características](#-características)
- [Componentes del Sistema](#-componentes-del-sistema)
- [Requisitos de Hardware](#-requisitos-de-hardware)
- [Instalación](#-instalación)
- [Configuración](#-configuración)
- [Uso](#-uso)
- [Estructura del Proyecto](#-estructura-del-proyecto)
- [Licencia](#-licencia)

## ✨ Características

- **Monitoreo de 3 Baterías**: Voltaje y Estado de Carga (SoC) para cada batería
- **Sensor de Nivel de Agua**: Tanque de agua dulce con alertas de nivel bajo
- **Sensores Ambientales**: Temperatura y humedad interior
- **Medición de Corriente**: Consumo eléctrico en tiempo real usando shunt
- **Interfaz Web**: Dashboard moderno para visualizar todos los datos
- **Bot de Telegram**: Alertas automáticas cuando los valores críticos bajan de umbrales configurados
- **Integración SignalK™**: Compatible con el estándar de datos náuticos SignalK™
- **LEDs de Estado**: Indicadores visuales (verde, amarillo, rojo) para alertas
- **Buzzer**: Alarma sonora para condiciones críticas

## 🧩 Componentes del Sistema

### 1. Firmware (`firmware/`)
Código Arduino que lee sensores y envía datos a SignalK™:
- Lectura de voltaje de 3 baterías con divisor de voltaje
- Sensor DHT11 para temperatura y humedad
- Sensor analógico de nivel de agua
- Shunt de corriente (300A/75mV)
- LEDs indicadores y buzzer de alarma
- Botón para cambiar modos de visualización

### 2. Interfaz Web (`entornoweb/`)
Dashboard HTML/CSS/JavaScript que se conecta a SignalK™ vía WebSocket:
- Visualización en tiempo real de todos los sensores
- Diseño responsivo para móviles y desktop
- Actualización automática de datos

### 3. Bot de Telegram (`telebrambot/`)
Bot Node.js que monitorea SignalK™ y envía alertas:
- Alertas de batería baja
- Alertas de nivel de agua bajo
- Configuración de umbrales personalizables
- Rate limiting para evitar spam de notificaciones

## 🔧 Requisitos de Hardware

### Placa Arduino (una de las siguientes):
- Arduino Uno / Nano (5V)
- Arduino Mega (5V)
- Arduino Mini Pro (3.3V o 5V)
- ESP8266 (NodeMCU, Wemos D1)
- ESP32

### Sensores y Componentes:
- **Sensor DHT11**: Temperatura y humedad
- **Sensor de nivel de agua**: Analógico
- **Shunt de corriente**: 300A/75mV (0.00025Ω)
- **3 divisores de voltaje**: Para medir baterías de 12V (resistencias 680kΩ + 80kΩ)
- **LEDs**: Verde (pin 4), Amarillo (pin 5), Rojo (pin 6)
- **Buzzer activo**: Pin 7
- **Botón pulsador**: Pin 3
- **Resistencias y cables** según necesite tu circuito

### Software:
- Arduino IDE con librerías:
  - `DHT sensor library`
  - `ArduinoJson`
- Node.js (para el bot de Telegram)
- Servidor SignalK™ instalado en tu embarcación

## 📦 Instalación

### 1. Firmware Arduino

1. Abre el archivo `firmware/nautical-Monitor-final4.ino` en Arduino IDE
2. Selecciona tu placa en la sección de configuración:
```cpp
#define ARDUINO_UNO           // Para Arduino Uno/Nano
// #define ARDUINO_MEGA        // Para Arduino Mega
// #define ARDUINO_MINI_PRO_3V3 // Para Mini Pro 3.3V
// #define ESP8266             // Para NodeMCU/Wemos
// #define ESP32               // Para ESP32
```
3. Ajusta los pines según tu cableado
4. Configura los parámetros de tus baterías en `batteries[]`
5. Instala las librerías requeridas desde el Gestor de Librerías
6. Compila y sube a tu placa Arduino

### 2. Interfaz Web

La interfaz web requiere un servidor HTTP simple:

```bash
cd entornoweb
# Opción 1: Python
python3 -m http.server 8080

# Opción 2: Node.js (necesita http-server)
npm install -g http-server
http-server -p 8080
```

Luego abre `http://localhost:8080` en tu navegador.

### 3. Bot de Telegram

```bash
cd telebrambot

# Instalar dependencias
npm install

# Configurar el bot (ver sección Configuración)
nano config.json

# Ejecutar el bot
node bot.js
```

## ⚙️ Configuración

### Configurar SignalK™

El firmware envía datos en formato SignalK™. Asegúrate de tener SignalK™ corriendo en tu sistema. Los paths utilizados son:

- `electrical.batteries.0.voltage` - Voltaje batería 1
- `electrical.batteries.0.stateOfCharge` - Estado de carga
- `tanks.freshWater.0.currentLevel` - Nivel de agua (0-1)
- `environment.inside.temperature` - Temperatura (Kelvin)
- `environment.inside.relativeHumidity` - Humedad (0-1)
- `electrical.batteries.0.current` - Corriente (Amperios)

### Configurar Bot de Telegram

Edita `telebrambot/config.json`:

```json
{
  "signalkServer": "localhost",      // IP de tu servidor SignalK
  "signalkPort": 3000,               // Puerto SignalK (default: 3000)
  "telegramToken": "TU_TOKEN_AQUI",  // Token de @BotFather
  "chatId": "TU_CHAT_ID_AQUI",       // Tu chat ID (usa @userinfobot)
  "alarms": {
    "lowBattery": 12.0,              // Voltaje mínimo para alerta
    "lowWater": 0.3                  // Nivel mínimo (30%) para alerta
  }
}
```

**Obtener tu Chat ID:**
1. Inicia una conversación con @BotFather en Telegram
2. Crea un nuevo bot y obtén el token
3. Envía un mensaje a tu nuevo bot
4. Consulta @userinfobot para obtener tu chat ID

### Calibración de Sensores

En el firmware Arduino, ajusta estos valores según tus componentes:

```cpp
// Divisor de voltaje (ej: 680kΩ + 80kΩ)
const float VOLTAGE_DIVIDER_RATIO = (680000.0 + 80000.0) / 80000.0;

// Shunt de corriente
const float SHUNT_RESISTANCE = 0.00025; // 300A/75mV
const float SHUNT_CALIBRATION = 1.024;   // Ajustar según medición real

// Calibración individual por batería
const BatteryConfig batteries[3] = {
  {11.8, 12.6, 0.961}, // minV, maxV, calibración
  {12.0, 14.6, 0.961},
  {10.5, 12.5, 0.961},
};
```

## 🚀 Uso

1. **Enciende el sistema**: Conecta la alimentación al Arduino
2. **Verifica los LEDs**: 
   - 🟢 Verde = Todo normal
   - 🟡 Amarillo = Advertencia
   - 🔴 Rojo = Alarma crítica
3. **Accede al dashboard**: Abre la interfaz web en tu navegador
4. **Recibe alertas**: El bot de Telegram te notificará automáticamente

### Comandos Útiles

```bash
# Ver logs del bot
tail -f ~/.github-check.log

# Subir cambios a GitHub (editar script primero con tu token)
./subir-github.sh
```

## 📁 Estructura del Proyecto

```
nautical-arduino-monitor/
├── firmware/
│   └── nautical-Monitor-final4.ino    # Código Arduino principal
├── entornoweb/
│   ├── index.html                      # Interfaz web HTML
│   ├── style.css                       # Estilos CSS
│   └── script.js                       # Lógica WebSocket
├── telebrambot/
│   ├── bot.js                          # Bot de Telegram
│   ├── config.json                     # Configuración del bot
│   ├── package.json                    # Dependencias Node.js
│   └── node_modules/                   # Librerías instaladas
├── subir-github.sh                     # Script de deploy a GitHub
└── README.md                           # Este archivo
```

## 🔐 Scripts de Utilidad

### subir-github.sh
Script automatizado para verificar y subir cambios a GitHub:
- Valida tu Personal Access Token
- Verifica permisos de escritura
- Corrige rama master → main automáticamente
- Hace commit y push de cambios

**Configuración requerida:**
```bash
# Editar el script con:
# - GITHUB_TOKEN: Tu PAT de GitHub
# - PROJECT_DIR: Ruta a tu proyecto local
```

## ⚠️ Consideraciones Importantes

- **Voltaje de alimentación**: Asegúrate de seleccionar la placa correcta en el firmware (3.3V vs 5V)
- **Calibración**: Mide con multímetro y ajusta los factores de calibración para lecturas precisas
- **SeñalK™**: El servidor debe estar accesible desde la red donde corre el bot
- **Telegram Token**: Nunca compartas tu token públicamente

## 🤝 Contribuciones

Las contribuciones son bienvenidas. Por favor:
1. Fork el repositorio
2. Crea una rama para tu feature
3. Haz commit de tus cambios
4. Push a la rama
5. Abre un Pull Request

## 📄 Licencia

Este proyecto está bajo la licencia MIT. Ver el archivo LICENSE para más detalles.

## 👨‍💻 Autor

Creado por @hernzum

**Repositorio**: https://github.com/hernzum/nautical-arduino-monitor

---

¡⚓ Buen viento y buena mar! 🌊
