# Fingerprint System - Complete Flow Reference

## Arduino State: `scanMode` Boolean
- **`true`**: Listening for fingerprints, will send `MATCH:id` when found
- **`false`**: Not scanning, finger detected but ignored

---

## Activation Sequence

### At Startup
```
Arduino: READY
Arduino: TEMPLATES:5

Qt sends: LOGIN_ON
Arduino sets: scanMode = true

Ready for first login!
```

### User Places Enrolled Finger (First Time)
```
Arduino: MATCH:10                    (found match)
Qt sends: NAME:John Doe              (show name on LCD)
Qt sends: (2-sec delay)
Qt sends: LOGIN_ON                   (re-enable scanning)
Qt: Switch to dashboard              (user logged in)

Now scanning is ON for next attempt!
```

### User Returns to Login Screen (or Logs Out)
```
Qt detects: MainStacked index changed to 0 (login screen)
Qt sends: LOGIN_ON
Arduino sets: scanMode = true

Ready for another login!
```

### User Places Finger (Second Time)
```
(Same as first time...)
Arduino: MATCH:10
Qt: Login succeeds again ✓
```

---

## State Diagram

```
┌─────────────────┐
│   Startup       │
│   scanMode=OFF  │
└────────┬────────┘
         │
         ├─► Qt sends "LOGIN_ON"
         │
         ▼
┌─────────────────┐
│  Scanning       │
│  scanMode=ON    │◄─────────────┐
└────────┬────────┘              │
         │                       │
         ├─► User places finger  │
         │                       │
         ▼                       │
┌─────────────────┐              │
│  Match Found    │              │
│  Send MATCH:id  │              │
└────────┬────────┘              │
         │                       │
         ├─► Qt sends NAME:...   │
         │                       │
         ├─► (2-sec wait)        │
         │                       │
         ├─► Qt sends LOGIN_ON ──┘
         │
         ├─► Qt switches page (if on login screen)
         │
         ▼
┌─────────────────┐
│  Logged In      │
│  (Dashboard)    │
└─────────────────┘
         │
         ├─► User clicks logout / navigates back
         │
         ├─► Qt detects MainStacked=0
         │
         ├─► Qt sends LOGIN_ON ─────┐
         │                          │
         └──────────────────────────┘
```

---

## Message Sequence Examples

### Success Scenario
```
Time  Source     Message
────  ──────     ───────────────────────
 0ms  Arduino    READY
 0ms  Arduino    TEMPLATES:5
 5ms  Qt         LOGIN_ON
 10ms Qt         (waiting for user action)
100ms User       (places finger on sensor)
150ms Arduino    MATCH:10
150ms Qt         [looks up ID 10 in DB → finds John Doe]
150ms Qt         NAME:John Doe
150ms Qt         [switches to dashboard]
150ms Qt         [starts 2-sec timer]
2150ms Qt        LOGIN_ON
2150ms Arduino   scanMode = true (ready again)
```

### Re-login Scenario
```
Time  Source     Message
────  ──────     ───────────────────────
2150ms Qt        (user on dashboard)
3000ms User      (clicks logout / back button)
3001ms Qt        [detects MainStacked index = 0]
3001ms Qt        LOGIN_ON
3001ms Arduino   scanMode = true
3100ms User      (places finger again)
3150ms Arduino   MATCH:10
3150ms Qt        [lookup successful again]
3150ms Qt        NAME:John Doe
3150ms Qt        [switches to dashboard]
```

### Failure Scenario (Unknown Fingerprint)
```
Time  Source     Message
────  ──────     ───────────────────────
100ms User       (places unenrolled finger)
150ms Arduino    [no match found]
150ms Arduino    [scanMode still true, keeps scanning silently]
200ms Arduino    [keeps scanning...]
300ms User       (removes finger)
301ms Arduino    [lastMatchId = -1, ready for next attempt]
```

---

## Configuration Summary

### Arduino
```cpp
BAUD_SERIAL = 115200  // Qt ↔ Arduino
BAUD_FINGER = 57600   // Arduino ↔ Sensor
SCAN_INTERVAL = 300ms // Check sensor every 300ms
ENROLL_TIMEOUT = 15s  // Wait 15s for finger during enrollment
REMOVAL_TIMEOUT = 3s  // Wait 3s for finger removal between captures
```

### Qt
```cpp
Fingerprint Status Label Update Delay = Immediate
Re-enable Scanning After Match = 2 seconds
Auto-enable Scanning When on Login Screen = Immediate
```

---

## Troubleshooting Quick Guide

| Symptom | Cause | Fix |
|---------|-------|-----|
| Only works once | Scanning disabled after login | **FIXED**: Qt now auto re-enables |
| No `MATCH:` line | Sensor not scanning (scanMode=false) | Send `LOGIN_ON` from Qt |
| Matches different fingerprints | Threshold too low (unlikely) | Re-enroll with clearer captures |
| `MATCH:X` but "Unknown fingerprint" | Employee FP_ID not set correctly | `UPDATE EMPLOYE SET FP_ID=X` |
| Connection lost | Serial port closed | Restart Qt app |
| `ERR:SENSOR` | Sensor not detected | Check I2C address (0x27) and wiring |

---

## Key Files

- **Arduino**: `fingerprint_terminal/fingerprint_terminal.ino`
  - Main logic: `scanFingerprint()`, `enableScanning()`, `processCommand()`
  
- **Qt**: `mainwindow.cpp`
  - `initFingerprintTerminal()` — Setup with auto-re-enable on page change
  - `processFingerprintTerminalLine()` — Handle Arduino messages
  - `processFingerprintTerminalLine()` → Match handling now includes 2-sec re-enable

---

## Performance Notes

- **Scan rate**: 300ms interval = ~3 scans per second
- **Enrollment**: ~20-30 seconds total (2 captures + processing)
- **Match latency**: ~150-200ms after finger placed
- **Re-enable delay**: 2 seconds (safety margin, can be reduced to 0 for instant re-login)

---

## Future Improvements (Optional)

1. Make re-enable delay configurable (Settings)
2. Add "Last Matched" employee display on dashboard
3. Add fingerprint enrollment counter/statistics
4. Add biometric timeout (auto-logout after idle)
5. Allow partial fingerprint matches (confidence threshold)
