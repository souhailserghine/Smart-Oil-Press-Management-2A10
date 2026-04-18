#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// UNO wiring:
// Fingerprint sensor TX -> D2 (UNO RX software serial)
// Fingerprint sensor RX -> D3 (UNO TX software serial)
SoftwareSerial fingerSerial(2, 3);
Adafruit_Fingerprint finger(&fingerSerial);

// Typical LCD I2C address is 0x27 (sometimes 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

static bool loginMode = true;
static uint32_t lastScanMs = 0;
static int lastReportedId = -1;

String serialLine;

uint8_t waitImage(uint16_t timeoutMs = 10000) {
  uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    uint8_t p = finger.getImage();
    if (p == FINGERPRINT_OK) return p;
    if (p != FINGERPRINT_NOFINGER) return p;
    delay(50);
  }
  return FINGERPRINT_NOFINGER;
}

int findFreeId() {
  for (int id = 1; id <= 127; ++id) {
    if (finger.loadModel(id) != FINGERPRINT_OK) {
      return id; // slot appears free
    }
  }
  return -1;
}

bool enrollAtId(int id) {
  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Enroll ID:"); lcd.print(id);
  lcd.setCursor(0, 1); lcd.print("Place finger");

  if (waitImage() != FINGERPRINT_OK) return false;
  if (finger.image2Tz(1) != FINGERPRINT_OK) return false;

  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Remove finger");
  delay(1500);

  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Place again");
  if (waitImage() != FINGERPRINT_OK) return false;
  if (finger.image2Tz(2) != FINGERPRINT_OK) return false;

  if (finger.createModel() != FINGERPRINT_OK) return false;
  if (finger.storeModel(id) != FINGERPRINT_OK) return false;

  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Enroll success");
  lcd.setCursor(0, 1); lcd.print("ID:"); lcd.print(id);
  return true;
}

int scanFingerprintOnce() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -2;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return -3;

  return finger.fingerID;
}

void sendLine(const String& s) {
  Serial.println(s);
}

void handleCommand(const String& cmdRaw) {
  String cmd = cmdRaw;
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd == "PING") {
    sendLine("PONG");
    return;
  }

  if (cmd == "LOGIN_ON") {
    loginMode = true;
    lcd.clear(); lcd.setCursor(0, 0); lcd.print("Login mode ON");
    sendLine("OK");
    return;
  }

  if (cmd == "LOGIN_OFF") {
    loginMode = false;
    lcd.clear(); lcd.setCursor(0, 0); lcd.print("Login mode OFF");
    sendLine("OK");
    return;
  }

  if (cmd == "ENROLL") {
    loginMode = false;
    int id = findFreeId();
    if (id < 0) {
      sendLine("ENROLL_FAIL:NO_SLOT");
      lcd.clear(); lcd.setCursor(0, 0); lcd.print("No free slot");
      return;
    }
    bool ok = enrollAtId(id);
    if (ok) sendLine("ENROLL_OK:" + String(id));
    else sendLine("ENROLL_FAIL");
    return;
  }

  if (cmd.startsWith("ENROLL:")) {
    loginMode = false;
    int id = cmd.substring(7).toInt();
    if (id < 1 || id > 127) {
      sendLine("ENROLL_FAIL:BAD_ID");
      return;
    }
    bool ok = enrollAtId(id);
    if (ok) sendLine("ENROLL_OK:" + String(id));
    else sendLine("ENROLL_FAIL");
    return;
  }

  if (cmd.startsWith("DELETE:")) {
    int id = cmd.substring(7).toInt();
    uint8_t p = finger.deleteModel(id);
    if (p == FINGERPRINT_OK) sendLine("DELETE_OK:" + String(id));
    else sendLine("DELETE_FAIL:" + String(id));
    return;
  }

  if (cmd.startsWith("NAME:")) {
    String name = cmd.substring(5);
    name.trim();
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Welcome");
    lcd.setCursor(0, 1); lcd.print(name.substring(0, 16));
    return;
  }

  if (cmd == "DENIED") {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Access denied");
    lcd.setCursor(0, 1); lcd.print("Unknown finger");
    return;
  }

  sendLine("ERR:UNKNOWN_CMD");
}

void setup() {
  Serial.begin(115200);
  finger.begin(57600);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Starting...");

  if (finger.verifyPassword()) {
    lcd.setCursor(0, 1); lcd.print("Sensor OK");
    sendLine("READY");
  } else {
    lcd.setCursor(0, 1); lcd.print("Sensor FAIL");
    sendLine("ERR:SENSOR");
  }

  delay(1200);
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Place finger");
  lcd.setCursor(0, 1); lcd.print("for login");
}

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialLine.length()) {
        handleCommand(serialLine);
        serialLine = "";
      }
    } else {
      serialLine += c;
    }
  }

  if (!loginMode) return;

  if (millis() - lastScanMs < 250) return;
  lastScanMs = millis();

  int id = scanFingerprintOnce();
  if (id > 0) {
    if (id != lastReportedId) {
      lastReportedId = id;
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("Fingerprint ID:");
      lcd.setCursor(0, 1); lcd.print(id);
      sendLine("MATCH:" + String(id)); // Qt must verify in DB and reply NAME:<...> or DENIED
    }
  } else if (id == -1) {
    lastReportedId = -1; // reset when no finger present
  }
}
