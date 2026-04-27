# Fingerprint Terminal Troubleshooting Guide

## What was broken

Three critical issues were found and fixed:

### ❌ Issue #1: Stray `CONFIDENCE:` line blocked matches (FIXED)
**Problem**: After finding a match, Arduino sent:
```
CONFIDENCE:12345
MATCH:5
```

Qt's parser **ignores** `CONFIDENCE:*` lines as debug output, so the match message was treated as noise and no match occurred.

**Fix**: Removed the stray `Serial.print("CONFIDENCE:...")` line. Now the Arduino sends `MATCH:id:confidence` as a single atomic protocol message.

### ❌ Issue #2: Lost `DBG:*` diagnostic lines  (FIXED)
**Problem**: Debug output from sensor errors was not being filtered on the Qt side, potentially interfering with protocol parsing.

**Fix**: Added `line.startsWith("DBG:")` to the Qt ignore list so diagnostic output never affects protocol state.

### ❌ Issue #3: Incorrect `MATCH:` parsing (FIXED)
**Problem**: Qt parser called `.toInt()` on the entire tail after `MATCH:`, but now Arduino sends `MATCH:id:confidence`. The colon in the confidence would cause parsing to fail.

**Fix**: Updated Qt to extract the ID by looking for the next `:` character, handling both `MATCH:5` and `MATCH:5:12345` formats.

---

## Verification Checklist

### Step 1: Upload Arduino sketch
Re-upload `fingerprint_terminal/fingerprint_terminal.ino` to your Arduino board.

### Step 2: Check fingerprints are enrolled
The fingerprint sensor database **must** have at least one fingerprint enrolled. To verify:

1. Open Arduino Serial Monitor (9600 baud or 115200 depending on your setup)
2. On the Arduino, when login mode starts, it sends: `TEMPLATES:<n>`
3. If `n` is 0, **no fingerprints are enrolled** and matching will always fail
4. Enroll a fingerprint via the Qt app's "Enroll" button before testing login

### Step 3: Run Qt app and monitor serial output
Expected startup sequence:
```
Fingerprint: "READY"
Fingerprint: "TEMPLATES:5"          <- Number of enrolled fingerprints
Fingerprint: "OK"                    <- Response to LOGIN_ON
(now in login mode, place finger)
Fingerprint: "MATCH:3:250"           <- Matched fingerprint ID 3 with confidence 250
Fingerprint: "NAME:John Doe"         <- Qt's lookup response
(login screen should disappear)
```

### Step 4: Verify employee record has fingerprint ID
If you see `MATCH:5` but login fails with "Empreinte inconnue", check:

1. Open the database viewer
2. Find the employee record
3. Verify the `FP_ID` or `ID_FINGERPRINT` column matches the ID sent by Arduino

**Example**: If Arduino sends `MATCH:3` but the employee's fingerprint ID is stored as `NULL` or `0`, the match will be rejected.

### Step 5: Test with a known fingerprint ID
1. Manually set an employee's fingerprint ID to match an enrolled template
2. Try logging in with that finger
3. If it works, the system is functioning; the previous enrollments weren't linked properly

---

## Protocol Specification (Final)

Arduino → Qt (one per line):

| Message | Meaning | Example |
|---------|---------|---------|
| `READY` | Boot complete, sensor initialized | `READY` |
| `TEMPLATES:n` | Number of enrolled templates | `TEMPLATES:5` |
| `MATCH:id:confidence` | Fingerprint matched (id=template ID, confidence=match score) | `MATCH:3:250` |
| `ENROLL_OK:id` | Enrollment succeeded | `ENROLL_OK:10` |
| `ENROLL_FAIL:reason` | Enrollment failed | `ENROLL_FAIL:TIMEOUT_1` |
| `DELETE_OK:id` | Deletion succeeded | `DELETE_OK:5` |
| `DELETE_FAIL:id` | Deletion failed | `DELETE_FAIL:5` |
| `ERR:reason` | Protocol/hardware error | `ERR:SENSOR` |

Qt → Arduino (one per line):

| Message | Meaning |
|---------|---------|
| `PING` | Test connectivity |
| `LOGIN_ON` | Start fingerprint scanning mode |
| `LOGIN_OFF` | Stop fingerprint scanning |
| `ENROLL` | Start auto-enroll (find first free slot) |
| `ENROLL:id` | Enroll at specific ID |
| `DELETE:id` | Delete fingerprint ID |
| `NAME:text` | Display name on LCD (max 16 chars) |
| `DENIED` | Show "access denied" message |

---

## Debugging: Arduino Serial Monitor Output

If you open the Arduino Serial Monitor and see lines like:
```
DBG:getImage_error:1
DBG:image2Tz_error:3
DBG:fingerFastSearch_nomatch:0
```

These are **diagnostic messages** (not protocol). They mean:
- `getImage_error:1` → Sensor couldn't capture image (finger not in frame)
- `image2Tz_error:3` → Captured image too poor quality
- `fingerFastSearch_nomatch:0` → Image was processed but doesn't match any enrolled template

---

## If Still Not Working

1. **Verify baud rates match**:
   - Serial (Qt ↔ Arduino): **115200 baud**
   - Fingerprint sensor: **57600 baud**

2. **Check wiring**:
   - RX pin 2 ← Fingerprint TX
   - TX pin 3 → Fingerprint RX
   - GND → GND
   - Power → Power (per sensor specs)

3. **Test sensor independently**:
   - Upload Adafruit's example sketch
   - Verify sensor responds with enrollment/matching

4. **Enable verbose logging in Qt**:
   - The Qt app already logs all lines via `qDebug() << "Fingerprint:" << line;`
   - Open Qt Creator's "Application Output" panel to see full traffic

---

## Quick Rebuild Checklist

- [ ] Upload Arduino sketch to board
- [ ] Verify `TEMPLATES:n` shows `n > 0` (templates enrolled)
- [ ] Run Qt app
- [ ] Place enrolled finger on sensor
- [ ] Confirm `Fingerprint: "MATCH:..."` appears in output
- [ ] Verify employee has matching fingerprint ID in database
- [ ] Check login succeeds without "Empreinte inconnue" error
