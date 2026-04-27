# Fingerprint Sensor Functions — Line-by-Line Logic (Part 2)

## Navigation

- Part 1 (init, DB link, read parsing): [`FINGERPRINT_FUNCTION_LINE_BY_LINE.md`](./FINGERPRINT_FUNCTION_LINE_BY_LINE.md)
- Part 2 (this file): `FINGERPRINT_FUNCTION_LINE_BY_LINE_PART2.md`

This file explains the fingerprint protocol state-machine function:

- `void MainWindow::processFingerprintTerminalLine(const QString& line)`

Source: `mainwindow.cpp` around lines ~486 onward.

---

## `processFingerprintTerminalLine(const QString& line)`

### Objective
Interpret each normalized message line received from the fingerprint terminal and trigger the correct business/UI action:

- login success/failure feedback,
- enrollment result handling,
- serial protocol response commands (`DENIED`, `NAME:...`, `LOGIN_ON`),
- state cleanup and user notifications.

### Inputs / outputs
- Input: one complete protocol line (`READY`, `MATCH:7`, `NO_MATCH`, `ENROLL_OK:...`, etc.)
- Output:
  - potential serial commands back to terminal,
  - potential DB lookups/writes,
  - UI navigation/status updates,
  - enrollment state changes.

### Supported inbound protocol messages
- `READY`
- `PONG`
- `MATCH:<id>`
- `NO_MATCH`
- `ENROLL_OK:<id>`
- `ENROLL_FAIL[:reason]`
- plus pass-through status logs: `DELETE_OK:*`, `DELETE_FAIL:*`, `ERR:*`

---

## Line-by-line

- **486** function declaration.
- **487** body start.
- **488–489** debug-log the incoming line.

### Local UI/style context and helper

- **491** `onLoginPage`
  - True when app currently displays login screen (`MainStacked` index 0).
- **492–497** pre-defined style strings
  - Normal error/warn/success + bold variants for enrollment statuses.
- **499–503** local lambda `finishEnrollmentFlow`
  - Resets enrollment-in-progress flag,
  - re-enables enroll button,
  - sends `LOGIN_ON` to return firmware to login mode.

### Ignore pure heartbeat lines

- **505** `if (line == "READY" || line == "PONG")`
  - Keep-alive / readiness lines require no extra action.
- **506** return.

---

## Branch A — `MATCH:<id>` handling

- **509** check line prefix `MATCH:`.
- **510** `bool ok = false;`
  - Parse-success flag.
- **511** parse `fpId` from suffix.
- **512** if parse fails:
- **513** send `DENIED` to terminal.
- **514** return.

### Resolve fingerprint id to employee

- **517–518** initialize output placeholders.
- **519** attempt DB resolve via `resolveEmployeeByFingerprintId`.
- **519–526** unresolved match path:
  - debug log unresolved id,
  - send `DENIED`,
  - if on login page, show red status message,
  - return.

### Successful match path

- **529** send `NAME:<displayName>` truncated to 16 chars.
  - Terminal display compatibility safeguard.
- **533** store software session user id (`m_loggedInId`).
- **534–536** update user name label when available.
- **537–538** clear login input fields.
- **539–541** if currently on login page, navigate to main page (`index 1`).
- **542** show green success status.
- **543** return.

---

## Branch B — `NO_MATCH`

- **546** check exact `NO_MATCH` line.
- **547–549** if on login page, show orange retry status.
- **550** return.

---

## Branch C — `ENROLL_OK:<id>`

- **553** check prefix.
- **554–555** parse enrolled fingerprint id.
- **556** always call `finishEnrollmentFlow()` first.
  - ensures firmware/UI returns to normal mode.
- **558** validate parsed id.
- **559–560** on invalid id, show red error and return.
- **563** store pending id in `m_pendingFingerprintId`.

### Immediate DB link when editing an existing employee

- **566–567** read `editingId` from add/edit button property.
- **568** if `editingId > 0`, currently editing existing employee.
- **569** try immediate DB save:
  - **570–572** success -> green confirmation with ids.
  - **573–576** failure -> orange warning (captured but DB not linked).
- **577** else branch for add/new mode.
- **578–580** show green message: fingerprint captured, will be linked upon employee save.
- **581** return.

---

## Branch D — `ENROLL_FAIL` (with optional reason)

- **584** check prefix.
- **585** call `finishEnrollmentFlow()`.
- **586** prepare optional detail text buffer.
- **587** locate first `:` in message.
- **588–590** if detail exists after colon, extract+trim it.
- **592–596** set red failure status:
  - generic message if no detail,
  - specific message if detail provided.
- **597** return.

---

## Branch E — Other status logs

- **600–603** if line starts with one of:
  - `ENROLL_FAIL`
  - `DELETE_OK:`
  - `DELETE_FAIL:`
  - `ERR:`

  then debug-log terminal status.

- **605** function end.

---

## Behavioral summary (state machine view)

| Incoming line | Primary action | UI impact | Outgoing command |
|---|---|---|---|
| `READY` / `PONG` | ignore | none | none |
| `MATCH:<id>` + resolved | login success path | green status + page switch | `NAME:<displayName>` |
| `MATCH:<id>` unresolved/invalid | deny path | red status (login page) | `DENIED` |
| `NO_MATCH` | retry feedback | orange status | none |
| `ENROLL_OK:<id>` valid | capture/link flow | green or orange status | `LOGIN_ON` (via finish lambda) |
| `ENROLL_OK:<id>` invalid | error | red status | `LOGIN_ON` |
| `ENROLL_FAIL[:reason]` | fail flow | red status | `LOGIN_ON` |
| `DELETE_OK/FAIL`, `ERR:*` | diagnostic logging | none | none |

---

## Safety and UX guarantees in this function

1. **Never leaves enrollment locked** after terminal result (`finishEnrollmentFlow` on both `ENROLL_OK` and `ENROLL_FAIL`).
2. **Never trusts malformed IDs** (`MATCH`/`ENROLL_OK` parsing guarded with `ok` and positive check).
3. **Avoids stale UI context noise** by showing some messages only on login page.
4. **Keeps terminal protocol synchronized** (`DENIED`, `NAME:*`, `LOGIN_ON` are explicit and deterministic).
5. **Supports two enrollment targets**:
   - existing employee (immediate DB link),
   - new employee form (pending link until save).

---

## Continue to Part 1

For initialization, command sending, DB lookup/save helpers, and serial line buffering, open:

- [`FINGERPRINT_FUNCTION_LINE_BY_LINE.md`](./FINGERPRINT_FUNCTION_LINE_BY_LINE.md)
