const WebSocket = require('ws');
const TelegramBot = require('node-telegram-bot-api');

const config = require('./config.json');

const bot = new TelegramBot(config.telegramToken, { polling: true });

const skWs = new WebSocket(`ws://${config.signalkServer}:${config.signalkPort}/signalk/v1/stream`);

skWs.on('open', () => {
  console.log('✅ Conectado a SignalK');
  skWs.send(JSON.stringify({
    "context": "vessels.self",
    "subscribe": [
      { "path": "electrical.batteries.0.voltage", "format": "delta", "minPeriod": 2000 },
      { "path": "tanks.freshWater.0.currentLevel", "format": "delta", "minPeriod": 2000 }
    ]
  }));
});

let batteryAlertSent = false;
let waterAlertSent = false;

skWs.on('message', (data) => {
  try {
    const msg = JSON.parse(data);
    if (msg.updates) {
      msg.updates.forEach(update => {
        update.values.forEach(value => {
          const path = value.path;
          const val = value.value;

          if (path === "electrical.batteries.0.voltage" && val < config.alarms.lowBattery && !batteryAlertSent) {
            bot.sendMessage(config.chatId, `🚨 Batería baja: ${val.toFixed(2)} V`);
            batteryAlertSent = true;
            setTimeout(() => { batteryAlertSent = false; }, 3600000);
          }

          if (path === "tanks.freshWater.0.currentLevel" && val < config.alarms.lowWater && !waterAlertSent) {
            bot.sendMessage(config.chatId, `💧 Agua baja: ${(val*100).toFixed(1)}%`);
            waterAlertSent = true;
            setTimeout(() => { waterAlertSent = false; }, 3600000);
          }
        });
      });
    }
  } catch (e) { console.error(e); }
});

skWs.on('error', (err) => console.error('❌ Error SignalK:', err));
skWs.on('close', () => setTimeout(() => process.exit(0), 5000));
