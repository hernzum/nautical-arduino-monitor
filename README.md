# **Nautical Arduino Monitor**  
📡 **Sistema de monitoreo para embarcaciones basado en Arduino y SignalK**  

---

## **📖 Descripción**  
**Nautical Arduino Monitor** es un sistema integral de monitoreo diseñado para **embarcaciones** que permite la supervisión de parámetros críticos en tiempo real, tales como:  
- **Estado de las baterías** (voltaje, estado de carga, capacidad restante).  
- **Condiciones ambientales** (temperatura y humedad).  
- **Nivel del tanque de agua dulce**.  
- **Consumo eléctrico mediante un shunt de corriente**.  
- **Detección de fugas de gas**.  
- **Alertas visuales y sonoras mediante LEDs y buzzer**.  

📡 **Conectividad:** El sistema envía los datos a **SignalK** a través de **puerto serial (UART, 115200 baud)**, permitiendo la visualización en sistemas de navegación y monitoreo marítimo.  

---

## **📦 Requisitos de Hardware**  

### **🔹 Microcontrolador Principal**  
- **Arduino Mini / Nano / Uno** (cualquier modelo compatible con UART y ADC).  

### **🔹 Sensores y Actuadores**  

| **Componente**            | **Descripción**                                     | **Conexión a Arduino** |
|---------------------------|-----------------------------------------------------|------------------------|
| **DHT11 / DHT22**         | Sensor de temperatura y humedad                     | Pin 2                  |
| **Sensores de Voltaje 0-25V** | Para medir voltajes de baterías (hasta 25V)   | A1, A2, A3             |
| **Shunt de corriente (Opcional)** | Medición de consumo de energía             | A4                     |
| **Sensor de Nivel de Agua** | Para medir el nivel del tanque de agua            | A0                     |
| **Sensor de Gas MQ-2/MQ-6** | Detección de fugas de gas                        | A5                     |
| **LED Verde**             | Estado Normal                                       | Pin 4                  |
| **LED Amarillo**          | Advertencia                                        | Pin 5                  |
| **LED Rojo**              | Alarma Crítica                                     | Pin 6                  |
| **Buzzer**               | Alertas sonoras                                    | Pin 7                  |
| **Pulsador**             | Cambio de modos / Reset de alarmas                 | Pin 3                  |

📌 **Nota:** En sistemas de 24V, se pueden usar **divisores de voltaje** en sensores de voltaje 0-25V.  

---

## **🖥️ Instalación del Software**  
### **1️⃣ Instalación de Arduino IDE**  
Descargar e instalar **Arduino IDE** (versión 1.8.19 o superior):  
🔗 [Descargar Arduino IDE](https://www.arduino.cc/en/software)  

### **2️⃣ Instalación de Librerías Necesarias**  
Desde **Arduino IDE**:  
1. Ir a **Sketch** → **Incluir Librería** → **Administrar Librerías**.  
2. Buscar e instalar las siguientes librerías:  

| **Librería**           | **Descripción**                                     | **Descarga** |
|------------------------|-----------------------------------------------------|--------------|
| **ArduinoJson**        | Serialización de datos en formato JSON             | [Descargar](https://github.com/bblanchon/ArduinoJson) |
| **DHT Sensor Library** | Comunicación con sensores DHT11/DHT22              | [Descargar](https://github.com/adafruit/DHT-sensor-library) |
| **Adafruit Unified Sensor** | Librería para sensores Adafruit              | [Descargar](https://github.com/adafruit/Adafruit_Sensor) |

### **3️⃣ Subir el Código a Arduino**  
1. **Abrir el archivo** `nautical_monitor.ino`.  
2. **Seleccionar la placa** (`Arduino Mini` o la correspondiente).  
3. **Configurar el puerto serial** a **115200 baud**.  
4. **Cargar el código en el microcontrolador**.  

---

## **⚙️ Conexión de Sensores**
### **🔹 1. Sensor DHT11/DHT22 (Temperatura y Humedad)**
| **DHT Pin** | **Arduino Mini** |
|------------|--------------|
| **VCC**    | **5V** |
| **GND**    | **GND** |
| **DATA**   | **Pin 2** |

📌 **DHT22 requiere una resistencia de 10kΩ entre VCC y DATA**.  

---

### **🔹 2. Sensores de Voltaje 0-25V (Monitoreo de Baterías)**
| **Sensor 0-25V** | **Arduino Mini** |
|--------------|-------------|
| **VCC**      | **5V** |
| **GND**      | **GND** |
| **OUT**      | **A1, A2, A3** |

📌 **Cada sensor 0-25V mide hasta 25V y entrega una salida de 0-5V**.  

---

### **🔹 3. Shunt de Corriente (Opcional)**
| **Shunt** | **Arduino Mini** |
|-----------|-------------|
| **VCC**   | **5V** |
| **GND**   | **GND** |
| **OUT**   | **A4** |

📌 Se recomienda usar un **amplificador INA219** para mayor precisión.  

---

### **🔹 4. Sensor de Nivel de Agua**
| **Sensor** | **Arduino Mini** |
|------------|-------------|
| **VCC**    | **5V** |
| **GND**    | **GND** |
| **OUT**    | **A0** |

📌 La medición se escala de **0-100%**.  

---

### **🔹 5. Sensor de Gas MQ-2/MQ-6**
| **Sensor** | **Arduino Mini** |
|-----------|-------------|
| **VCC**   | **5V** |
| **GND**   | **GND** |
| **OUT**   | **A5** |

📌 Se ajusta un **umbral de detección en el código**.  

---

## **📡 Configuración de SignalK**
1. **Instalar SignalK Server en ROCK 4C+**  
   ```sh
   curl -sSL https://get.signalk.org | sudo bash
   ```
2. **Conectar el Arduino Mini a ROCK 4C+** mediante USB/UART.  
3. **Configurar el puerto serial en SignalK**:  
   - **Abrir SignalK** (`http://localhost:3000`).  
   - Ir a `Data Connections` → `Add Connection`.  
   - **Seleccionar `Serial` y configurar:**  
     - **Device:** `/dev/ttyUSB0` (o el que corresponda).  
     - **Baud Rate:** `115200`.  
   - Guardar y reiniciar SignalK.  

---

## **📊 Datos Enviados a SignalK**
El sistema envía datos en **JSON**, estructurados así:

### **🔹 Datos de Baterías**
```json
{
  "updates": [{
    "source": { "label": "arduino-mini", "type": "sensor" },
    "timestamp": 12345678,
    "values": [
      { "path": "electrical.batteries.0.voltage", "value": 12.5 },
      { "path": "electrical.batteries.0.stateOfCharge", "value": 0.8 }
    ]
  }]
}
```

### **🔹 Datos Ambientales**
```json
{
  "updates": [{
    "source": { "label": "arduino-mini", "type": "sensor" },
    "timestamp": 12345678,
    "values": [
      { "path": "environment.inside.temperature", "value": 293.15 },
      { "path": "environment.inside.relativeHumidity", "value": 0.55 }
    ]
  }]
}
```

📌 **Los datos se envían cada `5s`, con pausas de `100ms` entre bloques.**  

---

## **📬 Contacto y Soporte**
Para dudas, mejoras o reportes de errores, abrir un **issue en GitHub** o contactar a **Hernzum**.  
