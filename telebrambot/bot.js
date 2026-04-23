const WebSocket = require('ws');
const TelegramBot = require('node-telegram-bot-api');

const config = require('./config.json');

const bot = new TelegramBot(config.telegramToken, { polling: true });

const skWs = new WebSocket(`ws://${config.signalkServer}:${config.signalkPort}/signalk/v1/stream`);

// Debounce helper to prevent rapid reconnections
let reconnectTimeout = null;

function connectSignalK() {
  skWs.send(JSON.stringify({
    "context": "vessels.self",
    "subscribe": [
      { "path": "electrical.batteries.0.voltage", "format": "delta", "minPeriod": 2000 },
      { "path": "tanks.freshWater.0.currentLevel", "format": "delta", "minPeriod": 2000 }
    ]
  }));
}

skWs.on('open', () => {
  console.log('✅ Conectado a SignalK');
  connectSignalK();
});

let batteryAlertSent = false;
let waterAlertSent = false;

// Use Map for faster path lookups
const alertHandlers = new Map([
  ["electrical.batteries.0.voltage", (val) => {
    if (val < config.alarms.lowBattery && !batteryAlertSent) {
      bot.sendMessage(config.chatId, `🚨 Batería baja: ${val.toFixed(2)} V`);
      batteryAlertSent = true;
      setTimeout(() => { batteryAlertSent = false; }, 3600000);
    }
  }],
  ["tanks.freshWater.0.currentLevel", (val) => {
    if (val < config.alarms.lowWater && !waterAlertSent) {
      bot.sendMessage(config.chatId, `💧 Agua baja: ${(val*100).toFixed(1)}%`);
      waterAlertSent = true;
      setTimeout(() => { waterAlertSent = false; }, 3600000);
    }
  }]
]);

skWs.on('message', (data) => {
  try {
    const msg = JSON.parse(data);
    if (msg.updates) {
      for (const update of msg.updates) {
        for (const value of update.values) {
          const handler = alertHandlers.get(value.path);
          if (handler) {
            handler(value.value);
          }
        }
      }
    }
  } catch (e) { console.error(e); }
});

skWs.on('error', (err) => console.error('❌ Error SignalK:', err));
skWs.on('close', () => {
  if (!reconnectTimeout) {
    reconnectTimeout = setTimeout(() => {
      reconnectTimeout = null;
      process.exit(0);
    }, 5000);
  }
});
