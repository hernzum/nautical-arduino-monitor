Nautical Arduino Monitor
Autor: Hernzum
Versión: 1.0
Plataforma: Arduino Mini + ROCK 4C+

📌 Descripción
Nautical Arduino Monitor es un sistema de monitoreo para embarcaciones que permite medir y enviar información en tiempo real sobre: ✅ Baterías (voltaje, estado de carga, capacidad restante).
✅ Temperatura y humedad ambiente (DHT11).
✅ Nivel del tanque de agua dulce.
✅ Consumo eléctrico (Shunt de corriente).
✅ Fugas de gas.
✅ Alertas con LEDs y buzzer.
✅ Envío de datos a SignalK en formato JSON.

📡 Conectividad: Se comunica con SignalK mediante puerto serial (UART) a 115200 baud.

📦 Componentes y Requisitos
Para montar el sistema, necesitas los siguientes componentes:

Componente	Descripción
Arduino Mini o similar	Microcontrolador principal
Rock 4C+	Recibe datos y los envía a SignalK
DHT11 o DHT22	Sensor de temperatura y humedad
Sensores 0-25V (x3)	Para monitoreo de voltajes de baterías
Shunt de corriente (opcional)	Para medir consumo de energía
Sensor de nivel de agua	Para medir el nivel del tanque de agua
Sensor de gas MQ-2/MQ-6	Para detección de fugas de gas
LEDs y buzzer	Para alertas visuales y sonoras
Pulsador	Para cambiar modos y resetear alarmas
📥 Instalación del Software
1️⃣ Instalar Arduino IDE
Descarga e instala Arduino IDE (versión 1.8.19 o superior):
🔗 Descargar Arduino IDE

2️⃣ Instalar Librerías Necesarias
En Arduino IDE, ve a: Sketch → Incluir Librería → Administrar Librerías

🔹 Busca e instala las siguientes librerías:

📌 ArduinoJson → Descargar
📌 DHT Sensor Library → Descargar
📌 Adafruit Unified Sensor → Descargar
⚡ Conexión de los Sensores
Aquí se detallan las conexiones necesarias en el Arduino Mini:

1️⃣ Conexión del DHT11/DHT22 (Temperatura y Humedad)
DHT11 Pin	Arduino Mini
VCC	5V
GND	GND
DATA	Pin 2
📌 Nota: Si usas DHT22, usa una resistencia pull-up de 10kΩ entre VCC y DATA.

2️⃣ Conexión de Sensores de Voltaje 0-25V (Monitoreo de Baterías)
Sensor 0-25V	Arduino Mini
VCC	5V
GND	GND
OUT	A1, A2, A3 (1 por cada batería)
📌 Importante: Cada sensor 0-25V mide hasta 25V y entrega una salida de 0-5V.
Se conectan tres sensores en A1, A2 y A3 para monitorear tres baterías.

3️⃣ Conexión del Shunt de Corriente (Opcional)
Para medir consumo de energía, conecta un shunt en el negativo de la batería.
El shunt genera una caída de voltaje proporcional a la corriente.

Shunt Pin	Arduino Mini
VCC	5V
GND	GND
OUT	A4
📌 Nota: Se requiere amplificador INA219 o similar si el shunt no tiene salida 0-5V.

4️⃣ Conexión del Sensor de Nivel de Agua
Sensor Agua	Arduino Mini
VCC	5V
GND	GND
OUT	A0
📌 Nota: Los valores analógicos se escalan a 0-100% de llenado del tanque.

5️⃣ Conexión del Sensor de Gas MQ-2/MQ-6
Sensor Gas	Arduino Mini
VCC	5V
GND	GND
OUT	A5
📌 Nota: Se define un umbral de fuga de gas para activar la alarma.

6️⃣ Conexión de LEDs, Buzzer y Pulsador
Componente	Arduino Mini
LED Verde	Pin 4
LED Amarillo	Pin 5
LED Rojo	Pin 6
Buzzer	Pin 7
Pulsador	Pin 3
📌 Nota: Los LEDs indican el estado de los sensores, y el buzzer suena en alarmas críticas.

🔧 Configuración y Calibración
1️⃣ Calibrar Sensores de Voltaje 0-25V
Usa un multímetro para medir el voltaje real de la batería.
Compara con el valor leído por el sensor.
Ajusta la conversión en el código si hay diferencias:
cpp
Copy
Edit
float voltage = (analogRead(A1) * 5.0) / 1023.0 * 5.0;  // 0-25V
2️⃣ Calibrar el Shunt de Corriente
Consulta el factor de calibración del shunt (Ejemplo: 75mV para 100A).
Ajusta la conversión en el código si es necesario.
3️⃣ Calibrar el Sensor de Gas
Exponlo a gas en concentración conocida y ajusta el umbral en el código:
cpp
Copy
Edit
const float GAS_THRESHOLD = 400;  // Ajustar según necesidad
4️⃣ Ajustar Límites de Voltaje de las Baterías
En el código, puedes modificar los valores mínimos y máximos:

cpp
Copy
Edit
BatteryConfig batteries[3] = {
  {11.8, 12.6, 100},  // Plomo-Ácido
  {12.0, 14.6, 80},   // Litio
  {10.5, 12.5, 60}    // Otro tipo
};
🚀 Subir el Código a GitHub
Para subir el código del proyecto:

sh
Copy
Edit
git add .
git commit -m "Añadiendo código del sketch"
git push -u origin main
📬 Contacto y Soporte
Si tienes dudas o mejoras, abre un issue en GitHub o contacta a Hernzum.

🚢



