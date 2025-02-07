# **📜 README - Nautical Arduino Monitor**  

## **📌 Descripción del Proyecto**
**Nautical Arduino Monitor** es un sistema de monitoreo basado en **Arduino Mini** diseñado para **embarcarse en sistemas eléctricos de barcos**.  
Este monitor permite **medir y reportar** los siguientes parámetros en **SignalK** en **formato JSON**:  
✅ **Estado de baterías (Voltaje, Estado de Carga - SOC, Capacidad restante)**  
✅ **Temperatura y humedad ambiental**  
✅ **Nivel del tanque de agua dulce**  
✅ **Consumo de corriente mediante un shunt de 300A/75mV**  
✅ **Alarmas visuales y sonoras en caso de fallas críticas**  
✅ **Pulsador para cambiar modos de operación y resetear alarmas**  

---

## **📌 Requerimientos de Hardware**
🔹 **Microcontrolador:** Arduino Mini (3.3V)  
🔹 **Sensores y componentes:**
  - **Divisores de voltaje** (Resistencias **680kΩ y 80kΩ**) para medición de baterías  
  - **Sensor de temperatura y humedad** (**DHT11**)  
  - **Sensor de nivel de agua** (**Sensor resistivo conectado a A0**)  
  - **Shunt de 300A/75mV** para medición de corriente  
  - **Buzzer** para alarmas sonoras  
  - **LEDs indicadores** (Verde, Amarillo, Rojo)  
  - **Pulsador** para reset de alarmas y cambio de modo  

---

## **📌 Conexión de los Componentes**
### **🔋 Conexión del Divisor de Voltaje para Baterías**
Cada batería debe conectarse a su propio **divisor de voltaje** antes de ingresar al Arduino.  
| **Batería** | **Arduino Mini** | **Resistencias (Divisor)** |
|------------|----------------|--------------------------|
| **Batería 1** | **A1** | R1 = 680kΩ, R2 = 80kΩ |
| **Batería 2** | **A2** | R1 = 680kΩ, R2 = 80kΩ |
| **Batería 3** | **A3** | R1 = 680kΩ, R2 = 80kΩ |

📌 **Importante:** Usa **resistencias de precisión (1% o menos)** para evitar errores en la medición.

### **💧 Conexión del Sensor de Nivel de Agua**
- **Salida del sensor** → **A0 (Arduino Mini)**  
- **GND** → **GND (Arduino Mini)**  
- **VCC (5V)** → **Alimentación del sensor**  

### **⚡ Conexión del Shunt de 300A/75mV**
| **Shunt** | **Arduino Mini** |
|-----------|-----------------|
| **V+ (Lado de la batería)** | **A4 (Medición de voltaje)** |
| **V- (Carga del sistema)** | **GND (Arduino Mini)** |

📌 **Nota:**  
- Si el valor de corriente es **negativo**, significa que la batería **se está cargando**.  
- Si el valor de corriente es **positivo**, significa que la batería **se está descargando**.  

---

## **📌 Cálibración de Sensores**
### **🔋 Cálibración de Baterías**
Cada batería tiene un **factor de calibración individual** para corregir errores en la medición.  

1️⃣ **Mide el voltaje real de cada batería con un multímetro (tester).**  
2️⃣ **Observa el voltaje que muestra el Arduino en el Monitor Serie.**  
3️⃣ **Calcula el Factor de Calibración:**  
   \[
   \text{CALIBRATION_FACTOR} = \frac{\text{Voltaje real (multímetro)}}{\text{Voltaje Arduino}}
   \]
4️⃣ **Modifica los valores en el código:**  
```cpp
BatteryConfig batteries[3] = {
  {BATTERY_1_PIN, 11.8, 12.6, 1.02},  // Batería 1 (Plomo-Ácido)
  {BATTERY_2_PIN, 12.0, 14.6, 1.01},  // Batería 2 (Litio)
  {BATTERY_3_PIN, 10.5, 12.5, 1.03}   // Batería 3 (AGM)
};
```
5️⃣ **Carga el código y verifica si los valores coinciden.**  

### **⚡ Cálibración del Shunt**
1️⃣ **Usa un amperímetro externo para medir la corriente real.**  
2️⃣ **Observa la corriente mostrada en el Monitor Serie del Arduino.**  
3️⃣ **Calcula el Factor de Calibración:**  
   \[
   \text{SHUNT\_CALIBRATION} = \frac{\text{Corriente real (amperímetro)}}{\text{Corriente Arduino}}
   \]
4️⃣ **Modifica este valor en el código:**  
```cpp
const float SHUNT_CALIBRATION = 1.0;
```
5️⃣ **Recarga el código y prueba nuevamente.**  

---

## **📌 Formato de Datos Enviados a SignalK**
Los datos se envían en **formato JSON** a través del puerto **Serial a 115200 baud**.  
### **Ejemplo de Datos Enviados**
```json
{
  "updates": [
    {
      "source": {
        "label": "arduino-mini",
        "type": "sensor"
      },
      "timestamp": 1678901234,
      "values": [
        {"path": "electrical.batteries.0.voltage", "value": 12.4},
        {"path": "electrical.batteries.0.stateOfCharge", "value": 0.85},
        {"path": "electrical.batteries.0.current", "value": 25.3},
        {"path": "tanks.freshWater.0.currentLevel", "value": 0.65},
        {"path": "environment.inside.temperature", "value": 298.15},
        {"path": "environment.inside.relativeHumidity", "value": 0.55}
      ]
    }
  ]
}
```
📌 **Explicación:**  
- `electrical.batteries.0.voltage` → Voltaje de la batería 1  
- `electrical.batteries.0.stateOfCharge` → Estado de carga (SOC)  
- `electrical.batteries.0.current` → Corriente en la batería 1 (A)  
- `tanks.freshWater.0.currentLevel` → Nivel del tanque de agua (fracción 0-1)  
- `environment.inside.temperature` → Temperatura en Kelvin  
- `environment.inside.relativeHumidity` → Humedad relativa (0-1)  

---

## **📌 Instalación y Uso**
1️⃣ **Instala las librerías necesarias en el IDE de Arduino:**  
```cpp
#include <DHT.h>
#include <ArduinoJson.h>
```
2️⃣ **Configura la conexión con SignalK a través del puerto serie:**  
- Usa **115200 baud** en la configuración de entrada de SignalK.  
- Asegúrate de que el Arduino Mini está correctamente conectado a la placa **Rock 4C+**.  

3️⃣ **Carga el código en el Arduino Mini.**  
4️⃣ **Abre el Monitor Serie y verifica las mediciones.**  
5️⃣ **Conéctalo a SignalK y observa los datos en la interfaz web.**  

---

## **📌 Mejoras Implementadas en Esta Versión**
✅ **Cada batería ahora tiene su propio voltaje máximo y mínimo** para calcular el SOC correctamente.  
✅ **Ahora el código mide el consumo de corriente usando un shunt de 300A/75mV.**  
✅ **Los datos de agua, temperatura y humedad ahora se envían correctamente a SignalK.**  
✅ **El código es más modular y fácil de modificar.**  
✅ **Se agregaron factores de calibración para baterías y shunt.**  

📌 **👉 Ahora puedes probar el código en tu embarcación y verificar que SignalK reciba todos los datos correctamente.** 🚢⚡  

---
### **📌 Autor**
**Desarrollado por:** Hernzum 🚢  
**Versión:** 1.5 (Última actualización)  
**Licencia:** Open Source 🚀
