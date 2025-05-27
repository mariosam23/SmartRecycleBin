#include <WiFi.h>
#include "soc/gpio_reg.h"


// Wi-Fi Credentials 
#define WIFI_SSID "AndroidAP8798"
#define WIFI_PASSWORD "12345677"
// #define WIFI_SSID "DIGI-SAs5"
// #define WIFI_PASSWORD "uyxZF3dsN2"

// UART Communication with Arduino 
#define ARD_RX_PIN 16
#define ARD_TX_PIN 17
#define UART_BAUD 9600

// HC-SR04 Hand Detection 
#define TRIG_PIN 18
#define ECHO_PIN 19
#define HAND_MIN_CM 2.0
#define HAND_MAX_CM 15.0

// Debounce and Status 
#define HAND_NEAR_REQUIRED 2
#define WATER_STATUS_INTERVAL_MS 2000

// Measurement Timing 
#define TRIG_PULSE_US 10
#define TRIG_SETTLE_US 2
#define ECHO_TIMEOUT_US 30000
#define SPEED_OF_SOUND_CMUS 0.0343

// HTTP Server 
#define SERVER_PORT 80

// Serial2 (to Uno)
HardwareSerial Ser2(2);
WiFiServer server(SERVER_PORT);

String lastMessage = "";
unsigned long lastWaterTime = 0;
int nearCount = 0;

// Functions
void setupWiFi();
long measureDistance();
void handleHandDetection();
void handleSerialInput();
void handleHttpRequest();
void sendToArduino(const String& cmd);
void updateStatus(const String& msg);

void setup() {
  Serial.begin(9600);
  // 8 data bits; No parity; 1 stop bit
  Ser2.begin(UART_BAUD, SERIAL_8N1, ARD_RX_PIN, ARD_TX_PIN);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT_PULLDOWN);

  setupWiFi();
  server.begin();
}

void loop() {
  handleHandDetection();
  handleSerialInput();
  handleHttpRequest();
  delay(200);
}

void setupWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected. IP: " + WiFi.localIP().toString());
}

long measureDistance() {
  // Clear the trigger pin
  (*(volatile uint32_t *)GPIO_OUT_W1TC_REG) = (1 << TRIG_PIN);
  delayMicroseconds(TRIG_SETTLE_US);

  // Generate 10-microsecond pulse to TRIG pin
  (*(volatile uint32_t *)GPIO_OUT_W1TS_REG) = (1 << TRIG_PIN);
  delayMicroseconds(TRIG_PULSE_US);

  (*(volatile uint32_t *)GPIO_OUT_W1TC_REG) = (1 << TRIG_PIN);

  // Measure duration of pulse from ECHO pin
  return pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
}

void handleHandDetection() {
  long duration = measureDistance();
  float distanceCm = (duration > 0) ? (duration * SPEED_OF_SOUND_CMUS / 2.0) : 999.0;

  // Serial.print("Distance: "); Serial.print(distanceCm); Serial.println(" cm");

  if (distanceCm >= HAND_MIN_CM && distanceCm <= HAND_MAX_CM) {
    nearCount++;
    Serial.print("nearCount: "); Serial.println(nearCount);
  } else {
    nearCount = 0;
  }

  if (nearCount >= HAND_NEAR_REQUIRED) {
    sendToArduino("OPEN");
    nearCount = 0;
  }
}

// Incoming Serial from Arduino 
void handleSerialInput() {
  if (Ser2.available()) {
    String msg = Ser2.readStringUntil('\n');
    msg.trim();
    Serial.println("From Arduino: " + msg);
    updateStatus(msg);
  }
}

// Update Status from Arduino 
void updateStatus(const String& msg) {
  if (msg == "WATER" && millis() - lastWaterTime > WATER_STATUS_INTERVAL_MS) {
    lastMessage = "WATER";
    lastWaterTime = millis();
  }
}

// Send Commands to Arduino 
void sendToArduino(const String& cmd) {
  Ser2.println(cmd);
  Serial.println(cmd + " sent to Uno (via Serial2)");
}

// HTTP Request Handling
void handleHttpRequest() {
  WiFiClient client = server.available();
  if (!client) return;

  String req = client.readStringUntil('\r');
  client.flush();
  Serial.print("HTTP Request: "); Serial.println(req);

  String response = "OK";

  if (req.indexOf("GET /open ") != -1) {
    sendToArduino("OPEN");
  } else if (req.indexOf("GET /close ") != -1) {
    sendToArduino("CLOSE");
  } else if (req.indexOf("GET /status") != -1) {
    response = (lastMessage == "WATER") ? "WATER" : "OK";
  }

  client.println("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + response);
  client.stop();
}
