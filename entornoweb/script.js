// Conexión al servidor SignalK
const signalKUrl = 'ws://localhost:3000/signalk/v1/stream?subscribe=none';
const ws = new WebSocket(signalKUrl);

ws.onopen = () => {
  console.log("✅ Conectado a SignalK");
  ws.send(JSON.stringify({
    "context": "vessels.self",
    "subscribe": [
      { "path": "electrical.batteries.0.voltage", "format": "delta", "minPeriod": 1000 },
      { "path": "electrical.batteries.0.stateOfCharge", "format": "delta", "minPeriod": 1000 },
      { "path": "tanks.freshWater.0.currentLevel", "format": "delta", "minPeriod": 1000 },
      { "path": "electrical.batteries.0.current", "format": "delta", "minPeriod": 1000 },
      { "path": "environment.inside.temperature", "format": "delta", "minPeriod": 1000 },
      { "path": "environment.inside.relativeHumidity", "format": "delta", "minPeriod": 1000 }
    ]
  }));
};

ws.onmessage = (event) => {
  try {
    const data = JSON.parse(event.data);
    if (data.updates) {
      data.updates.forEach(update => {
        update.values.forEach(value => {
          const path = value.path;
          const val = value.value;

          if (path === "electrical.batteries.0.voltage") {
            document.getElementById("voltage0").textContent = val.toFixed(2);
          }
          if (path === "electrical.batteries.0.stateOfCharge") {
            document.getElementById("soc0").textContent = (val * 100).toFixed(1);
          }
          if (path === "tanks.freshWater.0.currentLevel") {
            document.getElementById("water").textContent = (val * 100).toFixed(1);
          }
          if (path === "environment.inside.temperature") {
            document.getElementById("temp").textContent = (val - 273.15).toFixed(1);
          }
          if (path === "environment.inside.relativeHumidity") {
            document.getElementById("humidity").textContent = (val * 100).toFixed(1);
          }
          if (path === "electrical.batteries.0.current") {
            document.getElementById("current").textContent = val.toFixed(2);
          }
        });
      });
    }
  } catch (e) {
    console.error("❌ Error:", e);
  }
};

ws.onerror = (err) => console.error("❌ Error SignalK:", err);
ws.onclose = () => {
  console.log("🔌 Desconectado. Reintentando...");
  setTimeout(() => location.reload(), 5000);
};
