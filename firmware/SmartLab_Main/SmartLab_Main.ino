/*
  SmartLab - Merged firmware (ESP32)
  Feature 1: PIR-based automatic light (energy saving)
  Feature 2: MQ-2 smoke + flame sensor safety (fan, pump, buzzer)
  Feature 3: RFID door access with servo lock

  NOTE: Everything is NON-BLOCKING (millis() based), so the door timer
  never stops the fire/smoke monitoring.
*/

#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// ---------------- PIN DEFINITIONS ----------------

// Feature 1 + 2
const int PIR        = 27;
const int LED        = 2;
const int MQ2        = 34;
const int FLAME      = 25;
const int BUZZER     = 26;   // Buzzer 2 (fire/smoke alert), active-LOW
const int FAN_RELAY  = 32;   // active-LOW
const int PUMP_RELAY = 33;   // active-LOW

// Feature 3
#define SS_PIN      5
#define RST_PIN     22
#define SERVO_PIN   13
#define SCK_PIN     18
#define MISO_PIN    19
#define MOSI_PIN    23

// Buzzer 1 (RFID access denied), active-LOW.
// TODO: change this pin to the one you actually wired (or remove the buzzer lines)
const int ACCESS_BUZZER = 4;

// ---------------- SETTINGS ----------------

const unsigned long lightDelayTime   = 30000;  // LED off after 30 s of no motion
const unsigned long sensorInterval   = 500;    // read/print sensors every 0.5 s
const unsigned long doorOpenTime     = 6000;   // door stays open 6 s
const unsigned long deniedBeepTime   = 300;    // denied beep length
const unsigned long rfidPollInterval = 100;    // check RFID every 0.1 s

// Change these after testing your sensors
int smokeThreshold = 2000;
int flameThreshold = 1000;

// ---------------- OBJECTS ----------------

MFRC522 rfid(SS_PIN, RST_PIN);
Servo doorServo;

byte authorizedUID[] = { 0x4A, 0x0A, 0xF9, 0x34 };

// ---------------- STATE ----------------

// Feature 1
unsigned long lastMotion = 0;
bool ledState = false;

// Feature 2
unsigned long lastSensorRead = 0;

// Feature 3
bool waitingForRemoval = false;
bool doorOpen = false;
unsigned long doorOpenedAt = 0;
bool beepActive = false;
unsigned long beepStartedAt = 0;
unsigned long lastRfidPoll = 0;

// =========================================================
// FEATURE 3 HELPERS
// =========================================================

bool isAuthorized() {
  if (rfid.uid.size != 4) return false;
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != authorizedUID[i]) return false;
  }
  return true;
}

void printUID() {
  Serial.print("UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) Serial.print("0");
    Serial.print(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) Serial.print(":");
  }
  Serial.println();
}

void resetRFID() {
  rfid.PCD_Reset();
  delay(100);
  rfid.PCD_Init();
  delay(200);
  rfid.PCD_AntennaOn();
  delay(100);
}

// =========================================================
// FEATURE 1: PIR LIGHT CONTROL
// =========================================================

void handlePIR() {
  if (digitalRead(PIR) == HIGH) {
    lastMotion = millis();
    if (!ledState) {
      ledState = true;
      digitalWrite(LED, HIGH);
      Serial.println("Movement detected -> LED ON");
    }
  }

  if (ledState && millis() - lastMotion >= lightDelayTime) {
    ledState = false;
    digitalWrite(LED, LOW);
    Serial.println("No movement for 30 seconds -> LED OFF");
  }
}

// =========================================================
// FEATURE 2: SMOKE + FIRE SAFETY
// =========================================================

void handleSafety() {
  if (millis() - lastSensorRead < sensorInterval) return;
  lastSensorRead = millis();

  int smokeValue = analogRead(MQ2);
  int flameValue = analogRead(FLAME);

  Serial.print("MQ-2: ");
  Serial.print(smokeValue);
  Serial.print(" | Flame: ");
  Serial.println(flameValue);

  // Fire has highest priority
  if (flameValue < flameThreshold) {
    Serial.println("FIRE DETECTED!");
    Serial.println("Fire priority -> Pump ON + Buzzer ON");
    digitalWrite(FAN_RELAY, HIGH);   // Fan OFF
    digitalWrite(PUMP_RELAY, LOW);   // Pump ON
    digitalWrite(BUZZER, LOW);       // Buzzer ON
  }
  else if (smokeValue > smokeThreshold) {
    Serial.println("SMOKE DETECTED!");
    Serial.println("Smoke -> Fan ON + Buzzer ON");
    digitalWrite(FAN_RELAY, LOW);    // Fan ON
    digitalWrite(PUMP_RELAY, HIGH);  // Pump OFF
    digitalWrite(BUZZER, LOW);       // Buzzer ON
  }
  else {
    digitalWrite(FAN_RELAY, HIGH);
    digitalWrite(PUMP_RELAY, HIGH);
    digitalWrite(BUZZER, HIGH);
    Serial.println("Environment normal");
  }
}

// =========================================================
// FEATURE 3: RFID DOOR ACCESS
// =========================================================

void handleDoorTimer() {
  // Close the door after doorOpenTime (non-blocking)
  if (doorOpen && millis() - doorOpenedAt >= doorOpenTime) {
    Serial.println("Closing door...");
    doorServo.write(0);
    Serial.println("Door CLOSED");
    doorOpen = false;
  }

  // Stop the denied beep after deniedBeepTime (non-blocking)
  if (beepActive && millis() - beepStartedAt >= deniedBeepTime) {
    digitalWrite(ACCESS_BUZZER, HIGH);
    beepActive = false;
  }
}

void handleRFID() {
  if (millis() - lastRfidPoll < rfidPollInterval) return;
  lastRfidPoll = millis();

  // Wait until card is removed
  if (waitingForRemoval) {
    if (!rfid.PICC_IsNewCardPresent()) {
      waitingForRemoval = false;
      Serial.println();
      Serial.println("Card removed.");
      Serial.println("Ready for next scan.");
      Serial.println();
      resetRFID();
    }
    return;
  }

  // Check for new card
  if (!rfid.PICC_IsNewCardPresent()) return;

  Serial.println();
  Serial.println("CARD DETECTED");

  if (!rfid.PICC_ReadCardSerial()) {
    Serial.println("UID READ FAILED");
    resetRFID();
    return;
  }

  printUID();

  if (isAuthorized()) {
    Serial.println("ACCESS GRANTED");
    Serial.println("Authorized card!");
    Serial.println("Opening door...");
    doorServo.write(90);
    Serial.println("Door OPEN");
    doorOpen = true;
    doorOpenedAt = millis();
  }
  else {
    Serial.println("ACCESS DENIED");
    Serial.println("Unauthorized card.");
    Serial.println("Servo will NOT move.");
    digitalWrite(ACCESS_BUZZER, LOW);   // short beep
    beepActive = true;
    beepStartedAt = millis();
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  Serial.println();
  Serial.println("Remove card from reader.");
  waitingForRemoval = true;
}

// =========================================================
// SETUP
// =========================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Feature 1 + 2 pins
  pinMode(PIR, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(MQ2, INPUT);
  pinMode(FLAME, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(FAN_RELAY, OUTPUT);
  pinMode(PUMP_RELAY, OUTPUT);
  pinMode(ACCESS_BUZZER, OUTPUT);

  // Everything OFF at start
  digitalWrite(LED, LOW);
  digitalWrite(BUZZER, HIGH);
  digitalWrite(ACCESS_BUZZER, HIGH);
  digitalWrite(FAN_RELAY, HIGH);
  digitalWrite(PUMP_RELAY, HIGH);

  // Feature 3: RFID + servo
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();
  delay(500);
  rfid.PCD_AntennaOn();

  doorServo.setPeriodHertz(50);
  doorServo.attach(SERVO_PIN, 500, 2400);
  doorServo.write(0);   // Door closed

  Serial.println();
  Serial.println("================================");
  Serial.println("       SMART LAB STARTED");
  Serial.println("================================");
  rfid.PCD_DumpVersionToSerial();
  Serial.println("System ready. Place RFID card on reader.");
  Serial.println();
}

// =========================================================
// MAIN LOOP
// =========================================================

void loop() {
  handlePIR();        // Feature 1
  handleSafety();     // Feature 2
  handleRFID();       // Feature 3
  handleDoorTimer();  // Feature 3 timers
}
