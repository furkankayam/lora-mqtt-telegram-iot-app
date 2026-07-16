/* 
  Code Created By - Mehmet Furkan KAYA
  GitHub - github.com/furkankayam
*/

#include <SPI.h>
#include <WiFi.h>
#include <RadioLib.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Fonts/TomThumb.h>
#include <Adafruit_SSD1306.h>

// ─── WiFi AP ───────────────────────────────────────────────
#define WIFI_SSID     "lora-sender"
#define WIFI_PASS     "123456788"

// ─── LoRa Pins (Heltec WiFi LoRa 32 V3) ────────────────
#define LORA_SCK   9
#define LORA_MISO  11
#define LORA_MOSI  10
#define LORA_CS    8
#define LORA_RST   12
#define LORA_DIO1  14
#define LORA_BUSY  13

// ─── OLED Pins (Heltec V3) ──────────────────────────────
#define OLED_SDA   17
#define OLED_SCL   18
#define OLED_RST   21

// ─── Heltec Vext (OLED power line) ─────────────────────────
#ifndef Vext
#define Vext 36
#endif

void VextON() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

// ─── Objects ──────────────────────────────────────────────
SPIClass spi(FSPI);
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RST);
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY, spi);
WebServer server(80);

// ─── State Variables ────────────────────────────────────
String lastSent = "---";

// ─── Update OLED ─────────────────────────────────────────
void updateOled() {
  display.clearDisplay();
  display.setFont(&TomThumb);

  display.setCursor(0, 6);
  display.print("LoRa Sender");

  display.drawLine(0, 12, 64, 12, WHITE);

  display.setCursor(0, 20);
  display.print("IP: ");
  display.print(WiFi.softAPIP().toString());

  display.drawLine(0, 26, 64, 26, WHITE);

  display.setCursor(0, 34);
  display.print("Last Sent:");

  const int charsPerLine = 16;
  const int lineHeight = 7;
  int y = 41;

  for (int i = 0; i < lastSent.length(); i += charsPerLine) {
    display.setCursor(0, y);
    display.print(lastSent.substring(i, i + charsPerLine));
    y += lineHeight;
  }

  display.display();
}

// ─── Web UI HTML ──────────────────────────────────────
String htmlPage() {
  String html = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>LoRa Sender</title>
<style>
  body { font-family: Arial, sans-serif; background: #ffffff; color: #1a1a1a; display: flex; flex-direction: column; align-items: center; padding: 20px; }
  h2 { color: #1a1a1a; font-weight: 600; }
  .card { background: #ffffff; border: 1px solid #e0e0e0; border-radius: 4px; padding: 20px; width: 100%; max-width: 400px; margin: 10px 0; }
  label { font-size: 13px; color: #666666; }
  .msg { font-size: 18px; font-weight: bold; color: #2e7d32; margin-top: 5px; word-break: break-all; }
  input[type=text] { width: 100%; padding: 10px; border-radius: 4px; border: 1px solid #cccccc; background: #ffffff; color: #1a1a1a; font-size: 16px; box-sizing: border-box; margin-top: 8px; }
  input[type=text]:focus { outline: none; border-color: #2e7d32; }
  button { margin-top: 12px; width: 100%; padding: 12px; background: #2e7d32; color: #ffffff; border: none; border-radius: 4px; font-size: 16px; cursor: pointer; font-weight: 600; }
  button:hover { background: #256428; }
  .toast { position: fixed; bottom: 30px; left: 50%; transform: translateX(-50%) translateY(20px); background: #2e7d32; color: #ffffff; padding: 12px 24px; border-radius: 4px; font-size: 14px; font-weight: 600; opacity: 0; pointer-events: none; transition: opacity 0.3s ease, transform 0.3s ease; }
  .toast.show { opacity: 1; transform: translateX(-50%) translateY(0); }
</style>
</head>
<body>
<h2>Sender</h2>

<div class="card">
  <label>Send Message</label>
  <input type="text" id="message" placeholder="Type your message..." maxlength="50">
  <button onclick="sendMessage()">Send</button>
</div>

<div class="card">
  <label>Last Sent</label>
  <div class="msg" id="tx">)rawhtml";
  html += lastSent;
  html += R"rawhtml(</div>
</div>

<div class="toast" id="toast"></div>

<script>
function sendMessage() {
  const m = document.getElementById('message').value.trim();
  if (!m) return;
  fetch('/send?msg=' + encodeURIComponent(m))
    .then(r => r.text())
    .then(t => {
      showToast(t);
      document.getElementById('tx').innerText = m;
      document.getElementById('message').value = '';
    });
}

function showToast(text) {
  const toast = document.getElementById('toast');
  toast.innerText = text;
  toast.classList.add('show');
  setTimeout(() => {
    toast.classList.remove('show');
  }, 3000);
}
</script>
</body>
</html>
)rawhtml";
  return html;
}

// ─── Web Handlers ───────────────────────────────────────
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleSend() {
  if (server.hasArg("msg")) {
    String message = server.arg("msg");
    message.trim();
    if (message.length() > 0) {
      lastSent = message;
      updateOled();

      int state = radio.transmit(message);
      if (state == RADIOLIB_ERR_NONE) {
        Serial.println("[LoRa] Sent: " + message);
        server.send(200, "text/plain", "Sent");
      } else {
        Serial.printf("[LoRa] Send error: %d\n", state);
        server.send(200, "text/plain", "Error: " + String(state));
      }
    } else {
      server.send(400, "text/plain", "Empty message");
    }
  } else {
    server.send(400, "text/plain", "Missing parameter");
  }
}

// ─── Setup ─────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);

  VextON();
  delay(100);

  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);
  delay(20);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[OLED] Init failed!");
  }
  display.setRotation(1);
  display.setTextColor(WHITE);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(14, 60);
  display.print("Sender");
  display.display();
  delay(2000);

  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  Serial.print("[WiFi] AP IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/send", handleSend);
  server.begin();
  Serial.println("[Web] Server started");

  spi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  int state = radio.begin(868.0);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("[LoRa] Error: %d\n", state);
    display.clearDisplay();
    display.setCursor(0, 20);
    display.print("LoRa ERROR: ");
    display.print(state);
    display.display();
    while (true);
  }

  radio.setFrequency(868.0);
  radio.setBandwidth(125.0);
  radio.setSpreadingFactor(7);
  radio.setCodingRate(5);
  radio.setSyncWord(0xAB);

  Serial.println("[LoRa] Ready to send...");
  updateOled();
}

// ─── Loop ──────────────────────────────────────────────────
void loop() {
  server.handleClient();
}
