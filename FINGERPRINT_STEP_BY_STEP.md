# Fingerprint System - Step-by-Step Verification

## Pre-Upload Checklist

- [ ] Arduino board connected to COM7
- [ ] Fingerprint sensor wired correctly
  - RX pin 2 ← Sensor TX
  - TX pin 3 → Sensor RX
  - GND → GND
  - Power → Power
- [ ] LCD I2C display connected and address is 0x27
- [ ] SQL database online with EMPLOYE table
- [ ] Qt project compiled with latest mainwindow.cpp

---

## Step 1: Upload & Boot (5 min)

### Action
Upload `fingerprint_terminal/fingerprint_terminal.ino` to Arduino board.

### Expected Serial Output
Open Arduino Serial Monitor at **115200 baud**:
```
READY
TEMPLATES:5
```

If you see:
- ✅ Both lines → Go to Step 2
- ❌ `ERR:SENSOR` → Sensor not connected or bad I2C address
- ❌ `TEMPLATES:0` → No fingerprints enrolled; do Step 2 first anyway

---

## Step 2: Enroll a Test Fingerprint (5 min)

### Action
1. Start Qt app
2. Go to Personnel module
3. Click "Enroll Fingerprint" button
4. Place your finger on sensor when prompted
5. Remove and place again
6. Verify "Success" message

### Expected Serial Output
```
READY
TEMPLATES:5
OK                           <- LOGIN_ON response
ENROLL_OK:10                 <- Enrolled at ID 10
```

### Result
- ✅ Enrolled → Test fingerprint is now ID 10, verify on sensor
- ❌ `ENROLL_FAIL:TIMEOUT_1` → Finger not detected; try again with firm pressure
- ❌ `ENROLL_FAIL:TIMEOUT_2` → Second capture timed out; try again
- ❌ `ENROLL_FAIL:CREATE_MODEL` → Sensor error; reboot Arduino

---

## Step 3: Link Fingerprint to Employee (5 min)

### Action (SQLite browser or Qt app)
```sql
UPDATE EMPLOYE SET FP_ID = 10 WHERE ID_EMP = 1;
```

Or use Qt UI if there's a fingerprint ID field on the employee edit form.

### Result
- ✅ Employee 1 now has FP_ID = 10

---

## Step 4: Test Login with Fingerprint (10 min)

### Action
1. Start Qt app
2. On login screen, place enrolled finger on sensor
3. Observe results

### Expected Serial Output
```
Fingerprint: "READY"
Fingerprint: "OK"
(waiting for finger...)
Fingerprint: "MATCH:10"
Fingerprint: "NAME:John Doe"         <- Qt's lookup response
(login screen disappears, dashboard appears)
```

### Possible Outcomes

#### ✅ Success: Login works
- Serial shows `MATCH:10`
- Qt screen switches to dashboard
- Employee name appears on dashboard header

#### ❌ `MATCH:10` but "Unknown fingerprint"
- Sensor matched ID 10
- But no employee has FP_ID = 10 in database
- **Fix**: Run the SQL UPDATE from Step 3

#### ❌ No `MATCH:` line at all
- Sensor is scanning but no match found
- Possible causes:
  1. Fingerprint template lost (sensor reset?)
  2. Wrong finger placed (different finger than enrolled)
  3. Sensor image quality poor
- **Fix**: Re-enroll fingerprint in Step 2

#### ❌ `MATCH:10` appears twice or keeps repeating
- This is OK—Arduino sends one match, then waits for finger removal
- No multiple logins should occur

---

## Step 5: Test Denial Behavior (5 min)

### Action
1. On login screen, place **different finger** (unenrolled) on sensor
2. Observe behavior

### Expected Output
```
Fingerprint: "OK"
(waiting for finger...)
(no MATCH line; scan continues silently)
```

### Result
- ✅ Sensor tries to match but fails, keeps scanning
- This is expected—only enrolled fingerprints match

---

## Step 6: Test Logging Out & Re-Login (5 min)

### Action
1. From dashboard, click logout
2. Place enrolled finger again
3. Verify login works again

### Expected Output
```
Fingerprint: "OK"
...
Fingerprint: "MATCH:10"
Fingerprint: "NAME:John Doe"
```

### Result
- ✅ Repeatable logins work

---

## Troubleshooting

### Symptom: `READY` but no `TEMPLATES:` line
**Cause**: Sensor not responding or bad baud rate  
**Fix**:
1. Check `BAUD_FINGER = 57600` in Arduino code
2. Verify sensor is powered (LED should light up)
3. Check RX/TX pins are not swapped

### Symptom: `TEMPLATES:0` (no templates)
**Cause**: Sensor database is empty  
**Fix**: Enroll fingerprints using Step 2 (Qt app enrollment)

### Symptom: `ENROLL_FAIL:TIMEOUT_1`
**Cause**: Sensor can't detect finger  
**Fix**:
1. Clean sensor lens with soft cloth
2. Try different finger with more pressure
3. Hold finger still for 2+ seconds

### Symptom: Login shows "Unknown fingerprint"
**Cause**: Employee record doesn't have correct FP_ID  
**Fix**:
1. Note the ID from `MATCH:X` output
2. Update employee: `UPDATE EMPLOYE SET FP_ID = X WHERE ID_EMP = ?`
3. Try login again

### Symptom: Qt app shows nothing but READY
**Cause**: Qt not sending `LOGIN_ON` command  
**Fix**:
1. Check `sendFingerprintTerminalCommand("LOGIN_ON")` is called
2. Verify serial port is actually open
3. Check COM7 is the correct port (Arduino IDE should confirm)

### Symptom: Sensor seems to work but matches everything
**Cause**: Match threshold is too low (unlikely with official Adafruit library)  
**Fix**: Enroll fingerprints properly using 2 distinct captures

---

## Database Verification

### Check enrolled fingerprints
```sql
SELECT ID_EMP, NOM_EMP, PRENOM_EMP, FP_ID FROM EMPLOYE WHERE FP_ID > 0;
```

Should show employees with non-NULL FP_ID values.

### Check for orphaned fingerprints
```sql
SELECT * FROM EMPLOYE WHERE FP_ID IS NULL OR FP_ID = 0;
```

These employees can't log in with fingerprint.

### Add fingerprint to all employees (test mode)
```sql
UPDATE EMPLOYE SET FP_ID = 1 WHERE FP_ID IS NULL;
```

Now any employee can login with fingerprint ID 1.

---

## Final Checklist

- [ ] Arduino boots with `READY` + `TEMPLATES:n`
- [ ] At least 1 fingerprint enrolled (n > 0)
- [ ] At least 1 employee has FP_ID assigned
- [ ] Login with enrolled finger works
- [ ] Logout and re-login works
- [ ] Unenrolled finger doesn't match (as expected)

**If all checks pass**: System is fully functional! 🎉
