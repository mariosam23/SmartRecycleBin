#include <Servo.h>
#include <SoftwareSerial.h>

//  Pins 
const int servoPin = 6;
const int waterSensorPin = A0;
const int espRx = 2;
const int espTx = 3;

//  Constants 
const int waterThreshold = 500;
const unsigned long OPEN_DURATION = 3000;
const unsigned long WATER_MSG_INTERVAL = 2000;

//  State Variables 
Servo myServo;
SoftwareSerial espSerial(espRx, espTx);
bool isOpen = false;
unsigned long lastOpenTime = 0;
unsigned long lastWaterMsg = 0;

//  Function Prototypes 
void setupServo();
void checkWaterSensor();
void handleEspCommand();
void openServo();
void closeServo();
void autoCloseCheck();

void setup() {
	Serial.begin(9600);
	espSerial.begin(9600);
	setupServo();
}

void loop() {
	checkWaterSensor();
	handleEspCommand();
	autoCloseCheck();
}

//  Initialize Servo 
void setupServo() {
	myServo.attach(servoPin);
	// Initial CLOSED
	myServo.write(90);
	isOpen = false;
}

//  Check Water Sensor 
void checkWaterSensor() {
	int waterValue = analogRead(waterSensorPin);
	
	if (waterValue > waterThreshold && millis() - lastWaterMsg > WATER_MSG_INTERVAL) {
		espSerial.println("WATER");
		Serial.println("Water detected. Sent 'WATER' to ESP32");
		lastWaterMsg = millis();
	}
}

//  Handle Incoming Command from ESP32 
void handleEspCommand() {
	if (espSerial.available()) {
		String cmd = espSerial.readStringUntil('\n');
		cmd.trim();
		Serial.print("Received from ESP32: "); Serial.println(cmd);

		if (cmd.equalsIgnoreCase("OPEN")) {
			if (!isOpen) openServo();
			lastOpenTime = millis();
		} else if (cmd.equalsIgnoreCase("CLOSE")) {
			if (isOpen) closeServo();
		}
	}
}

//  Open Servo 
void openServo() {
	myServo.write(0);
	isOpen = true;
	Serial.println("Servo OPEN");
}

//  Close Servo 
void closeServo() {
	myServo.write(90);
	isOpen = false;
	Serial.println("Servo CLOSE");
}

//  Auto-close after timeout 
void autoCloseCheck() {
	if (isOpen && millis() - lastOpenTime > OPEN_DURATION) {
		closeServo();
		Serial.println("Servo close after 3s");
	}
}
