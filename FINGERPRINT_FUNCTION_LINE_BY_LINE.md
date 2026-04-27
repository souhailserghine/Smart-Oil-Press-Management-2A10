# Fingerprint Sensor Functions — Line-by-Line Logic (Part 1)

## Navigation

- Part 1 (this file): `FINGERPRINT_FUNCTION_LINE_BY_LINE.md`
- Part 2 (terminal message state machine): [`FINGERPRINT_FUNCTION_LINE_BY_LINE_PART2.md`](./FINGERPRINT_FUNCTION_LINE_BY_LINE_PART2.md)

This file explains, line by line, these fingerprint integration functions in `mainwindow.cpp`:

1. `setFingerprintStatus(...)`
2. `initFingerprintTerminal()`
3. `sendFingerprintTerminalCommand(...)`
4. `startFingerprintEnrollmentFromForm()`
5. `saveFingerprintIdForEmployee(...)`
6. `tryLinkPendingFingerprintForEmployee(...)`
7. `onFingerprintTerminalReadyRead()`
8. `resolveEmployeeByFingerprintId(...)`

---

## 1) `setFingerprintStatus(const QString& text, const QString& style)` (around line 332)

### Objective

Centralize status label updates used by the fingerprint workflow (text + style), while safely handling missing UI objects.

### Inputs / outputs

- Input: `text`, `style`
- Output: updates `ui->faceStatusLabel`

### Line-by-line

- **332** function declaration.
- **333** function body start.
- **334** `if (!ui || !ui->faceStatusLabel) return;`
  - Safety guard against null UI pointers.
- **335** set label text.
- **336** set stylesheet (color/weight/etc.).
- **337** end function.

---

## 2) `initFingerprintTerminal()` (around line 341)

### Objective

Initialize serial communication with Arduino fingerprint terminal, wire incoming data callback, and switch terminal to login mode.

### Inputs / outputs

- Input: none (uses class member `m_fingerprintTerminal`)
- Output:
  - serial connection attempt
  - `readyRead` signal wiring
  - sends startup commands (`PING`, `LOGIN_ON`)

### Line-by-line

- **341** function declaration.
- **342** body start.
- **343–345** protocol comments.
- **346** `const int rc = m_fingerprintTerminal.connect_arduino();`
  - Attempts serial connection.
- **347** check connection failure.
- **348** debug output with return code.
- **349–353** set warning status text depending on `rc`:
  - `rc == 1`: device detected but port busy.
  - else: device not detected.
- **354** return on connection failure.
- **355** end failure branch.
- **357–358** connect serial `readyRead` signal to `onFingerprintT erminalReadyRead`.
- **360** send `PING`.
- **361** send `LOGIN_ON`.
- **362** function end.

### Notes

- This function is both connectivity setup and protocol bootstrap.
- It is intentionally idempotent enough for startup use; repeated calls depend on lower-level serial class behavior.

---

## 3) `sendFingerprintTerminalCommand(const QString& command)` (around line 364)

### Objective

Send one clean line-delimited command to firmware.

### Inputs / outputs

- Input: logical command string (`ENROLL`, `DENIED`, `LOGIN_ON`, etc.)
- Output: writes bytes to serial using `write_to_arduino`

### Line-by-line

- **364** function declaration.
- **365** body start.
- **366** comment about one-command-per-line parser requirement.
- **367** convert command to UTF-8 payload.
- **368** append `\n` if missing.
- **369** write payload to terminal.
- **370** function end.

---

## 4) `startFingerprintEnrollmentFromForm()` (around line 372)

### Objective

Start enrollment workflow from UI form: validate terminal availability, avoid concurrent enrollments, update UI state, and send `ENROLL` command.

### Inputs / outputs

- Input: current serial open state and enrollment-in-progress flag
- Output:
  - enrollment state updates (`m_fingerprintEnrollInProgress`, `m_pendingFingerprintId`)
  - button state update
  - user status message
  - command `ENROLL`

### Line-by-line

- **372** declaration.
- **373** body start.
- **374** check serial pointer/open state.
- **375–376** warning dialog if terminal unavailable.
- **377** return.
- **378** end unavailable branch.
- **380** if enrollment already in progress.
- **381–383** info dialog (prevents starting a second enrollment).
- **384** return.
- **385** end in-progress branch.
- **387** set `m_fingerprintEnrollInProgress = true`.
- **388** reset `m_pendingFingerprintId = -1`.
- **390–392** disable enroll button if present.
- **394–396** show orange “enrollment in progress” status.
- **399** send `ENROLL` command.
- **400** end function.

### Edge cases handled

- Terminal disconnected
- duplicate enrollment click while previous session not finished

---

## 5) `saveFingerprintIdForEmployee(int employeeId, int fingerprintId) const` (around line 406)

### Objective

Persist sensor fingerprint ID into employee DB row (`EMPLOYE.FINGERID`).

### Inputs / outputs

- Input: `employeeId`, `fingerprintId`
- Output: boolean success/failure

### Line-by-line

- **406** declaration.
- **407** body start.
- **408** reject non-positive ids.
- **409** verify DB connection open.
- **410–411** debug + return false if DB closed.
- **414** create query object.
- **415–418** prepare update SQL dynamically using `kFingerprintColumn` (`FINGERID`).
- **416–417** debug + return false if prepare fails.
- **420** bind `:fp` to fingerprint id string.
- **421** bind `:id` to employee id.
- **422** execute query; on fail:
- **423–424** debug + return false.
- **427** return true on success.
- **428** end function.

### SQL behavior

`UPDATE EMPLOYE SET FINGERID = :fp WHERE id_emp = :id`

---

## 6) `tryLinkPendingFingerprintForEmployee(int employeeId, const QString& contextPastPart)` (around line 430)

### Objective

Attach currently pending enrolled fingerprint ID to a given employee and show a user warning if DB linking fails.

### Inputs / outputs

- Input: employee id, localized context string (`modifié`, `ajouté`, etc.)
- Output: no return; may show warning dialog

### Line-by-line

- **430** declaration.
- **431** body start.
- **432** if no pending id (`<=0`), no-op.
- **433** attempt DB save via `saveFingerprintIdForEmployee`.
- **434–436** on failure show warning with context-aware sentence.
- **437** end function.

---

## 7) `onFingerprintTerminalReadyRead()` (around line 440)

### Objective

Consume raw serial bytes, split into newline-delimited protocol messages, and dispatch each complete line to parser.

### Inputs / outputs

- Input: arbitrary chunks from serial `readyRead`
- Output: calls `processFingerprintTerminalLine(line)` for each complete line

### Line-by-line

- **440** declaration.
- **441** body start.
- **442** append newly-read bytes into persistent receive buffer.
- **444** initialize end-of-line index tracker.
- **445** loop while a newline exists in buffer.
- **446** extract one raw line before newline.
- **447** remove consumed bytes (+ newline) from buffer.
- **448** convert to UTF-8 QString and trim spaces.
- **449** dispatch non-empty lines to parser.
- **450** end loop.
- **451** function end.

### Why buffer-based parsing is necessary

Serial reads are chunked unpredictably; one read can contain:

- half a message,
- one exact message,
- multiple messages.

This function normalizes all three cases.

---

## 8) `resolveEmployeeByFingerprintId(int fingerprintId, int& employeeId, QString& fullName) const` (around line 454)

### Objective

Map a fingerprint sensor ID to an employee record (ID + display name) using DB lookup.

### Inputs / outputs

- Input: `fingerprintId`
- Output by reference: `employeeId`, `fullName`
- Return: `true` if a valid employee match is found

### Line-by-line

- **454** declaration.
- **455** body start.
- **456** initialize output id to `-1`.
- **457** clear output name.
- **459** DB-open guard.
- **460–461** debug + return false if DB not open.
- **464** create query.
- **465–468** prepare SQL selecting employee by `TRIM(FINGERID) = :fp`.
- **469** return false if prepare fails.
- **471** bind fingerprint id as string.
- **473** execute; on failure:
- **474–475** debug + return false.
- **478** if one row exists:
- **479** set output employee id.
- **480** set output full name (trimmed).
- **481** return true only if id > 0.
- **483** if no row matched, return false.
- **484** function end.

### Notes

- Uses `TRIM` in SQL to tolerate accidental whitespace in stored values.
- Output references are always reset first, preventing stale values on failure.

---

## Continue to Part 2

For full line-by-line breakdown of the terminal message state machine:

- login match/no-match handling
- enrollment success/failure handling
- device status fallthrough handling

Open: [`FINGERPRINT_FUNCTION_LINE_BY_LINE_PART2.md`](./FINGERPRINT_FUNCTION_LINE_BY_LINE_PART2.md)
