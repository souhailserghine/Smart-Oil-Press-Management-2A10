#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============ CONFIG ============
#define RX_PIN 2
#define TX_PIN 3
#define BAUD_SERIAL   115200
#define BAUD_FINGER   57600
#define LCD_ADDR 0x27

// ============ HARDWARE ============
SoftwareSerial fingerSerial(RX_PIN, TX_PIN);
Adafruit_Fingerprint finger(&fingerSerial);
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// ============ STATE ============
bool scanMode = false;
uint32_t lastScanTime = 0;
int lastMatchId = -1;
String commandBuffer;

// ============ SETUP ============
void setup() {
  Serial.begin(BAUD_SERIAL);
  finger.begin(BAUD_FINGER);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  delay(500);
  
  // Test sensor
  if (!finger.verifyPassword()) {
    Serial.println("ERR:SENSOR");
    lcd.setCursor(0, 0);
    lcd.print("Sensor FAIL");
    return;
  }
  
  // Boot OK
  Serial.println("READY");
  lcd.setCursor(0, 0);
  lcd.print("Ready");
  
  // Get template count
  finger.getTemplateCount();
  Serial.print("TEMPLATES:");
  Serial.println(finger.templateCount);
  
  delay(1000);
  enableScanning();
}

// ============ MAIN LOOP ============
void loop() {
  // Handle commands from Qt
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (commandBuffer.length() > 0) {
        processCommand(commandBuffer);
        commandBuffer = "";
      }
    } else if (c >= 32 && c <= 126) {  // Only printable ASCII
      commandBuffer += c;
    }
  }
  
  // Scan for fingerprints if enabled
  if (!scanMode) return;
  
  // Scan frequently for fast recognition (50ms interval = 20 scans/sec)
  if (millis() - lastScanTime < 50) return;
  lastScanTime = millis();
  
  scanFingerprint();
}

// ============ COMMAND HANDLER ============
void processCommand(const String& cmd) {
  String trimmed = cmd;
  trimmed.trim();
  
  if (trimmed == "LOGIN_ON") {
    enableScanning();
    Serial.println("OK");
    return;
  }
  
  if (trimmed == "LOGIN_OFF") {
    disableScanning();
    Serial.println("OK");
    return;
  }
  
  if (trimmed == "PING") {
    Serial.println("PONG");
    return;
  }
  
  if (trimmed.startsWith("NAME:")) {
    String name = trimmed.substring(5);
    name.trim();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Welcome");
    lcd.setCursor(0, 1);
    lcd.print(name.substring(0, 16));
    return;
  }
  
  if (trimmed == "DENIED") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Denied");
    delay(1500);
    enableScanning();
    return;
  }
  
  if (trimmed.startsWith("ENROLL:")) {
    int id = trimmed.substring(7).toInt();
    if (id > 0 && id <= 127) {
      enrollFingerprint(id);
    }
    return;
  }
  
  if (trimmed == "ENROLL") {
    int id = findFreeSlot();
    if (id > 0) {
      enrollFingerprint(id);
    } else {
      Serial.println("ENROLL_FAIL:NO_SLOT");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("No free slot");
    }
    return;
  }
  
  if (trimmed.startsWith("DELETE:")) {
    int id = trimmed.substring(7).toInt();
    if (finger.deleteModel(id) == FINGERPRINT_OK) {
      Serial.print("DELETE_OK:");
      Serial.println(id);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Deleted:");
      lcd.setCursor(9, 0);
      lcd.print(id);
    } else {
      Serial.print("DELETE_FAIL:");
      Serial.println(id);
    }
    delay(1000);
    enableScanning();
    return;
  }
}

// ============ FINGERPRINT SCANNING ============
void scanFingerprint() {
  uint8_t p = finger.getImage();
  
  // No finger
  if (p == FINGERPRINT_NOFINGER) {
    lastMatchId = -1;
    return;
  }
  
  // Error
  if (p != FINGERPRINT_OK) {
    return;
  }
  
  // Convert to template
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    return;
  }
  
  // Search database
  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) {
    return;  // No match found
  }
  
  // Match found!
  int matchId = finger.fingerID;
  
  // Only report once per match
  if (matchId == lastMatchId) {
    return;
  }
  
  lastMatchId = matchId;
  
  // Send to Qt
  Serial.print("MATCH:");
  Serial.println(matchId);
  
  // Show on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Match ID:");
  lcd.setCursor(10, 0);
  lcd.print(matchId);
}

// ============ ENROLLMENT ============
void enrollFingerprint(int id) {
  scanMode = false;
  
  // First capture
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enroll ID:");
  lcd.setCursor(10, 0);
  lcd.print(id);
  lcd.setCursor(0, 1);
  lcd.print("Place finger");
  
  if (!waitForImage(5000)) {
    Serial.println("ENROLL_FAIL:TIMEOUT_1");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Timeout 1");
    delay(1000);
    enableScanning();
    return;
  }
  
  uint8_t p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("ENROLL_FAIL:IMAGE2TZ_1");
    enableScanning();
    return;
  }
  
  // Remove finger and wait (1 second is plenty)
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Remove finger");
  
  uint32_t start = millis();
  while (millis() - start < 1000) {
    if (finger.getImage() == FINGERPRINT_NOFINGER) {
      break;
    }
    delay(50);
  }
  
  // Second capture
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place again");
  
  if (!waitForImage(5000)) {
    Serial.println("ENROLL_FAIL:TIMEOUT_2");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Timeout 2");
    delay(1000);
    enableScanning();
    return;
  }
  
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("ENROLL_FAIL:IMAGE2TZ_2");
    enableScanning();
    return;
  }
  
  // Create and store
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("ENROLL_FAIL:CREATE_MODEL");
    enableScanning();
    return;
  }
  
  p = finger.storeModel(id);
  if (p != FINGERPRINT_OK) {
    Serial.println("ENROLL_FAIL:STORE_MODEL");
    enableScanning();
    return;
  }
  
  // Success
  Serial.print("ENROLL_OK:");
  Serial.println(id);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Success ID:");
  lcd.setCursor(11, 0);
  lcd.print(id);
  
  delay(2000);
  enableScanning();
}

// ============ HELPERS ============
bool waitForImage(uint32_t timeoutMs) {
  uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    if (finger.getImage() == FINGERPRINT_OK) {
      return true;
    }
    delay(10);  // Check every 10ms instead of 50ms
  }
  return false;
}

int findFreeSlot() {
  for (int i = 1; i <= 127; i++) {
    if (finger.loadModel(i) != FINGERPRINT_OK) {
      return i;
    }
  }
  return -1;
}

void enableScanning() {
  scanMode = true;
  lastMatchId = -1;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scanning...");
  lcd.setCursor(0, 1);
  lcd.print("Place finger");
}

void disableScanning() {
  scanMode = false;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scanning OFF");
}
