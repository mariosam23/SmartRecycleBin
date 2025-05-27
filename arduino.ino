#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// Constants 
#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

#define SERVO_PIN 6
#define WATER_SENSOR_PIN A0
#define BUZZER_PIN 7

#define WATER_THRESHOLD 300
#define OPEN_DURATION_MS 3000
#define WATER_MSG_INTERVAL 2000

#define SERVO_OPEN_POS 0
#define SERVO_CLOSED_POS 90
#define BUZZER_FREQ_HZ 1000
#define BUZZER_TONE_DURATION 300

// Objects and State Variables 
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
Servo sgServo;

bool isOpen = false;
unsigned long lastOpenTime = 0;
unsigned long lastWaterMsg = 0;
int lastWaterRead = 0;

void setupServo();
void checkWaterSensor();
void handleEspCommand();
void openServo();
void closeServo();
void autoCloseCheck();
void updateLcd();

void setup() {
  Serial.begin(9600);

  setupServo();
  // Buzzer pin is set as output
  DDRD |= (1 << BUZZER_PIN);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing");
  // Simulating loading...
  delay(500);
}

void loop() {
  checkWaterSensor();
  handleEspCommand();
  autoCloseCheck();
  updateLcd();
}

// Initially servo is closed
void setupServo() {
  sgServo.attach(SERVO_PIN);
  sgServo.write(SERVO_CLOSED_POS);
  isOpen = false;
}

void checkWaterSensor() {
  int waterValue = analogRead(WATER_SENSOR_PIN);
  lastWaterRead = waterValue;

  if (waterValue > WATER_THRESHOLD && millis() - lastWaterMsg > WATER_MSG_INTERVAL) {
    Serial.println("WATER");

    unsigned long startTime = millis();
    while (millis() - startTime < BUZZER_TONE_DURATION) {
      // High
      PORTD |= (1 << BUZZER_PIN);
      delayMicroseconds(500);
      // Low
      PORTD &= ~(1 << BUZZER_PIN);
      delayMicroseconds(500);
    }
    PORTD &= ~(1 << BUZZER_PIN);

    lastWaterMsg = millis();
  }
}

void handleEspCommand() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.equalsIgnoreCase("OPEN")) {
    if (!isOpen) openServo();
    lastOpenTime = millis();
  }
  else if (cmd.equalsIgnoreCase("CLOSE")) {
    if (isOpen) closeServo();
  }
}

void openServo() {
  sgServo.write(SERVO_OPEN_POS);
  isOpen = true;
}

void closeServo() {
  sgServo.write(SERVO_CLOSED_POS);
  isOpen = false;
}

void autoCloseCheck() {
  if (isOpen && millis() - lastOpenTime > OPEN_DURATION_MS) {
    closeServo();
  }
}

void updateLcd() {
  lcd.setCursor(0, 0);
  lcd.print("Status:");
  lcd.print(isOpen ? "OPENED " : "CLOSED");

  lcd.setCursor(0, 1);
  lcd.print("H2O:");
  lcd.print(lastWaterRead);
  lcd.print("    ");
}
