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
static uint32_t lastReportedAtMs = 0;
static uint32_t lastNoMatchAtMs = 0;
static String lastEnrollError;

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

bool waitFingerRelease(uint16_t timeoutMs = 10000) {
  uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    uint8_t p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) return true;
    delay(50);
  }
  return false;
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
  lastEnrollError = "";

  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Enroll ID:"); lcd.print(id);
  lcd.setCursor(0, 1); lcd.print("Place finger");

  uint8_t p = waitImage(15000);
  if (p != FINGERPRINT_OK) {
    lastEnrollError = String("TIMEOUT_1:") + String(p);
    return false;
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    lastEnrollError = String("IMAGE2TZ_1:") + String(p);
    return false;
  }

  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Remove finger");
  if (!waitFingerRelease(10000)) {
    lastEnrollError = "REMOVE_TIMEOUT";
    return false;
  }

  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Place again");
  p = waitImage(15000);
  if (p != FINGERPRINT_OK) {
    lastEnrollError = String("TIMEOUT_2:") + String(p);
    return false;
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    lastEnrollError = String("IMAGE2TZ_2:") + String(p);
    return false;
  }

  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    lastEnrollError = String("CREATE_MODEL:") + String(p);
    return false;
  }

  p = finger.storeModel(id);
  if (p != FINGERPRINT_OK) {
    lastEnrollError = String("STORE_MODEL:") + String(p);
    return false;
  }

  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Enroll success");
  lcd.setCursor(0, 1); lcd.print("ID:"); lcd.print(id);
  return true;
}

int scanFingerprintOnce() {
  // Wait a short window so quick finger placement is still captured.
  uint8_t p = waitImage(500);
  if (p == FINGERPRINT_NOFINGER) return -1;
  if (p != FINGERPRINT_OK) return -2;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -2;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) {
    // Fallback to the regular (slower) search for improved tolerance.
    p = finger.fingerSearch();
    if (p != FINGERPRINT_OK) return -3;
  }

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
    if (!loginMode) {
      loginMode = true;
      lcd.clear(); lcd.setCursor(0, 0); lcd.print("Place finger");
      lcd.setCursor(0, 1); lcd.print("for login");
      sendLine("OK");
    }
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
    else {
      if (lastEnrollError.length()) sendLine("ENROLL_FAIL:" + lastEnrollError);
      else sendLine("ENROLL_FAIL");
    }
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
    else {
      if (lastEnrollError.length()) sendLine("ENROLL_FAIL:" + lastEnrollError);
      else sendLine("ENROLL_FAIL");
    }
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

  if (millis() - lastScanMs < 220) return;
  lastScanMs = millis();

  int id = scanFingerprintOnce();
  if (id > 0) {
    // Emit immediately on first valid match, but rate-limit repeats for same ID.
    const bool sameRecent = (id == lastReportedId) && ((millis() - lastReportedAtMs) < 2000);
    if (!sameRecent) {
      lastReportedId = id;
      lastReportedAtMs = millis();
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("Fingerprint ID:");
      lcd.setCursor(0, 1); lcd.print(id);
      sendLine("MATCH:" + String(id)); // Qt verifies in DB and replies NAME:<...> or DENIED
    }
  } else if (id == -1) {
    // Finger removed: allow immediate re-report on next touch.
    lastReportedId = -1;
  } else {
    // Finger detected but not matched/decoded: provide throttled feedback.
    if (millis() - lastNoMatchAtMs > 1500) {
      lastNoMatchAtMs = millis();
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("No match");
      lcd.setCursor(0, 1); lcd.print("Try again");
      sendLine("NO_MATCH");
    }
  }
}
