# Adafruit Library Functions Reference

## Adafruit_Fingerprint Library - Official Pattern Functions

### 1. **Initialization & Verification**

```cpp
// Initialize fingerprint sensor at specific baud rate
finger.begin(57600);

// Verify sensor password (check if sensor is responding)
if (finger.verifyPassword()) {
  Serial.println("Sensor connected!");
} else {
  Serial.println("Sensor not found!");
}
```

---

### 2. **Image Capture**

```cpp
// Capture fingerprint image from sensor
uint8_t p = finger.getImage();

// Return values:
// FINGERPRINT_OK       - Image captured successfully
// FINGERPRINT_NOFINGER - No finger detected
// Other codes          - Sensor errors
```

**Adafruit Pattern:**
```cpp
uint8_t p = finger.getImage();
if (p == FINGERPRINT_OK) {
  // Process image
} else if (p == FINGERPRINT_NOFINGER) {
  // No finger, wait and try again
} else {
  // Sensor error
}
```

---

### 3. **Image to Template Conversion**

```cpp
// Convert captured image to template (characteristic vector)
// slot = 1 or 2 (for enrollment comparison)
uint8_t p = finger.image2Tz(1);  // First image

if (p == FINGERPRINT_OK) {
  // Template created successfully
}
```

**Enrollment Pattern (Official):**
```cpp
// First image
finger.getImage();
finger.image2Tz(1);

// [Remove finger and wait]

// Second image
finger.getImage();
finger.image2Tz(2);

// Now compare templates
finger.createModel();  // Combines both templates
```

---

### 4. **Fingerprint Matching (Official Pattern) ✅**

### **Method 1: fingerFastSearch() - RECOMMENDED**
```cpp
uint8_t p = finger.fingerFastSearch();

if (p == FINGERPRINT_OK) {
  uint16_t matchID = finger.fingerID;      // ID of matched print
  uint16_t confidence = finger.confidence; // Match quality (0-65535)
  
  Serial.print("Match found! ID: ");
  Serial.print(matchID);
  Serial.print(" Confidence: ");
  Serial.println(confidence);
} else {
  // No match found
}
```

**Why fingerFastSearch() is better:**
- ✅ Faster search algorithm
- ✅ Returns confidence score
- ✅ Recommended by Adafruit
- ✅ More reliable matching

### **Method 2: fingerSearch() - NOT RECOMMENDED**
```cpp
uint8_t p = finger.fingerSearch();
// Searches from page 0 to 127 (slower)
// Less recommended for continuous use
```

---

### 5. **Template Management**

#### Create Model
```cpp
// After capturing both images with image2Tz(1) and image2Tz(2)
uint8_t p = finger.createModel();

if (p == FINGERPRINT_OK) {
  // Model ready to store
} else {
  // Templates don't match well enough
}
```

#### Store Model
```cpp
// Store the model at specific ID
uint8_t p = finger.storeModel(id);  // id: 1-127

if (p == FINGERPRINT_OK) {
  Serial.println("Model stored successfully");
}
```

#### Load Model
```cpp
// Load existing model for deletion check
uint8_t p = finger.loadModel(id);

if (p == FINGERPRINT_OK) {
  // Slot is occupied
} else {
  // Slot is empty
}
```

#### Delete Model
```cpp
// Delete fingerprint from database
uint8_t p = finger.deleteModel(id);

if (p == FINGERPRINT_OK) {
  Serial.println("Fingerprint deleted");
}
```

---

### 6. **Database Information**

```cpp
// Get number of enrolled fingerprints
uint16_t count = finger.getTemplateCount();
Serial.print("Enrolled fingerprints: ");
Serial.println(count);

// Empty specific slot
finger.deleteModel(id);

// Empty entire database
finger.emptyDatabase();
```

---

## Official Enrollment Procedure (Adafruit Pattern)

```cpp
void enrollFingerprint(uint8_t id) {
  Serial.println("Place first finger...");
  
  // STEP 1: First image
  while (finger.getImage() != FINGERPRINT_OK) {
    delay(50);
  }
  
  if (finger.image2Tz(1) != FINGERPRINT_OK) {
    Serial.println("Image conversion failed");
    return;
  }
  
  Serial.println("Remove finger");
  delay(2000);
  
  // STEP 2: Wait for finger to be removed
  while (finger.getImage() != FINGERPRINT_NOFINGER) {
    delay(100);
  }
  
  Serial.println("Place same finger again...");
  
  // STEP 3: Second image
  while (finger.getImage() != FINGERPRINT_OK) {
    delay(50);
  }
  
  if (finger.image2Tz(2) != FINGERPRINT_OK) {
    Serial.println("Image conversion failed");
    return;
  }
  
  // STEP 4: Create model
  if (finger.createModel() != FINGERPRINT_OK) {
    Serial.println("Templates don't match");
    return;
  }
  
  // STEP 5: Store model
  if (finger.storeModel(id) == FINGERPRINT_OK) {
    Serial.println("Enrollment successful!");
  }
}
```

---

## Official Matching Procedure (Adafruit Pattern) ✅

```cpp
uint16_t getFingerprintID() {
  uint8_t p = finger.getImage();
  
  if (p != FINGERPRINT_OK) {
    return -1;  // No valid image
  }
  
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    return -1;  // Image conversion failed
  }
  
  // USE fingerFastSearch() - OFFICIAL PATTERN
  p = finger.fingerFastSearch();
  
  if (p == FINGERPRINT_OK) {
    return finger.fingerID;        // Return matched ID
  }
  
  return 0;  // No match found
}
```

---

## LiquidCrystal_I2C Library - Official Pattern

### Initialization
```cpp
// OFFICIAL PATTERN with error checking
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Address, cols, rows

void setup() {
  if (!lcd.init()) {
    Serial.println("LCD init failed!");
  }
  lcd.backlight();
  lcd.clear();
}
```

### Display Functions
```cpp
// Clear and position at (0,0)
lcd.clear();

// Set cursor position
lcd.setCursor(col, row);  // row: 0 or 1

// Print text
lcd.print("Hello");
lcd.print(value);

// Print number with padding
lcd.print(12345);

// Display backlight
lcd.backlight();
lcd.noBacklight();
```

### Recommended Wrapper Pattern
```cpp
// Helper functions for consistent usage
void lcdClear() {
  lcd.clear();
  lcd.setCursor(0, 0);
}

void lcdPrintLine(uint8_t row, const char* text) {
  lcd.setCursor(0, row);
  lcd.print(text);
}
```

---

## Return Codes Reference

### Adafruit_Fingerprint Constants
```cpp
#define FINGERPRINT_OK           0x00
#define FINGERPRINT_PACKETRECIEVEERROR 0x01
#define FINGERPRINT_NOFINGER     0x02
#define FINGERPRINT_IMAGEFAIL    0x03
#define FINGERPRINT_IMAGEMESS    0x06
#define FINGERPRINT_FEATUREFAIL  0x07
#define FINGERPRINT_NOMATCH      0x08
#define FINGERPRINT_NOTFOUND     0x09
#define FINGERPRINT_ENROLLMISMATCH 0x0A
#define FINGERPRINT_BADLOCATION  0x0B
#define FINGERPRINT_DBRANGEOUT   0x0C
#define FINGERPRINT_UPLOADFEATUREFAIL 0x0D
#define FINGERPRINT_PACKETRESPONSEFAIL 0x0E
#define FINGERPRINT_INVALIDIMAGE 0x0F
#define FINGERPRINT_FLASHERR     0x10
#define FINGERPRINT_INVALIDREG   0x11
#define FINGERPRINT_ADDRCODE     0x20
#define FINGERPRINT_PASSVERIFY   0x13
#define FINGERPRINT_TEMPLATENUM  0x14
#define FINGERPRINT_READTEMPLATE 0x15
#define FINGERPRINT_SETTEMPLATE  0x16
#define FINGERPRINT_GETTEMPLATENUM 0x17
#define FINGERPRINT_READINDEX    0x18
#define FINGERPRINT_TEMPLATEEXIST 0x19
#define FINGERPRINT_TRANSFERCANCEL 0x41
#define FINGERPRINT_DUPSEARCH    0x42
#define FINGERPRINT_ENROLLCOUNT  0x43
#define FINGERPRINT_CHECKSUM     0x44
#define FINGERPRINT_IMAGEMESSINESS 0x80
```

---

## Performance Notes

### fingerFastSearch() vs fingerSearch()
```cpp
// FAST (✅ Recommended)
finger.fingerFastSearch()    // Optimized binary search
                             // Returns in ~50-100ms
                             // Returns confidence score

// SLOW (❌ Not recommended)
finger.fingerSearch()        // Linear search from page 0
                             // Takes ~500-1000ms
                             // No confidence score
```

### Confidence Levels
```cpp
uint16_t confidence = finger.confidence;

// Interpretation:
// > 50000  : Strong match (reliable)
// 40000-50000 : Good match
// 30000-40000 : Acceptable match
// < 30000  : Weak match (consider re-enrollment)
```

---

## Complete Example: Enrollment + Matching (Official Pattern)

```cpp
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3);
Adafruit_Fingerprint finger(&mySerial);

void setup() {
  Serial.begin(115200);
  finger.begin(57600);
  
  if (finger.verifyPassword()) {
    Serial.println("Sensor found!");
  } else {
    Serial.println("Sensor not found!");
    while (1);
  }
}

void loop() {
  // Example: Enroll fingerprint 5
  enrollUser(5);
  
  // Then try to match
  delay(1000);
  
  while (1) {
    int id = getFingerprintID();
    if (id > 0) {
      Serial.print("Matched ID: ");
      Serial.println(id);
      break;
    }
  }
}

void enrollUser(uint8_t id) {
  Serial.print("Enrolling ID: ");
  Serial.println(id);
  
  // First image
  while (finger.getImage() != FINGERPRINT_OK);
  finger.image2Tz(1);
  
  Serial.println("Remove finger");
  delay(3000);
  
  // Second image
  while (finger.getImage() != FINGERPRINT_OK);
  finger.image2Tz(2);
  
  if (finger.createModel() == FINGERPRINT_OK) {
    if (finger.storeModel(id) == FINGERPRINT_OK) {
      Serial.println("Enrollment successful!");
    }
  }
}

int getFingerprintID() {
  uint8_t p = finger.getImage();
  
  if (p != FINGERPRINT_OK) {
    return -1;
  }
  
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    return -1;
  }
  
  p = finger.fingerFastSearch();  // ✅ Official pattern
  if (p == FINGERPRINT_OK) {
    return finger.fingerID;
  }
  
  return 0;
}
```

---

## Summary: Adafruit Official Practices

✅ **DO:**
- Use `fingerFastSearch()` for matching
- Check `finger.confidence` for match quality
- Always check return codes (`uint8_t p`)
- Follow the two-image enrollment procedure
- Wrap LCD functions for consistency
- Use constants for timeouts
- Add error handling at every step

❌ **DON'T:**
- Use `fingerSearch()` - it's slow
- Ignore confidence scores
- Skip return code checking
- Modify sensor baud rate unnecessarily
- Make blocking calls in main loop
- Use magic numbers in code
