#include <WiFi.h>

// Wi-Fi credentials
const char* ssid     = "ssid";
const char* password = "password";

// UART to UNO: RX2=GPIO16, TX2=GPIO17
#define ARD_RX_PIN 16
#define ARD_TX_PIN 17
#define BAUD_RATE  9600

// HC-SR04 for hand detection
const int TRIG = 18;
const int ECHO = 19;
const float HAND_MIN = 2.0;
const float HAND_MAX = 15.0;

// Status handling
String lastMessage = "";
unsigned long lastWaterTime = 0;
const unsigned long WATER_INTERVAL = 2000;

// Debounce config
int nearCount = 0;
const int NEED = 3;

// Serial & Wi-Fi
HardwareSerial Ser2(2);
WiFiServer server(80);

// Function Prototypes
void setupWiFi();
long measureDistance();
void handleHandDetection();
void handleSerialInput();
void handleHttpRequest();
void sendToArduino(String& cmd);
void updateStatus(String& msg);

// Setup
void setup() {
	Serial.begin(115200);
	Ser2.begin(BAUD_RATE, SERIAL_8N1, ARD_RX_PIN, ARD_TX_PIN);
	pinMode(TRIG, OUTPUT);
	pinMode(ECHO, INPUT_PULLDOWN);
	setupWiFi();
	server.begin();
}

// Main Loop
void loop() {
	handleHandDetection();
	handleSerialInput();
	handleHttpRequest();
	delay(200);
}

// Wi-Fi Connection
void setupWiFi() {
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500); Serial.print(".");
	}
	Serial.println("\nWi-Fi up, IP = " + WiFi.localIP().toString());
}

// Measure Distance
long measureDistance() {
	digitalWrite(TRIG, LOW);
	delayMicroseconds(2);
	digitalWrite(TRIG, HIGH);
	delayMicroseconds(10);
	digitalWrite(TRIG, LOW);
	return pulseIn(ECHO, HIGH, 30000);
}

// Hand Detection & Servo Trigger
void handleHandDetection() {
	long d = measureDistance();
	float cm = (d > 0) ? (d * 0.0343 / 2.0) : 999.0;
	Serial.print("Distance: "); Serial.print(cm); Serial.println(" cm");

	if (cm >= HAND_MIN && cm <= HAND_MAX) {
		nearCount++;
		Serial.print("nearCount: "); Serial.println(nearCount);
	} else {
		nearCount = 0;
	}

	if (nearCount >= NEED) {
		sendToArduino("OPEN");
		nearCount = 0;
	}
}

// Read from Arduino (Serial2)
void handleSerialInput() {
	if (Ser2.available()) {
		String msg = Ser2.readStringUntil('\n');
		msg.trim();
		Serial.println("From Arduino: " + msg);
		updateStatus(msg);
	}
}

// Status Update
void updateStatus(String& msg) {
	if (msg == "WATER" && millis() - lastWaterTime > WATER_INTERVAL) {
		lastMessage = "WATER";
		lastWaterTime = millis();
	}
}

// Send Command to Arduino
void sendToArduino(String& cmd) {
	Ser2.println(cmd);
	Serial.println("→ " + cmd + " sent to Uno (via Serial2)");
}

// HTTP Server Handler
void handleHttpRequest() {
	WiFiClient cli = server.available();
	if (!cli) return;

	String req = cli.readStringUntil('\r');
	cli.flush();
	Serial.print("HTTP Request: "); Serial.println(req);

	String response = "OK";
	if (req.indexOf("GET /open ") != -1) {
		sendToArduino("OPEN");
	} else if (req.indexOf("GET /close ") != -1) {
		sendToArduino("CLOSE");
	} else if (req.indexOf("GET /status") != -1) {
		response = (lastMessage == "WATER") ? "WATER" : "OK";
	}

	cli.println("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + response);
	cli.stop();
}

