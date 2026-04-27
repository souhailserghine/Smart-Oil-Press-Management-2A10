# Qt Fingerprint Functions - Line-by-Line Breakdown

## Overview

All fingerprint handling in Qt is in `mainwindow.cpp`. Key class member variables:

```cpp
// mainwindow.h (class members)
private:
  ArduinoConnection m_fingerprintTerminal;  // Serial communication object
  QByteArray m_fingerprintRxBuffer;         // Buffer for incoming serial data
  int m_pendingFingerprintId;               // Holds FP ID after enrollment
  int m_loggedInId;                         // Current user ID
  
  // Methods:
  void initFingerprintTerminal();
  void sendFingerprintTerminalCommand(const QString& cmd);
  void onFingerprintTerminalReadyRead();
  void processFingerprintTerminalLine(const QString& line);
  bool resolveEmployeeByFingerprintId(int fpId, int& empId, QString& name) const;
  bool saveFingerprintIdForEmployee(int empId, int fpId) const;
  void startFingerprintEnrollmentFromForm();
  void setFingerprintStatus(const QString& text, const QString& style);
```

---

## Function 1: initFingerprintTerminal()

**Location:** `mainwindow.cpp` (called from MainWindow constructor)

**Purpose:** Initialize fingerprint system at app startup

```cpp
void MainWindow::initFingerprintTerminal()
{
  // ============ PART 1: Connect to Arduino ============
  const int rc = m_fingerprintTerminal.connect_arduino();
  
  // Line explanation:
  // - m_fingerprintTerminal: Custom wrapper (see arduino.h/cpp)
  // - .connect_arduino(): Opens serial port (auto-finds COM port or uses config)
  // - Returns: 0 on success, non-zero (error code) on failure
  // - rc: Return code variable
  
  if (rc != 0) {
    // Connection failed (COM port not found, wrong port, etc.)
    setFingerprintStatus(
      tr("⚠ Terminal d'empreintes non détecté."),  // French: "Fingerprint terminal not detected"
      QStringLiteral("color: #ef6c00; font-weight: bold;")  // Orange text, bold
    );
    return;  // Exit, don't set up signal handlers
  }
  
  // ============ PART 2: Signal Handler 1 - Serial Data Ready ============
  QObject::connect(
    m_fingerprintTerminal.getserial(),  // Source: Serial port object
    &QSerialPort::readyRead,            // Signal: Data received
    this,                                // Receiver: This window
    &MainWindow::onFingerprintTerminalReadyRead  // Slot: Handle data
  );
  
  // What happens:
  // - When Arduino sends data, Qt's serial driver emits readyRead signal
  // - Qt automatically calls onFingerprintTerminalReadyRead()
  // - We don't need to poll; signal-driven (event-based)
  
  // ============ PART 3: Signal Handler 2 - Page Changed ============
  if (ui && ui->MainStacked) {
    // ui->MainStacked: The main page switcher widget
    // Index 0 = login screen
    // Index 1 = dashboard
    // Index 2+ = other modules
    
    QObject::connect(
      ui->MainStacked,                          // Source: Stacked widget
      QOverload<int>::of(                        // Signal overload resolution
        &QStackedWidget::currentChanged),        // Signal: Page changed
      this,                                      // Receiver: This window
      [this](int index) {                        // Lambda (inline function)
        if (index == 0) {  // User switched to login screen?
          sendFingerprintTerminalCommand("LOGIN_ON");  // Re-enable scanning
        }
      }
    );
    
    // QOverload<int>::of(...):
    // QStackedWidget::currentChanged has multiple overloads
    // We need the version that passes an int (the new index)
    // Overload resolution tells compiler which one we want
    
    // Lambda function: [this](int index) { ... }
    // [this]: Capture 'this' pointer (so we can access m_fingerprintTerminal, etc)
    // (int index): Parameter passed by Qt signal
  }
  
  // ============ PART 4: Start Scanning ============
  sendFingerprintTerminalCommand(QStringLiteral("LOGIN_ON"));
  
  // Sends: "LOGIN_ON\n" to Arduino
  // Arduino receives, sets scanMode=true
  // Main loop starts checking for fingerprints every 50ms
}
```

### Key Insights

1. **Error Handling:** If Arduino not found, show error message and exit gracefully
2. **Signal-Driven:** Uses Qt signals instead of polling
3. **Auto Re-Enable:** When user returns to login screen, automatically re-enable scanning
4. **Two Serial Connections:** 
   - Regular serial communication (data events)
   - Separate serial port object for polling (if needed)

---

## Function 2: sendFingerprintTerminalCommand()

**Location:** `mainwindow.cpp`

**Purpose:** Send a command string to Arduino (internal helper)

```cpp
void MainWindow::sendFingerprintTerminalCommand(const QString& command)
{
  // Convert Qt string to raw bytes
  QByteArray payload = command.toUtf8() + "\n";
  
  // Example: "LOGIN_ON" → ['L','O','G','I','N','_','O','N','\n']
  // toUtf8(): Convert QString (Unicode) to UTF-8 bytes
  // + "\n": Append newline terminator
  
  // Send to Arduino via serial
  m_fingerprintTerminal.write_to_arduino(payload);
  
  // m_fingerprintTerminal.write_to_arduino():
  // - Custom wrapper method (from arduino.h/cpp)
  // - Actually calls QSerialPort::write()
  // - Non-blocking: returns immediately, data sent in background
}
```

### Usage Examples

```cpp
sendFingerprintTerminalCommand("LOGIN_ON");
// Sends bytes: 'L','O','G','I','N','_','O','N','\n' (115200 baud)
// Arduino receives and executes

sendFingerprintTerminalCommand("ENROLL");
// Starts enrollment

sendFingerprintTerminalCommand("NAME:John Doe");
// Sends employee name to LCD

sendFingerprintTerminalCommand("DELETE:10");
// Delete fingerprint at slot 10
```

### Why Non-Blocking?

- `write_to_arduino()` returns immediately
- Data is sent in background by Qt's event loop
- Application stays responsive
- Multiple commands can be queued

---

## Function 3: onFingerprintTerminalReadyRead()

**Location:** `mainwindow.cpp`

**Purpose:** Called when Arduino sends data (slot for QSerialPort::readyRead signal)

```cpp
void MainWindow::onFingerprintTerminalReadyRead()
{
  // ============ PART 1: Read All Available Data ============
  m_fingerprintRxBuffer.append(m_fingerprintTerminal.read_from_arduino());
  
  // m_fingerprintRxBuffer: QByteArray that accumulates incoming bytes
  // read_from_arduino(): Custom wrapper, actually calls QSerialPort::readAll()
  // readAll(): Read all available bytes (0 to several KB)
  
  // Example: Arduino sends "READY\nTEMPLATES:5\n" in two parts:
  // 
  // Signal 1: readyRead() fired, only "READY\nT" received
  //   → m_fingerprintRxBuffer = "READY\nT"
  //   → Process "READY", buffer = "T"
  //
  // Signal 2: readyRead() fired, "EMPLATES:5\n" received
  //   → m_fingerprintRxBuffer = "TEMPLATES:5\n"
  //   → Process "TEMPLATES:5", buffer = ""
  
  // ============ PART 2: Extract and Process Lines ============
  int eolIndex = -1;
  while ((eolIndex = m_fingerprintRxBuffer.indexOf('\n')) >= 0) {
    // Loop while buffer contains at least one complete line
    
    // Step 2a: Extract line up to newline
    const QString line = QString::fromUtf8(m_fingerprintRxBuffer.left(eolIndex)).trimmed();
    
    // .left(eolIndex): Get first eolIndex characters (everything before '\n')
    // Example: "MATCH:10\n" → left(8) → "MATCH:10"
    //
    // QString::fromUtf8(): Convert raw bytes to Unicode string
    //
    // .trimmed(): Remove leading/trailing whitespace
    // Handles "\r\n", " MATCH:10 ", etc.
    
    // Step 2b: Remove processed line from buffer
    m_fingerprintRxBuffer.remove(0, eolIndex + 1);
    
    // .remove(0, eolIndex + 1): Delete first (eolIndex + 1) bytes
    // +1 to include the '\n' character itself
    // Example: buffer = "MATCH:10\nPONG\n", eolIndex = 8
    //   remove(0, 9) → buffer = "PONG\n"
    
    // Step 2c: Process the line if non-empty
    if (!line.isEmpty()) {
      processFingerprintTerminalLine(line);
    }
    
    // If line is empty (just whitespace), skip it
  }
}
```

### Why This Approach?

1. **Non-Blocking:** Doesn't block waiting for complete line
2. **Handles Fragmentation:** Serial data can arrive in chunks
3. **Handles Multiple Lines:** Single signal might contain "LINE1\nLINE2\nLINE3\n"
4. **Robust:** Strips whitespace and handles edge cases

### Example Scenario

```
Timeline:
0ms    Arduino boots → Serial.println("READY")
       Sends 6 bytes: 'R','E','A','D','Y','\n'

1ms    Qt receives → onFingerprintTerminalReadyRead()
       buffer = "READY\n"
       Extract line = "READY"
       buffer = ""
       processFingerprintTerminalLine("READY")

5ms    Arduino: Serial.print("TEMPLATES:"); Serial.println(5);
       Sends 11 bytes: 'T','E','M','P','L','A','T','E','S',':','5','\n'

6ms    Qt receives → onFingerprintTerminalReadyRead()
       buffer = "TEMPLATES:5\n"
       Extract line = "TEMPLATES:5"
       buffer = ""
       processFingerprintTerminalLine("TEMPLATES:5")

1000ms User places finger

1100ms Arduino detects match: Serial.print("MATCH:"); Serial.println(10);
       Sends 8 bytes: 'M','A','T','C','H',':','1','0','\n'

1101ms Qt receives → onFingerprintTerminalReadyRead()
       buffer = "MATCH:10\n"
       Extract line = "MATCH:10"
       processFingerprintTerminalLine("MATCH:10")
       [Now Qt processes the match, looks up employee, etc.]
```

---

## Function 4: processFingerprintTerminalLine()

**Location:** `mainwindow.cpp`

**Purpose:** Handle each message from Arduino (state machine)

```cpp
void MainWindow::processFingerprintTerminalLine(const QString& line)
{
  // Debug logging
  qDebug() << "Fingerprint:" << line;  // Output to debug console
  
  // ============ CONTEXT: Are we on login screen? ============
  const bool onLogin = ui->MainStacked && ui->MainStacked->currentIndex() == 0;
  // Used later to decide whether to show messages to user
  // If on dashboard, some messages are suppressed
  
  // ============ SECTION 1: IGNORE HOUSEKEEPING MESSAGES ============
  if (line == "READY" || line == "PONG" || line.startsWith("TEMPLATES:")) {
    return;  // Just housekeeping, don't need to act on it
  }
  
  // READY: Sent once at boot
  // PONG: Response to our PING (connection test)
  // TEMPLATES:n: Number of enrolled fingerprints
  // → We don't need to respond to these or show user
  
  // ============ SECTION 2: IGNORE DIAGNOSTIC OUTPUT ============
  if (line.startsWith("DBG:")) {
    return;  // Debug messages from Arduino, ignore in Qt
  }
  
  // These go to Arduino Serial Monitor only (for technician)
  // Example: "DBG:getImage_error:3"
  
  // ============ SECTION 3: ERROR CONDITION ============
  if (line.startsWith("ERR:")) {
    // Line is like "ERR:SENSOR" (sensor not detected)
    
    if (onLogin) {
      // Only show error if user is on login screen
      setFingerprintStatus(
        tr("⚠ Error: %1").arg(line),          // Show full error message
        QStringLiteral("color:#ef6c00; font-weight:bold;")  // Orange, bold
      );
    }
    return;  // Exit, don't process further
  }
  
  // ============ SECTION 4: ENROLLMENT SUCCESS ============
  if (line.startsWith("ENROLL_OK:")) {
    // Format: "ENROLL_OK:15" = Enrolled at slot 15
    
    // Extract ID from "ENROLL_OK:15"
    // mid(10): Skip first 10 characters ("ENROLL_OK:")
    bool ok = false;
    int fpId = line.mid(10).toInt(&ok);
    
    // toInt(&ok): Convert "15" to integer 15
    // ok = true if conversion succeeded
    
    if (ok && fpId > 0) {
      // Valid ID extracted
      m_pendingFingerprintId = fpId;  // Store for later (linking to employee)
      
      setFingerprintStatus(
        QString("✔ Enrolled ID %1").arg(fpId),  // "✔ Enrolled ID 15"
        QStringLiteral("color:#2e7d32; font-weight:bold;")  // Green, bold
      );
    }
    return;
  }
  
  // ============ SECTION 5: ENROLLMENT FAILURE ============
  if (line.startsWith("ENROLL_FAIL")) {
    // Format: "ENROLL_FAIL:TIMEOUT_1" (user didn't place finger)
    // Other examples: "ENROLL_FAIL:IMAGE2TZ_1", "ENROLL_FAIL:NO_SLOT"
    
    setFingerprintStatus(
      tr("❌ Enrollment failed"),
      QStringLiteral("color:#c62828; font-weight:bold;")  // Red, bold
    );
    return;
  }
  
  // ============ SECTION 6: FINGERPRINT MATCHED ============
  if (line.startsWith("MATCH:")) {
    // Format: "MATCH:10" = Fingerprint ID 10 matched
    
    // ===== Step 6a: Parse the fingerprint ID =====
    int colonPos = line.indexOf(':', 6);  // Find ':' after position 6
    // "MATCH:10" → colonPos = 5
    // "MATCH:10:500" → colonPos = 5 (first ':' after "MATCH")
    
    QString idStr;
    if (colonPos > 0) {
      // There's a colon somewhere (unusual format with confidence)
      idStr = line.mid(6, colonPos - 6);  // Extract between colons
    } else {
      // Standard format "MATCH:10"
      idStr = line.mid(6);  // Extract from position 6 onward
    }
    
    bool ok = false;
    int fpId = idStr.toInt(&ok);  // Convert string "10" to integer 10
    
    if (!ok || fpId <= 0) {
      // Failed to parse ID or invalid
      sendFingerprintTerminalCommand("DENIED");
      return;
    }
    
    // ===== Step 6b: Look up employee in database =====
    int empId = -1;
    QString empName;
    if (!resolveEmployeeByFingerprintId(fpId, empId, empName) || empId <= 0) {
      // Fingerprint not found in employee database
      // (Or employee record corrupted)
      
      sendFingerprintTerminalCommand("DENIED");  // Tell Arduino to show "Access Denied"
      
      if (onLogin) {
        setFingerprintStatus(
          tr("Unknown fingerprint"),  // Show to user
          QStringLiteral("color:#c62828;")  // Red
        );
      }
      return;
    }
    
    // ===== Step 6c: LOGIN SUCCESS =====
    // Send employee name to Arduino (for LCD display)
    sendFingerprintTerminalCommand("NAME:" + empName.left(16));
    // empName.left(16): Cap at 16 characters (LCD width)
    // Example: "NAME:John Doe"
    
    // Update global state
    m_loggedInId = empId;  // Remember who logged in (employee ID 5)
    
    // Update UI
    if (ui->userNameLabel) {
      ui->userNameLabel->setText(empName);  // Display name in top-right
    }
    
    // Clear login form
    if (ui->userinput) ui->userinput->clear();     // Clear email field
    if (ui->pwdinput) ui->pwdinput->clear();       // Clear password field
    
    // Navigate to main screen
    if (onLogin) {
      ui->MainStacked->setCurrentIndex(1);  // Go to dashboard
    }
    
    // Schedule auto re-enable of scanning (2 seconds)
    QTimer::singleShot(2000, this, [this]() {
      sendFingerprintTerminalCommand("LOGIN_ON");
    });
    // This allows immediate re-login if user logs out quickly
    // Otherwise, requires timeout or explicit re-enable
    
    // Show success message
    setFingerprintStatus(
      tr("✔ Login successful"),
      QStringLiteral("color:#2e7d32; font-weight:bold;")  // Green, bold
    );
    return;
  }
  
  // ============ SECTION 7: DELETION RESPONSES ============
  if (line.startsWith("DELETE_OK:")) {
    setFingerprintStatus(
      tr("✔ Deleted"),
      QStringLiteral("color:#2e7d32;")
    );
    return;
  }
  
  if (line.startsWith("DELETE_FAIL")) {
    setFingerprintStatus(
      tr("❌ Delete failed"),
      QStringLiteral("color:#c62828;")
    );
    return;
  }
}
```

### Message Handling Flow

```
Incoming line: "MATCH:10"
  │
  ├─ Starts with "READY"? No
  ├─ Starts with "DBG:"? No
  ├─ Starts with "ERR:"? No
  ├─ Starts with "ENROLL_OK:"? No
  ├─ Starts with "ENROLL_FAIL"? No
  ├─ Starts with "MATCH:"? YES
  │   │
  │   ├─ Parse ID → 10
  │   ├─ Look up in database → Found employee 5 "John Doe"
  │   ├─ Send "NAME:John Doe" to Arduino
  │   ├─ Set m_loggedInId = 5
  │   ├─ Update UI
  │   ├─ Go to dashboard
  │   ├─ Schedule re-enable scanning
  │   └─ Show "✔ Login successful"
  │
  └─ END
```

---

## Function 5: resolveEmployeeByFingerprintId()

**Location:** `mainwindow.cpp`

**Purpose:** Database lookup (fingerprint ID → employee info)

```cpp
bool MainWindow::resolveEmployeeByFingerprintId(
  int fingerprintId,           // Input: Fingerprint slot ID (1-127)
  int& empId,                  // Output: Employee ID
  QString& fullName            // Output: Employee name
) const
{
  // Initialize outputs (so caller knows they're invalid if we fail)
  empId = -1;
  fullName.clear();
  
  // ============ CHECK DATABASE CONNECTION ============
  if (!QSqlDatabase::database().isOpen()) {
    return false;  // Database not connected
  }
  
  // ============ BUILD SQL QUERY ============
  QSqlQuery q;
  q.prepare(QStringLiteral(
    "SELECT ID_EMP, NOM_EMP || ' ' || PRENOM_EMP FROM EMPLOYE WHERE %1 = :fp"
  ).arg(kFingerprintColumn));
  
  // Breakdown:
  // QSqlQuery q: Create query object
  // q.prepare(...): Prepare query string (prevents SQL injection)
  //
  // SQL: "SELECT ID_EMP, NOM_EMP || ' ' || PRENOM_EMP FROM EMPLOYE WHERE FINGERID = :fp"
  // 
  // .arg(kFingerprintColumn): Replace %1 with "FINGERID"
  // (kFingerprintColumn = QStringLiteral("FINGERID"))
  //
  // NOM_EMP || ' ' || PRENOM_EMP: Concatenate firstname + space + lastname
  // (SQLite uses || for string concatenation)
  //
  // :fp: Named parameter (bound later)
  
  // ============ BIND PARAMETERS ============
  q.bindValue(":fp", QString::number(fingerprintId));
  
  // bindValue(":fp", value): Replace :fp with actual value
  // QString::number(10): Convert integer 10 to string "10"
  // (SQL needs everything as strings in parameterized queries)
  
  // ============ EXECUTE QUERY ============
  if (!q.exec() || !q.next()) {
    // q.exec(): Execute query, return false if SQL error
    // q.next(): Fetch first result row, return false if no rows found
    return false;  // No employee found with this fingerprint ID
  }
  
  // ============ EXTRACT RESULTS ============
  empId = q.value(0).toInt();  // Column 0: ID_EMP
  fullName = q.value(1).toString().trimmed();  // Column 1: concatenated name
  
  // q.value(column): Get value from current result row
  // .toInt(): Convert to integer
  // .toString(): Convert to string
  // .trimmed(): Remove leading/trailing whitespace
  
  // ============ FALLBACK NAME ============
  if (fullName.isEmpty()) {
    fullName = QString("Emp. %1").arg(empId);  // Fallback: "Emp. 5"
  }
  
  // ============ RETURN SUCCESS ============
  return empId > 0;  // true if valid employee ID found
}
```

### Example Scenarios

**Scenario 1: Fingerprint found**
```
Input: fingerprintId = 10

SQL: SELECT ID_EMP, NOM_EMP || ' ' || PRENOM_EMP 
     FROM EMPLOYE 
     WHERE FINGERID = 10

Result: 
  ID_EMP = 5
  Name = "John Doe"

Output: empId = 5, fullName = "John Doe", return true
```

**Scenario 2: Fingerprint not found**
```
Input: fingerprintId = 99

SQL: SELECT ... WHERE FINGERID = 99

Result: No rows

Output: empId = -1, fullName = "", return false
→ Qt shows "Unknown fingerprint"
```

**Scenario 3: Empty name (fallback)**
```
Input: fingerprintId = 10

Result: ID_EMP = 5, Name = "" (empty)

Fallback triggered:
Output: fullName = "Emp. 5"
```

---

## Function 6: saveFingerprintIdForEmployee()

**Location:** `mainwindow.cpp`

**Purpose:** Link fingerprint to employee (after enrollment succeeds)

```cpp
bool MainWindow::saveFingerprintIdForEmployee(int employeeId, int fingerprintId) const
{
  // ============ VALIDATION ============
  if (employeeId <= 0 || fingerprintId <= 0) return false;
  if (!QSqlDatabase::database().isOpen()) return false;
  
  // Ensure both IDs are valid positive integers
  // Ensure database is connected
  
  // ============ BUILD UPDATE QUERY ============
  QSqlQuery q;
  q.prepare(
    QStringLiteral("UPDATE EMPLOYE SET %1 = :fp WHERE ID_EMP = :id")
    .arg(kFingerprintColumn)
  );
  
  // SQL: "UPDATE EMPLOYE SET FINGERID = :fp WHERE ID_EMP = :id"
  // This sets the FINGERID column for employee with ID_EMP = :id
  
  // ============ BIND PARAMETERS ============
  q.bindValue(":fp", QString::number(fingerprintId));  // FINGERID = 10
  q.bindValue(":id", employeeId);                      // ID_EMP = 5
  
  // ============ EXECUTE QUERY ============
  return q.exec();  // true if successful, false if error
}
```

### Usage

Called after successful enrollment:

```cpp
// In processFingerprintTerminalLine(), after ENROLL_OK:15 received:
m_pendingFingerprintId = 15;

// Later, when user clicks "Link to Employee":
int currentEmployeeId = 5;
saveFingerprintIdForEmployee(currentEmployeeId, m_pendingFingerprintId);

// Result: Database updated
// UPDATE EMPLOYE SET FINGERID = 15 WHERE ID_EMP = 5
//
// Now employee 5's fingerprint ID is linked to slot 15 in sensor
// Next time fingerprint 15 matches, employee 5 will login
```

---

## Function 7: startFingerprintEnrollmentFromForm()

**Location:** `mainwindow.cpp`

**Purpose:** UI button handler - start enrollment process

```cpp
void MainWindow::startFingerprintEnrollmentFromForm()
{
  // ============ CHECK CONNECTION ============
  if (!m_fingerprintTerminal.getserial() || 
      !m_fingerprintTerminal.getserial()->isOpen()) {
    // Serial port not open
    QMessageBox::warning(
      this,
      tr("Erreur"),
      tr("Terminal non connecté.")  // "Terminal not connected"
    );
    return;
  }
  
  // ============ SHOW STATUS ============
  setFingerprintStatus(
    tr("⏳ Enrôlement en cours..."),  // "Enrollment in progress..."
    QStringLiteral("color: #ef6c00; font-weight: bold;")  // Orange, bold
  );
  
  // ============ START ENROLLMENT ============
  sendFingerprintTerminalCommand(QStringLiteral("ENROLL"));
  
  // Sends "ENROLL\n" to Arduino
  // Arduino auto-finds next free slot (1-127)
  // Then:
  //   - Waits for first fingerprint capture (5 seconds)
  //   - Waits for finger removal (1 second)
  //   - Waits for second capture (5 seconds)
  //   - Creates and stores model
  //   - Sends "ENROLL_OK:X" back
  // Qt receives "ENROLL_OK:X" and processes it
}
```

### Flow

```
User clicks "Enroll Fingerprint" button
  │
  ├─ Is Arduino connected? 
  │   No → Show error message, exit
  │   Yes → Continue
  │
  ├─ Show "⏳ Enrôlement en cours..." message
  │
  └─ Send "ENROLL\n" to Arduino
      │
      └─ [Arduino handles enrollment for ~10-15 seconds]
          │
          └─ Sends "ENROLL_OK:15\n"
              │
              └─ Qt receives and processes
                  │
                  ├─ m_pendingFingerprintId = 15
                  ├─ Show "✔ Enrolled ID 15"
                  └─ User then clicks "Link to Employee"
                      │
                      └─ saveFingerprintIdForEmployee(5, 15)
```

---

## Function 8: setFingerprintStatus()

**Location:** `mainwindow.cpp`

**Purpose:** Update status label on UI (info, warnings, errors)

```cpp
void MainWindow::setFingerprintStatus(const QString& text, const QString& style)
{
  if (!ui || !ui->faceStatusLabel) return;
  
  ui->faceStatusLabel->setText(text);
  ui->faceStatusLabel->setStyleSheet(style);
}
```

### Usage Examples

```cpp
// Success (green)
setFingerprintStatus(
  "✔ Login successful",
  "color:#2e7d32; font-weight:bold;"
);

// Info (orange)
setFingerprintStatus(
  "⏳ Enrôlement en cours...",
  "color:#ef6c00; font-weight:bold;"
);

// Error (red)
setFingerprintStatus(
  "Unknown fingerprint",
  "color:#c62828;"
);
```

---

## Helper Function: tryLinkPendingFingerprintForEmployee()

**Location:** `mainwindow.cpp`

**Purpose:** Called when user selects employee to link pending fingerprint

```cpp
void MainWindow::tryLinkPendingFingerprintForEmployee(
  int employeeId,
  const QString& contextPastPart
)
{
  // Only proceed if enrollment previously succeeded
  if (m_pendingFingerprintId <= 0) return;
  
  // Save the link to database
  saveFingerprintIdForEmployee(employeeId, m_pendingFingerprintId);
}
```

**Flow:**
```
1. User clicks "Enroll Fingerprint"
2. Arduino completes enrollment, sends "ENROLL_OK:15"
3. Qt sets m_pendingFingerprintId = 15
4. User navigates to "Personnel" module
5. User selects employee "John Doe" (ID 5)
6. User clicks "Link This Fingerprint"
7. Qt calls tryLinkPendingFingerprintForEmployee(5, ...)
8. Database updated: FINGERID = 15 for employee 5
9. Done! Employee can now login with fingerprint
```

---

## State Management

### Instance Variables

```cpp
int m_pendingFingerprintId;    // -1 = none pending, 1-127 = pending FP ID
int m_loggedInId;              // -1 = not logged in, 1+ = employee ID
QByteArray m_fingerprintRxBuffer;  // Accumulates serial data
ArduinoConnection m_fingerprintTerminal;  // Serial communication
```

### State Transitions

```
START:
  m_loggedInId = -1
  m_pendingFingerprintId = -1

USER LOGS IN:
  MATCH received → m_loggedInId = 5 ✓

USER STARTS ENROLLMENT:
  ENROLL sent → (wait)

ENROLLMENT SUCCEEDS:
  ENROLL_OK:15 received → m_pendingFingerprintId = 15 ✓

USER LINKS FINGERPRINT:
  tryLinkPendingFingerprintForEmployee(5, ...) called
  → saveFingerprintIdForEmployee(5, 15)
  → m_pendingFingerprintId = -1 (cleared)

NEXT LOGIN:
  MATCH:15 received → lookup employee 5 → m_loggedInId = 5 ✓
```

---

## Error Handling Patterns

### 1. Connection Check
```cpp
if (!m_fingerprintTerminal.getserial() || !m_fingerprintTerminal.getserial()->isOpen()) {
  QMessageBox::warning(...);
  return;
}
```

### 2. Database Lookup
```cpp
if (!resolveEmployeeByFingerprintId(fpId, empId, name)) {
  sendFingerprintTerminalCommand("DENIED");
  return;
}
```

### 3. Parameter Validation
```cpp
bool ok = false;
int id = line.mid(10).toInt(&ok);
if (!ok || id <= 0) return;
```

### 4. UI Safety Check
```cpp
if (!ui || !ui->faceStatusLabel) return;
```

---

## Performance Notes

| Operation | Time |
|-----------|------|
| Database lookup (SQL) | ~1-2ms |
| Serial send | <1ms (async) |
| String parsing | <1ms |
| UI update | ~5-10ms |

All Qt operations are non-blocking (don't freeze UI).

---

This detailed breakdown explains every line and the reasoning behind each decision in the Qt fingerprint code.
