// Conexión al servidor SignalK
const signalKUrl = 'ws://localhost:3000/signalk/v1/stream?subscribe=none';
const ws = new WebSocket(signalKUrl);

// Cache DOM elements to avoid repeated queries
const domElements = {};

function cacheElement(id) {
  if (!domElements[id]) {
    domElements[id] = document.getElementById(id);
  }
  return domElements[id];
}

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

// Use requestAnimationFrame for batched DOM updates
let pendingUpdates = {};
let updateScheduled = false;

function scheduleUpdate() {
  if (!updateScheduled) {
    updateScheduled = true;
    requestAnimationFrame(() => {
      Object.entries(pendingUpdates).forEach(([id, value]) => {
        const el = cacheElement(id);
        if (el) el.textContent = value;
      });
      pendingUpdates = {};
      updateScheduled = false;
    });
  }
}

ws.onmessage = (event) => {
  try {
    const data = JSON.parse(event.data);
    if (data.updates) {
      data.updates.forEach(update => {
        update.values.forEach(value => {
          const path = value.path;
          const val = value.value;

          if (path === "electrical.batteries.0.voltage") {
            pendingUpdates["voltage0"] = val.toFixed(2);
          }
          if (path === "electrical.batteries.0.stateOfCharge") {
            pendingUpdates["soc0"] = (val * 100).toFixed(1);
          }
          if (path === "tanks.freshWater.0.currentLevel") {
            pendingUpdates["water"] = (val * 100).toFixed(1);
          }
          if (path === "environment.inside.temperature") {
            pendingUpdates["temp"] = (val - 273.15).toFixed(1);
          }
          if (path === "environment.inside.relativeHumidity") {
            pendingUpdates["humidity"] = (val * 100).toFixed(1);
          }
          if (path === "electrical.batteries.0.current") {
            pendingUpdates["current"] = val.toFixed(2);
          }
        });
      });
      scheduleUpdate();
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
