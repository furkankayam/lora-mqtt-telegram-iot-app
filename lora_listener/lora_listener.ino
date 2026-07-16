/* 
  Code Created By - Mehmet Furkan KAYA
  GitHub - /furkankayam
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RadioLib.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <Preferences.h>

// ─── WiFi AP (local panel, always on) ─────────────────
#define WIFI_SSID     "lora-listener"
#define WIFI_PASS     "123456788"

// ─── Default MQTT settings (used until configured) ──
#define MQTT_PORT_DEFAULT   1883
#define MQTT_CLIENT_ID      "lora-listener"
#define MQTT_TOPIC_RX       "lora/rx"
#define MQTT_TOPIC_STATUS   "lora/status"

// ─── LoRa Pins (LilyGO T3-S3) ──────────────────────────
#define LORA_SCK   5
#define LORA_MISO  3
#define LORA_MOSI  6
#define LORA_CS    7
#define LORA_RST   8
#define LORA_DIO1  33
#define LORA_BUSY  34

// ─── OLED Pins (LilyGO T3-S3) ──────────────────────────
#define I2C_SDA    18
#define I2C_SCL    17

// ─── Objects ──────────────────────────────────────────────
SPIClass spi(FSPI);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY, spi);
WebServer server(80);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Preferences prefs;

// ─── Config Variables (loaded from NVS) ──────────────────
String cfgStaSsid;
String cfgStaPass;
String cfgMqttHost;
int    cfgMqttPort;
String cfgMqttUser;
String cfgMqttPass;

// ─── State Variables ────────────────────────────────────
String lastReceived = "---";
volatile bool loraReceived = false;
unsigned long lastMqttAttempt = 0;
const unsigned long MQTT_RETRY_MS = 5000;

// ─── Load / Save Settings ────────────────────────────────
void loadSettings() {
  prefs.begin("cfg", true);
  cfgStaSsid  = prefs.getString("sta_ssid", "");
  cfgStaPass  = prefs.getString("sta_pass", "");
  cfgMqttHost = prefs.getString("mqtt_host", "");
  cfgMqttPort = prefs.getInt("mqtt_port", MQTT_PORT_DEFAULT);
  cfgMqttUser = prefs.getString("mqtt_user", "");
  cfgMqttPass = prefs.getString("mqtt_pass", "");
  prefs.end();
}

void saveSettings(const String &staSsid, const String &staPass,
                   const String &mqttHost, int mqttPort,
                   const String &mqttUser, const String &mqttPass) {
  prefs.begin("cfg", false);
  prefs.putString("sta_ssid", staSsid);
  prefs.putString("sta_pass", staPass);
  prefs.putString("mqtt_host", mqttHost);
  prefs.putInt("mqtt_port", mqttPort);
  prefs.putString("mqtt_user", mqttUser);
  prefs.putString("mqtt_pass", mqttPass);
  prefs.end();
}

// ─── Update OLED ─────────────────────────────────────────
void updateOled() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("LoRa Listener");

  display.drawLine(0, 12, 128, 12, WHITE);

  display.setCursor(0, 20);
  display.print("IP: ");
  display.print(WiFi.softAPIP().toString());

  display.drawLine(0, 30, 128, 30, WHITE);

  display.setCursor(0, 38);
  display.print("Last Received:");
  display.setCursor(0, 48);
  display.print(lastReceived.substring(0, 21));

  display.display();
}

// ─── MQTT Publish ────────────────────────────────────────
void mqttPublishRx(const String &message, float rssi, float snr) {
  if (!mqttClient.connected()) return;

  String payload = "{";
  payload += "\"msg\":\"" + message + "\",";
  payload += "\"rssi\":" + String(rssi, 1) + ",";
  payload += "\"snr\":" + String(snr, 1);
  payload += "}";

  mqttClient.publish(MQTT_TOPIC_RX, payload.c_str());
}

void mqttConnect() {
  if (cfgMqttHost.length() == 0) return;
  if (mqttClient.connected()) return;
  if (WiFi.status() != WL_CONNECTED) return;

  unsigned long now = millis();
  if (now - lastMqttAttempt < MQTT_RETRY_MS) return;
  lastMqttAttempt = now;

  Serial.print("[MQTT] Connecting...");
  bool success;
  if (cfgMqttUser.length() > 0) {
    success = mqttClient.connect(MQTT_CLIENT_ID, cfgMqttUser.c_str(), cfgMqttPass.c_str());
  } else {
    success = mqttClient.connect(MQTT_CLIENT_ID);
  }

  if (success) {
    Serial.println(" connected.");
    mqttClient.publish(MQTT_TOPIC_STATUS, "online", true);
  } else {
    Serial.printf(" failed, rc=%d\n", mqttClient.state());
  }
}

// ─── Shared HTML Style/Header ─────────────────────────────────
String htmlStart(const String &title) {
  String html = "<!DOCTYPE html><html lang=\"en\"><head>";
  html += "<meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<title>" + title + "</title><style>";
  html += "body{font-family:Arial,sans-serif;background:#ffffff;color:#1a1a1a;display:flex;flex-direction:column;align-items:center;padding:20px;}";
  html += "h2{color:#1a1a1a;font-weight:600;}";
  html += ".card{background:#ffffff;border:1px solid #e0e0e0;border-radius:4px;padding:20px;width:100%;max-width:400px;margin:10px 0;}";
  html += "label{font-size:13px;color:#666666;display:block;margin-top:10px;}";
  html += ".msg{font-size:18px;font-weight:bold;color:#2e7d32;margin-top:5px;word-break:break-all;}";
  html += "input[type=text],input[type=password],input[type=number]{width:100%;padding:10px;border-radius:4px;border:1px solid #cccccc;background:#ffffff;color:#1a1a1a;font-size:16px;box-sizing:border-box;margin-top:4px;}";
  html += "input[type=text]:focus,input[type=password]:focus,input[type=number]:focus{outline:none;border-color:#2e7d32;}";
  html += "button{margin-top:12px;width:100%;padding:12px;background:#2e7d32;color:#ffffff;border:none;border-radius:4px;font-size:16px;cursor:pointer;font-weight:600;}";
  html += "button:hover{background:#256428;}";
  html += "a{color:#2e7d32;text-decoration:none;font-size:14px;margin-top:14px;}";
  html += ".btn-settings{display:block;width:100%;max-width:400px;padding:12px;margin-top:6px;background:#ffffff;color:#2e7d32;border:1px solid #2e7d32;border-radius:4px;font-size:16px;font-weight:600;text-align:center;box-sizing:border-box;}";
  html += "</style></head><body>";
  return html;
}

// ─── Web UI HTML (home page) ──────────────────────────
String htmlPage() {
  String html = htmlStart("Listener");
  html += "<h2>Listener</h2>";

  html += "<div class=\"card\"><label>Last Received</label><div class=\"msg\" id=\"rx\">" + lastReceived + "</div></div>";

  html += "<a class=\"btn-settings\" href=\"/settings\">WiFi / MQTT Settings</a>";

  html += R"rawhtml(
<script>
setInterval(() => {
  fetch('/status')
    .then(r => r.json())
    .then(d => {
      document.getElementById('rx').innerText = d.rx;
    });
}, 2000);
</script>
</body></html>
)rawhtml";
  return html;
}

// ─── Settings Page HTML ───────────────────────────────────
String htmlSettings(const String &message = "") {
  String html = htmlStart("Settings");
  html += "<h2>Settings</h2>";

  if (message.length() > 0) {
    html += "<div class=\"card\" style=\"color:#2e7d32;text-align:center;\">" + message + "</div>";
  }

  html += "<form method=\"POST\" action=\"/settings/save\">";
  html += "<div class=\"card\">";
  html += "<label>Internet WiFi (STA) SSID</label>";
  html += "<input type=\"text\" name=\"sta_ssid\" value=\"" + cfgStaSsid + "\">";
  html += "<label>Internet WiFi Password</label>";
  html += "<input type=\"password\" name=\"sta_pass\" value=\"" + cfgStaPass + "\">";
  html += "</div>";

  html += "<div class=\"card\">";
  html += "<label>MQTT Broker Address</label>";
  html += "<input type=\"text\" name=\"mqtt_host\" value=\"" + cfgMqttHost + "\" placeholder=\"e.g. 192.168.1.50 or broker.hivemq.com\">";
  html += "<label>MQTT Port</label>";
  html += "<input type=\"number\" name=\"mqtt_port\" value=\"" + String(cfgMqttPort) + "\">";
  html += "<label>MQTT Username (optional)</label>";
  html += "<input type=\"text\" name=\"mqtt_user\" value=\"" + cfgMqttUser + "\">";
  html += "<label>MQTT Password (optional)</label>";
  html += "<input type=\"password\" name=\"mqtt_pass\" value=\"" + cfgMqttPass + "\">";
  html += "</div>";

  html += "<button type=\"submit\">Save and Restart</button>";
  html += "</form>";
  html += "<a href=\"/\">Back to home</a>";
  html += "</body></html>";
  return html;
}

// ─── Web Handlers ───────────────────────────────────────
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleStatus() {
  String json = "{\"rx\":\"" + lastReceived + "\"}";
  server.send(200, "application/json", json);
}

void handleSettings() {
  server.send(200, "text/html", htmlSettings());
}

void handleSettingsSave() {
  String staSsid  = server.arg("sta_ssid");
  String staPass  = server.arg("sta_pass");
  String mqttHost = server.arg("mqtt_host");
  int mqttPort    = server.arg("mqtt_port").toInt();
  String mqttUser = server.arg("mqtt_user");
  String mqttPass = server.arg("mqtt_pass");

  if (mqttPort <= 0) mqttPort = MQTT_PORT_DEFAULT;

  saveSettings(staSsid, staPass, mqttHost, mqttPort, mqttUser, mqttPass);

  String html = htmlStart("Saved");
  html += "<h2>Saved</h2>";
  html += "<div class=\"card\" style=\"text-align:center;\">Settings saved. The device is restarting...</div>";
  html += R"rawhtml(
<script>
setTimeout(() => {
  window.location.href = '/';
}, 4000);
</script>
</body></html>
)rawhtml";
  server.send(200, "text/html", html);

  delay(1000);
  ESP.restart();
}

// ─── LoRa Interrupt ────────────────────────────────────────
void IRAM_ATTR loraIRQ() {
  loraReceived = true;
}

// ─── LoRa Setup (called from setup) ──────────────────
bool loraSetup() {
  spi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  int state = radio.begin(868.0);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("[LoRa] Error: %d\n", state);
    return false;
  }

  radio.setFrequency(868.0);
  radio.setBandwidth(125.0);
  radio.setSpreadingFactor(7);
  radio.setCodingRate(5);
  radio.setSyncWord(0xAB);

  radio.setDio1Action(loraIRQ);
  radio.startReceive();
  return true;
}

// ─── Setup ─────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[OLED] Init failed!");
  }
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(20, 28);
  display.print("Listener");
  display.display();
  delay(2000);

  loadSettings();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  Serial.print("[WiFi] AP IP: ");
  Serial.println(WiFi.softAPIP());

  if (cfgStaSsid.length() > 0) {
    WiFi.begin(cfgStaSsid.c_str(), cfgStaPass.c_str());
    Serial.print("[WiFi] Connecting to STA");
    unsigned long staStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - staStart < 15000) {
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(" connected: " + WiFi.localIP().toString());
    } else {
      Serial.println(" failed, running in AP mode only.");
    }
  } else {
    Serial.println("[WiFi] No STA config, use /settings page.");
  }

  if (cfgMqttHost.length() > 0) {
    mqttClient.setServer(cfgMqttHost.c_str(), cfgMqttPort);
    mqttConnect();
  }

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/settings/save", HTTP_POST, handleSettingsSave);
  server.begin();
  Serial.println("[Web] Server started");

  if (!loraSetup()) {
    display.clearDisplay();
    display.setCursor(0, 20);
    display.print("LoRa ERROR!");
    display.display();
    while (true) { server.handleClient(); }
  }

  Serial.println("[LoRa] Ready, listening...");
  updateOled();
}

// ─── Loop ──────────────────────────────────────────────────
void loop() {
  server.handleClient();
  mqttConnect();
  mqttClient.loop();

  if (loraReceived) {
    loraReceived = false;
    String incoming;
    int state = radio.readData(incoming);
    if (state == RADIOLIB_ERR_NONE) {
      lastReceived = incoming;
      float rssi = radio.getRSSI();
      float snr = radio.getSNR();
      Serial.println("[LoRa] Received: " + incoming);
      mqttPublishRx(incoming, rssi, snr);
      updateOled();
    } else {
      Serial.printf("[LoRa] Read error: %d\n", state);
    }
    radio.startReceive();
  }
}
