A continuación, te proporciono una versión optimizada del **README** original. He mejorado la estructura, eliminado redundancias y reorganizado la información para que sea más clara y profesional, manteniendo todos los detalles importantes.

---

### **README - Nautical Arduino Monitor**

#### **📌 Descripción del Proyecto**
El **Nautical Arduino Monitor** es un sistema de monitoreo basado en **Arduino Mini**, diseñado específicamente para sistemas eléctricos en embarcaciones. Este monitor permite medir y reportar los siguientes parámetros clave:

✅ **Estado de baterías**: Voltaje, Estado de Carga (SOC) y capacidad restante.  
✅ **Condiciones ambientales**: Temperatura y humedad.  
✅ **Nivel del tanque de agua dulce**.  
✅ **Consumo de corriente**: Medido mediante un shunt de 300A/75mV.  
✅ **Alarmas visuales y sonoras**: Indican fallas críticas como fugas de gas o voltajes bajos.  
✅ **Control manual**: Pulsador para resetear alarmas y cambiar modos de operación.  

Los datos se envían en formato JSON a través del puerto serial para su integración con **SignalK**.

---

### **📌 Requerimientos de Hardware**
- **Microcontrolador**: Arduino Mini (3.3V).  
- **Sensores y componentes**:  
  - Divisores de voltaje (resistencias **680kΩ y 80kΩ**) para medición de baterías.  
  - Sensor de temperatura y humedad (**DHT11**).  
  - Sensor resistivo para nivel de agua dulce.  
  - Shunt de 300A/75mV para medición de corriente.  
  - Buzzer para alarmas sonoras.  
  - LEDs indicadores (Verde, Amarillo, Rojo).  
  - Pulsador para control manual.  

📌 **Nota**: Utiliza resistencias de precisión (1% o menos) para evitar errores en las mediciones.

---

### **📌 Conexión de Componentes**

#### **🔋 Divisor de Voltaje para Baterías**
Cada batería debe conectarse a su propio divisor de voltaje antes de ingresar al Arduino Mini:
| **Batería** | **Pin Arduino** | **Resistencias (Divisor)** |
|-------------|----------------|---------------------------|
| Batería 1    | A1             | R1 = 680kΩ, R2 = 80kΩ    |
| Batería 2    | A2             | R1 = 680kΩ, R2 = 80kΩ    |
| Batería 3    | A3             | R1 = 680kΩ, R2 = 80kΩ    |

📌 **Importante**: Usa divisores de voltaje para reducir los voltajes altos a niveles seguros (<3.3V).

#### **💧 Sensor de Nivel de Agua**
- **Salida del sensor** → **A0 (Arduino Mini)**.  
- **GND** → **GND (Arduino Mini)**.  
- **VCC (5V)** → **Alimentación del sensor**.

#### **⚡ Shunt de 300A/75mV**
| **Shunt**       | **Pin Arduino** |
|-----------------|----------------|
| V+ (Lado de la batería) | A4            |
| V- (Carga del sistema) | GND           |

📌 **Nota**: Si el valor de corriente es **negativo**, indica carga; si es **positivo**, indica descarga.

---

### **📌 Calibración de Sensores**

#### **🔋 Calibración de Baterías**
1. Mide el voltaje real de cada batería con un multímetro.  
2. Observa el voltaje mostrado en el Monitor Serie del Arduino.  
3. Calcula el Factor de Calibración:  
   \[
   \text{CALIBRATION_FACTOR} = \frac{\text{Voltaje real (multímetro)}}{\text{Voltaje Arduino}}
   \]
4. Modifica los valores en el código:  
   ```cpp
   BatteryConfig batteries[3] = {
     {BATTERY_1_PIN, 11.8, 12.6, 1.02}, // Batería 1 (Plomo-Ácido)
     {BATTERY_2_PIN, 12.0, 14.6, 1.01}, // Batería 2 (Litio)
     {BATTERY_3_PIN, 10.5, 12.5, 1.03}  // Batería 3 (AGM)
   };
   ```
5. Verifica que los valores coincidan después de cargar el código.

#### **⚡ Calibración del Shunt**
1. Usa un amperímetro externo para medir la corriente real.  
2. Observa la corriente mostrada en el Monitor Serie del Arduino.  
3. Calcula el Factor de Calibración:  
   \[
   \text{SHUNT_CALIBRATION} = \frac{\text{Corriente real (amperímetro)}}{\text{Corriente Arduino}}
   \]
4. Modifica este valor en el código:  
   ```cpp
   const float SHUNT_CALIBRATION = 1.0;
   ```

---

### **📌 Formato de Datos Enviados a SignalK**
Los datos se envían en formato JSON a través del puerto serial a 115200 baudios.

#### **Ejemplo de Datos Enviados**
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

#### **Explicación de los Campos**
- `electrical.batteries.0.voltage`: Voltaje de la batería 1.  
- `electrical.batteries.0.stateOfCharge`: Estado de carga (SOC) de la batería 1.  
- `electrical.batteries.0.current`: Corriente en la batería 1 (A).  
- `tanks.freshWater.0.currentLevel`: Nivel del tanque de agua (fracción 0-1).  
- `environment.inside.temperature`: Temperatura en Kelvin.  
- `environment.inside.relativeHumidity`: Humedad relativa (fracción 0-1).

---

### **📌 Instalación y Uso**
1. **Instala las librerías necesarias** en el IDE de Arduino:  
   ```cpp
   #include <DHT.h>
   #include <ArduinoJson.h>
   ```
2. **Configura la conexión con SignalK**:  
   - Usa **115200 baud** en la configuración de entrada de SignalK.  
   - Conecta el Arduino Mini a la placa **Rock 4C+**.  
3. **Carga el código en el Arduino Mini**.  
4. **Abre el Monitor Serie** y verifica las mediciones.  
5. **Conéctalo a SignalK** y observa los datos en la interfaz web.

---

### **📌 Mejoras Implementadas**
- Cada batería tiene su propio voltaje máximo y mínimo para calcular el SOC correctamente.  
- Se agregó medición de corriente usando un shunt de 300A/75mV.  
- Los datos de agua, temperatura y humedad ahora se envían correctamente a SignalK.  
- El código es más modular y fácil de modificar.  
- Se incluyeron factores de calibración para baterías y shunt.

📌 **Prueba el código en tu embarcación y verifica que SignalK reciba todos los datos correctamente.**

---

### **📌 Autor**
**Desarrollado por:** Hernzum 🚢  
**Versión:** 1.5 (Última actualización)  
**Licencia:** Open Source 🚀  

Si tienes preguntas o sugerencias, no dudes en contactarme:  
📧 Email: [hernzum@gmail.com](mailto:hernzum@gmail.com)  
🌐 GitHub: [https://github.com/hernzum/nautical-arduino-monitor](https://github.com/hernzum/nautical-arduino-monitor)

--- 

Esperamos que este proyecto sea útil para tu aplicación náutica. ¡Gracias por usar el **Nautical Arduino Monitor**! 🚤
