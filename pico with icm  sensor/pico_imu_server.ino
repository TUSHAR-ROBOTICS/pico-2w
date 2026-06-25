/*
  CubeSat Control Panel — IMU HTTP Server
  Raspberry Pi Pico 2W + ICM-20948
  
  Connects to WiFi, serves sensor data as JSON at:
    http://<pico-ip>/data

  Wiring:
    ICM-20948 SDA → GP0
    ICM-20948 SCL → GP1
    ICM-20948 VCC → 3V3
    ICM-20948 GND → GND
    ICM-20948 AD0 → GND  (I2C addr = 0x68)
*/

#include <Wire.h>
#include <math.h>
#include <WiFi.h>

// ─── WiFi credentials ────────────────────────────────────────────────────────
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// ─── ICM-20948 ───────────────────────────────────────────────────────────────
#define ICM_ADDR 0x68

int16_t ax, ay, az;
float rollFiltered  = 0;
float pitchFiltered = 0;
const float alpha   = 0.25;

// ─── HTTP Server ──────────────────────────────────────────────────────────────
WiFiServer server(80);

// ─── Register helpers ────────────────────────────────────────────────────────
void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

uint8_t readReg(uint8_t reg) {
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(ICM_ADDR, 1);
  return Wire.available() ? Wire.read() : 0;
}

void readAccel() {
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(0x2D);
  Wire.endTransmission(false);
  Wire.requestFrom(ICM_ADDR, 6);
  if (Wire.available() >= 6) {
    ax = (Wire.read() << 8) | Wire.read();
    ay = (Wire.read() << 8) | Wire.read();
    az = (Wire.read() << 8) | Wire.read();
  }
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);

  // I2C init
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();
  delay(200);

  // Check sensor
  uint8_t whoami = readReg(0x00);
  Serial.print("WHO_AM_I = 0x");
  Serial.println(whoami, HEX);
  if (whoami != 0xEA) {
    Serial.println("ICM-20948 NOT FOUND — check wiring!");
    while (1);
  }
  writeReg(0x06, 0x01);  // Wake sensor
  Serial.println("ICM-20948 ready");

  // WiFi connect
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("Pico IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Open your dashboard and enter this IP.");

  server.begin();
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  // Update sensor
  readAccel();
  float roll  = atan2((float)ay, (float)az) * 57.2958;
  float pitch = atan2(-(float)ax, sqrt((float)ay*ay + (float)az*az)) * 57.2958;
  rollFiltered  = rollFiltered  * (1.0 - alpha) + roll  * alpha;
  pitchFiltered = pitchFiltered * (1.0 - alpha) + pitch * alpha;

  // Handle HTTP client
  WiFiClient client = server.accept();
  if (!client) return;

  // Wait for request
  unsigned long t = millis();
  while (!client.available() && millis() - t < 500);

  String req = client.readStringUntil('\r');
  client.readString(); // flush headers

  if (req.indexOf("GET /data") >= 0) {
    // Build JSON
    String json = "{";
    json += "\"roll\":"  + String(rollFiltered,  2) + ",";
    json += "\"pitch\":" + String(pitchFiltered, 2) + ",";
    json += "\"ax\":"    + String(ax)               + ",";
    json += "\"ay\":"    + String(ay)               + ",";
    json += "\"az\":"    + String(az)               + ",";
    json += "\"uptime\":" + String(millis())        ;
    json += "}";

    // HTTP response with CORS so any local file can fetch
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.println();
    client.print(json);

  } else if (req.indexOf("GET /") >= 0) {
    // Root — helpful redirect message
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("CubeSat IMU Server running.");
    client.println("Sensor data available at /data");

  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Connection: close");
    client.println();
  }

  client.stop();
}
