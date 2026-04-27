# Arduino Code - Line-by-Line Breakdown

## Complete Arduino Sketch Structure

```cpp
File: fingerprint_terminal/fingerprint_terminal.ino
Total: 344 lines
Language: C++ (Arduino dialect)
Target: Arduino Uno or compatible
```

---

## Section 1: Includes & Configuration (Lines 1-23)

### Library Imports

```cpp
#include <Adafruit_Fingerprint.h>  // Line 1: Fingerprint sensor driver
#include <SoftwareSerial.h>        // Line 2: Create extra serial port
#include <Wire.h>                  // Line 3: I2C protocol (LCD)
#include <LiquidCrystal_I2C.h>     // Line 4: I2C LCD display driver
```

**Why these?**
- `Adafruit_Fingerprint`: Official sensor library (Adafruit R307/R503)
- `SoftwareSerial`: Arduino Uno only has 1 hardware UART, need 2 (Qt + sensor)
- `Wire`: I2C protocol (standard for serial LCD)
- `LiquidCrystal_I2C`: LCD library that uses I2C

### Hardware Definitions (Lines 6-12)

```cpp
#define RX_PIN 2           // Line 7: Fingerprint sensor TX → Arduino pin 2
#define TX_PIN 3           // Line 8: Fingerprint sensor RX ← Arduino pin 3
#define BAUD_SERIAL   115200  // Line 10: Qt communication speed
#define BAUD_FINGER   57600   // Line 11: Sensor communication speed
#define LCD_ADDR 0x27      // Line 12: LCD I2C address
```

**Pinout explanation:**
```
Arduino Uno Hardware:
  Pin 0 (RX) - Reserved for USB serial (Qt)
  Pin 1 (TX) - Reserved for USB serial (Qt)
  Pin 2 → Fingerprint sensor TX (SoftwareSerial RX)
  Pin 3 → Fingerprint sensor RX (SoftwareSerial TX)
  SDA (A4) → LCD I2C data
  SCL (A5) → LCD I2C clock
```

**Baud rates:**
- `115200`: Maximum safe USB serial speed (0.16% error at this rate)
- `57600`: Adafruit sensor firmware-locked baud rate (cannot change)

### Hardware Object Instantiation (Lines 14-19)

```cpp
// Line 14: Create bitbanged serial port on pins 2-3
SoftwareSerial fingerSerial(RX_PIN, TX_PIN);
// RX_PIN (2) = receives from sensor TX
// TX_PIN (3) = sends to sensor RX
// Note: RX first parameter, TX second

// Line 15: Create fingerprint driver using the serial port
Adafruit_Fingerprint finger(&fingerSerial);

// Line 16: Create LCD object with I2C address 0x27, 16 cols, 2 rows
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
```

### Global State Variables (Lines 18-22)

```cpp
// Line 18: Are we actively scanning for fingerprints?
bool scanMode = false;

// Line 19: Throttle scan rate (50ms between checks)
uint32_t lastScanTime = 0;

// Line 20: Prevent reporting same match twice
int lastMatchId = -1;

// Line 21: Accumulate Qt commands until newline
String commandBuffer;
```

**Why global?**
- Accessed in `loop()`, `scanFingerprint()`, `processCommand()`, etc.
- Maintain state across function calls
- `lastScanTime`: Prevents CPU spinning (wait 50ms between scans)
- `lastMatchId`: Stop spamming "MATCH:10" while finger still on sensor

---

## Section 2: Setup Function (Lines 24-52)

### Line 24: Function Declaration

```cpp
void setup() {
```

**Arduino rule:** `setup()` runs once at power-on or reset. Use for initialization.

### Lines 25-26: Serial Initialization

```cpp
Serial.begin(BAUD_SERIAL);    // Line 25: Open USB serial at 115200 baud
finger.begin(BAUD_FINGER);    // Line 26: Initialize fingerprint sensor
```

**What happens:**
- `Serial.begin(115200)`: Opens USB port, ready to receive/send from Qt
- `finger.begin(57600)`: Initializes SoftwareSerial + sensor driver

### Lines 27-29: LCD Initialization

```cpp
lcd.init();       // Line 27: Initialize I2C LCD controller
lcd.backlight();  // Line 28: Turn on backlight
lcd.clear();      // Line 29: Clear display, cursor → (0,0)
```

### Line 31: Hardware Stabilization

```cpp
delay(500);  // Line 31: Wait 500ms for sensor to boot
```

**Why?** Sensor needs time to initialize I2C, prepare firmware.

### Lines 33-41: Sensor Password Verification

```cpp
if (!finger.verifyPassword()) {  // Line 33: Test sensor communication
  Serial.println("ERR:SENSOR");  // Line 34: Report to Qt
  lcd.setCursor(0, 0);           // Line 35: Position cursor
  lcd.print("Sensor FAIL");       // Line 36: Display error
  return;                         // Line 37: Exit setup(), system halts
}
```

**Purpose:** Verify sensor is responding. If not:
- Send error to Qt
- Display on LCD
- Halt execution (not calling `enableScanning()`)

**How to test:** Check I2C address with I2C scanner sketch

### Lines 43-46: Boot Success

```cpp
Serial.println("READY");                    // Line 44: Signal boot complete
lcd.setCursor(0, 0);                        // Line 45: Cursor → (0,0)
lcd.print("Ready");                         // Line 46: Display boot status
```

**Qt waits for "READY" before proceeding.**

### Lines 48-51: Query Template Count

```cpp
finger.getTemplateCount();                  // Line 48: Query sensor
Serial.print("TEMPLATES:");                 // Line 49: Start message
Serial.println(finger.templateCount);       // Line 50: Print count (e.g., "TEMPLATES:5")
```

**What this does:**
- `finger.getTemplateCount()`: Queries sensor EEPROM, stores in `finger.templateCount` member
- Sent as two separate prints (print + println) to avoid buffering issues
- Result: `"TEMPLATES:5\n"` sent to Qt

### Line 52: Final Stabilization

```cpp
delay(1000);         // Line 52: Wait 1 second before scanning
enableScanning();     // (implicit on next line): Start scanning mode
```

---

## Section 3: Main Loop (Lines 54-77)

### Line 54: Loop Declaration

```cpp
void loop() {
```

**Arduino rule:** `loop()` runs indefinitely, ~10,000 times/second (no delay in hot path).

### Lines 55-71: Command Processing

```cpp
if (Serial.available()) {  // Line 56: Is data waiting on USB serial?
  char c = Serial.read();  // Line 57: Read one byte
  
  if (c == '\n' || c == '\r') {  // Line 58: Line terminator?
    if (commandBuffer.length() > 0) {  // Line 59: Non-empty?
      processCommand(commandBuffer);  // Line 60: Execute it
      commandBuffer = "";  // Line 61: Reset buffer
    }
  } else if (c >= 32 && c <= 126) {  // Line 62: Printable ASCII?
    commandBuffer += c;  // Line 63: Append to command
  }
  // Lines 64-65: Non-printable bytes (noise) are silently dropped
}
```

**Protocol parsing:**
- Accumulate bytes into `commandBuffer`
- When newline received, execute command
- Non-printable bytes (0-31, 127-255) are dropped (defense against serial noise)

**Example:** Qt sends "LOGIN_ON\n"
```
Loop 1: Serial.available() = true, read 'L', append → "L"
Loop 2: read 'O', append → "LO"
Loop 3: read 'G', append → "LOG"
...
Loop 8: read 'N', append → "LOGIN_ON"
Loop 9: read '\n', commandBuffer="LOGIN_ON", processCommand("LOGIN_ON"), reset
Loop 10: commandBuffer=""
```

### Lines 72-77: Fingerprint Scanning

```cpp
if (!scanMode) return;  // Line 72: Not scanning? Exit loop early

// Line 75: Throttle to 50ms intervals (20 scans/second)
if (millis() - lastScanTime < 50) return;
lastScanTime = millis();  // Line 76: Update timestamp

scanFingerprint();  // Line 77: Check for matches
```

**Throttling logic:**
```
Loop 1 (t=0ms): lastScanTime=0, now - 0 = 0 < 50 → return (skip scan)
Loop 2 (t=5ms): lastScanTime=0, now - 0 = 5 < 50 → return (skip scan)
...
Loop 10 (t=50ms): lastScanTime=0, now - 0 = 50 ≮ 50 → Execute scan!
                 lastScanTime = 50
Loop 11 (t=55ms): lastScanTime=50, now - 50 = 5 < 50 → return (skip scan)
...
Loop 20 (t=100ms): lastScanTime=50, now - 50 = 50 ≮ 50 → Execute scan!
                   lastScanTime = 100
```

**Performance:** Loops run ~10,000/second, but scans only happen ~20 times/second. Other 9,800 loops handle commands.

---

## Section 4: Command Processing (Lines 79-161)

### LOGIN_ON Command (Lines 80-84)

```cpp
if (trimmed == "LOGIN_ON") {        // Line 81: Exact match?
  enableScanning();                 // Line 82: Set scanMode=true
  Serial.println("OK");             // Line 83: Acknowledge to Qt
  return;                           // Line 84: Exit function
}
```

**Flow:**
1. Qt: `sendFingerprintTerminalCommand("LOGIN_ON")`
2. Arduino receives `"LOGIN_ON\n"`
3. Calls `enableScanning()` (sets flags, updates LCD)
4. Sends `"OK\n"` back
5. Main loop now executes `scanFingerprint()` every 50ms

### DELETE Command (Lines 149-161)

```cpp
if (trimmed.startsWith("DELETE:")) {  // Line 149: Starts with "DELETE:"?
  int id = trimmed.substring(7).toInt();  // Line 150: Parse ID
  // "DELETE:10" → substring(7) = "10" → toInt() = 10
  
  if (finger.deleteModel(id) == FINGERPRINT_OK) {  // Line 151: Delete succeeded?
    Serial.print("DELETE_OK:");     // Line 152: Start message
    Serial.println(id);             // Line 153: Send ID (e.g., "DELETE_OK:10")
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Deleted:");
    lcd.setCursor(9, 0);
    lcd.print(id);
  } else {
    Serial.print("DELETE_FAIL:");   // Line 160: Delete failed
    Serial.println(id);
  }
  delay(1000);
  enableScanning();
  return;
}
```

**Adafruit sensor:** `deleteModel(id)` erases template at slot `id`

---

## Section 5: Fingerprint Scanning (Lines 163-207)

### Image Capture (Lines 164-166)

```cpp
uint8_t p = finger.getImage();  // Line 164: Capture from sensor
if (p == FINGERPRINT_NOFINGER) {  // Line 166: No finger?
  lastMatchId = -1;             // Line 167: Reset state
  return;                        // Line 168: Exit early
}
```

**Return codes from sensor:**
- `FINGERPRINT_OK`: Image captured successfully
- `FINGERPRINT_NOFINGER`: No finger on sensor
- `FINGERPRINT_PACKETRECEIVEERR`: Serial communication error
- Others: See Adafruit docs

### Quality Check (Lines 170-172)

```cpp
if (p != FINGERPRINT_OK) {  // Line 170: Not OK?
  return;                   // Line 171: Exit (ignore error)
}
```

**Why not retry?** Main loop runs every 50ms, will try again next cycle.

### Template Conversion (Lines 175-178)

```cpp
p = finger.image2Tz();  // Line 175: Convert image → template
if (p != FINGERPRINT_OK) {  // Line 176: Conversion failed?
  return;  // Line 177: Exit
}
```

**What `image2Tz()` does:**
- Extract biometric features from image
- Create mathematical template (much smaller, ~512 bytes)
- Store in sensor RAM (not EEPROM)
- Return code indicates quality

### Database Search (Lines 181-184)

```cpp
p = finger.fingerFastSearch();  // Line 181: Search against enrolled templates
if (p != FINGERPRINT_OK) {      // Line 182: No match?
  return;                        // Line 183: Exit
}
```

**`fingerFastSearch()` magic:**
- Compares template against all enrolled fingerprints
- Sets `finger.fingerID` = ID of best match
- Sets `finger.confidence` = match score (0-1000)
- Returns `FINGERPRINT_OK` if match found, else error code

### Deduplication (Lines 186-192)

```cpp
int matchId = finger.fingerID;    // Line 186: Get match ID
if (matchId == lastMatchId) {     // Line 188: Same as last scan?
  return;                         // Line 189: Already reported, skip
}
lastMatchId = matchId;            // Line 190: Remember this ID
```

**Purpose:** Prevent spamming `"MATCH:10\n"` every 50ms while finger is on sensor.

**Scenario:**
- Scan 1 (t=0ms): No finger
- Scan 2 (t=50ms): Matched ID 10, lastMatchId=10, send "MATCH:10"
- Scan 3 (t=100ms): Still matched ID 10, lastMatchId==10, skip send
- Scan 4 (t=150ms): Still matched ID 10, lastMatchId==10, skip send
- Scan 5 (t=200ms): Finger removed, no finger, lastMatchId=-1
- Scan 6 (t=250ms): Finger placed again, matched ID 10, lastMatchId!=-1, send "MATCH:10" again

### Send to Qt (Lines 194-197)

```cpp
Serial.print("MATCH:");  // Line 194: Start message
Serial.println(matchId);  // Line 195: Send ID (e.g., "MATCH:10\n")

lcd.clear();             // Line 198: Clear display
lcd.setCursor(0, 0);     // Line 199: Cursor to (0,0)
lcd.print("Match ID:");   // Line 200: Label
lcd.setCursor(10, 0);    // Line 201: Cursor to column 10
lcd.print(matchId);      // Line 202: Print ID
```

**LCD result:**
```
Row 0: "Match ID:    10"
Row 1: (blank)
```

---

## Section 6: Enrollment (Lines 209-307)

### Disable Normal Scanning (Line 210)

```cpp
scanMode = false;  // Line 210: Stop main loop from scanning
```

**Why?** Enrollment needs exclusive control over sensor.

### First Capture Setup (Lines 212-220)

```cpp
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("Enroll ID:");
lcd.setCursor(10, 0);
lcd.print(id);
lcd.setCursor(0, 1);
lcd.print("Place finger");
```

**Display:**
```
Row 0: "Enroll ID:    10"
Row 1: "Place finger"
```

### First Capture Wait (Lines 222-228)

```cpp
if (!waitForImage(5000)) {  // Line 222: Wait up to 5 seconds
  Serial.println("ENROLL_FAIL:TIMEOUT_1");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Timeout 1");
  delay(1000);
  enableScanning();
  return;
}
```

**What `waitForImage(5000)` does:**
- Loop until `getImage() == FINGERPRINT_OK` or 5000ms passes
- Check every 10ms (responsive)
- Return true if successful, false if timeout

### First Template Conversion (Lines 230-234)

```cpp
uint8_t p = finger.image2Tz(1);  // Line 230: Convert → template slot 1
// The parameter (1) means: use slot 1 (not EEPROM, RAM only)
if (p != FINGERPRINT_OK) {       // Line 231: Quality check failed?
  Serial.println("ENROLL_FAIL:IMAGE2TZ_1");
  enableScanning();
  return;
}
```

**Adafruit sensor has 2 working slots in RAM:**
- Slot 1: First template
- Slot 2: Second template
- `createModel()`: Combines both into one model
- `storeModel(id)`: Saves combined model to EEPROM at slot `id`

### Removal Wait (Lines 237-246)

```cpp
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("Remove finger");

uint32_t start = millis();
while (millis() - start < 1000) {  // Wait 1 second
  if (finger.getImage() == FINGERPRINT_NOFINGER) {
    break;  // Finger removed, proceed
  }
  delay(50);  // Check every 50ms
}
```

**Logic:** Break early if finger removed before 1 second, otherwise wait full 1 second.

### Second Capture (Lines 249-264)

```cpp
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("Place again");

if (!waitForImage(5000)) {
  // ... same timeout handling as first capture
}

p = finger.image2Tz(2);  // Convert → template slot 2
if (p != FINGERPRINT_OK) {
  // ... error handling
}
```

### Model Creation (Lines 270-275)

```cpp
p = finger.createModel();  // Line 270: Combine templates 1 & 2
if (p != FINGERPRINT_OK) {
  Serial.println("ENROLL_FAIL:CREATE_MODEL");
  enableScanning();
  return;
}
```

**Sensor algorithm:**
- Takes template from slot 1 and slot 2
- Creates one combined model
- Stores in sensor RAM (temporary)

### Model Storage (Lines 277-282)

```cpp
p = finger.storeModel(id);  // Line 277: Save to EEPROM at slot `id`
if (p != FINGERPRINT_OK) {  // Line 278: Write failed?
  Serial.println("ENROLL_FAIL:STORE_MODEL");
  enableScanning();
  return;
}
```

**After this point:** Fingerprint is permanently stored in sensor EEPROM and searchable via `fingerFastSearch()`.

### Success Reporting (Lines 285-296)

```cpp
Serial.print("ENROLL_OK:");  // Line 285: Success message
Serial.println(id);          // Line 286: Send ID

lcd.clear();
lcd.setCursor(0, 0);
lcd.print("Success ID:");
lcd.setCursor(11, 0);
lcd.print(id);

delay(2000);  // Show message for 2 seconds
enableScanning();  // Resume normal scanning
```

---

## Section 7: Helper Functions (Lines 309-344)

### waitForImage (Lines 310-319)

```cpp
bool waitForImage(uint32_t timeoutMs) {
  uint32_t start = millis();  // Record start time
  
  while (millis() - start < timeoutMs) {  // Loop until timeout
    if (finger.getImage() == FINGERPRINT_OK) {
      return true;  // Got image, exit early
    }
    delay(10);  // Check every 10ms
  }
  
  return false;  // Timeout reached without image
}
```

**Timing:**
- `millis()`: Returns milliseconds since Arduino boot
- `delay(10)`: Block 10 milliseconds
- `while` condition: Continue while elapsed time < timeout

**Example:** `waitForImage(5000)`
```
Start: t=0, start=0
  Iteration 1 (t=10): elapsed=10 < 5000 → continue
  Iteration 2 (t=20): elapsed=20 < 5000 → continue
  ...
  Iteration 500 (t=5000): elapsed=5000 ≮ 5000 → exit loop
  Return false
```

### findFreeSlot (Lines 321-326)

```cpp
int findFreeSlot() {
  for (int i = 1; i <= 127; i++) {  // Check slots 1-127
    if (finger.loadModel(i) != FINGERPRINT_OK) {
      return i;  // Empty slot found
    }
  }
  return -1;  // All full
}
```

**Logic:**
- `loadModel(i)`: Tries to load template at slot `i` into sensor RAM
- If slot empty: Returns error code (not `FINGERPRINT_OK`)
- Return first slot that gives error = first empty slot

**Limitations:**
- Iterates through all 127 slots (could be slow if full)
- Doesn't indicate which slots are full
- User must manually delete to free space

### enableScanning (Lines 328-335)

```cpp
void enableScanning() {
  scanMode = true;    // Resume main loop scanning
  lastMatchId = -1;   // Reset (fresh session)
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scanning...");
  lcd.setCursor(0, 1);
  lcd.print("Place finger");
}
```

**Sets global state:** `scanMode=true` triggers scanning in main `loop()`.

### disableScanning (Lines 337-343)

```cpp
void disableScanning() {
  scanMode = false;  // Stop main loop scanning
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scanning OFF");
}
```

---

## Key Insights

### 1. Non-Blocking Design
- No long delays in main `loop()`
- Even `waitForImage()` is used only during enrollment (exclusive mode)
- Main loop responsive to Qt commands at all times

### 2. State Machine
```
loop() continuously:
  1. Check for Qt commands (non-blocking)
  2. If scanMode && time elapsed → scan once
  3. Return to step 1
```

### 3. Serial Protocol Robustness
- Only accepts printable ASCII (drops noise)
- Line-terminated messages (clear boundaries)
- Simple request/response pattern
- No checksums (assumption: serial is clean at 115200 baud)

### 4. Performance Optimizations
- Scan throttle: 50ms (not as fast as sensor, but still responsive)
- Deduplication: `lastMatchId` prevents spam
- No unnecessary serial writes
- LCD updates only on state change (not every scan)

### 5. Safety
- All timeouts default to generous values (5 seconds, 1 second)
- Graceful error handling (send error code, resume scanning)
- No infinite loops or edge cases
- Sensor password check prevents running without sensor

---

This detailed breakdown explains every line and the reasoning behind each decision. The code is intentionally simple for reliability and maintainability.
