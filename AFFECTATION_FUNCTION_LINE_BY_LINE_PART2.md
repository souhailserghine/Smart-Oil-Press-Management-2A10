# Affectation Functions — Line-by-Line Logic (Part 2)

## Navigation

- Part 1 (save flow + insert helpers): [`AFFECTATION_FUNCTION_LINE_BY_LINE.md`](./AFFECTATION_FUNCTION_LINE_BY_LINE.md)
- Part 2 (this file): `AFFECTATION_FUNCTION_LINE_BY_LINE_PART2.md`

This document follows the same format as `AFFECTATION_FUNCTION_LINE_BY_LINE.md`, with explicit objective + line-by-line behavior for:

1. `updateAffectationRemainingInfo()`
2. `loadAffectationTable()`
3. `tryAutoAssignForSerie()`

Source file: `mainwindow.cpp`

---

## 1) `updateAffectationRemainingInfo()` (starts around line 918)

### Objective
Compute how many **active affectations** the currently selected employee still has available, update the information label (`affRemainingInfoLabel`), and enable/disable the save button (`affSaveBtn`) accordingly.

### Inputs / outputs
- Input: selected employee id from `affEmpCombo`, plus edit-state members and `m_maxAffectationsPerEmployee`
- Output:
  - informational text/style in `affRemainingInfoLabel`
  - enabled/disabled state of `affSaveBtn`

### UX policy encoded here
- No employee selected => save disabled + instructional message.
- DB count failure => save disabled + red error message.
- Limit reached in count-increase scenario => save disabled + red "limite atteinte".
- Edit that does not increase count => allowed, even if remaining is zero.

### Line-by-line

- **918** `void MainWindow::updateAffectationRemainingInfo()`
  - Declares the function.
- **919** `{`
  - Starts function body.
- **926** `const bool hasRemainingLabel = (ui->affRemainingInfoLabel != nullptr);`
  - Pre-checks if label exists to avoid null access.
- **927–929** local lambda `setSaveEnabled`
  - Small helper to safely enable/disable `affSaveBtn`.
- **930** `bool canSave = true;`
  - Default allow-save state before checks.
- **932** read `empId` from `affEmpCombo`
  - Gets selected employee id (or `-1` if combo missing).

#### No employee selected branch

- **933** `if (empId <= 0) {`
  - Invalid/missing employee selection.
- **934** `canSave = false;`
  - Blocks save.
- **935–940** set informative neutral message + style
  - Tells user to choose an employee.
- **941** `setSaveEnabled(canSave);`
  - Applies disabled state.
- **942** `return;`
  - Stops here.
- **943** `}`
  - End branch.

#### Query active affectation count for selected employee

- **945** `QSqlQuery q;`
  - Creates SQL query object.
- **946–950** prepare SQL `SELECT COUNT(*) ... date_fin IS NULL`
  - Counts only **active** rows for `id_emp`.
- **951** bind `:id_emp`
  - Injects selected employee id.
- **952** execute + fetch check
  - If query fails or returns nothing:
- **953** `canSave = false;`
  - Blocks save on DB failure.
- **954–959** set red error message + style
  - Indicates remaining count cannot be computed.
- **960** apply disabled save state.
- **961** return.
- **962** end DB-failure block.

#### Compute remaining slots and whether current operation is allowed

- **964** `used = q.value(0).toInt();`
  - Current active assignments.
- **965** `remaining = qMax(0, max - used);`
  - Remaining slots (never negative).
- **966** `isEditMode = (m_editingAffIdEmp > 0 && m_editingAffIdSerie > 0);`
  - Detects edit mode.
- **967** `increasesCount = !isEditMode || (empId != m_editingAffIdEmp);`
  - In edit mode, only blocks if switching to another employee.
- **968** `limitBlocksSave = increasesCount && (remaining <= 0);`
  - True when save would violate max limit.
- **970** `canSave = !limitBlocksSave;`
  - Final save availability.

#### Update label text/color according to state

- **972** if label exists
  - Start label rendering.
- **973–980** if `limitBlocksSave`
  - Message: limit reached, style red bold.
- **981–988** else if `remaining <= 0 && isEditMode`
  - Message: modification allowed, style orange bold.
- **989–1000** else normal state message
  - Green when comfortable, orange warning when only 1 slot left.
- **1001** end label rendering block.

#### Commit save-button state

- **1003** `setSaveEnabled(canSave);`
  - Applies computed state.
- **1004** `}`
  - Function end.

### Edge cases handled
- Combo widget missing (`affEmpCombo == nullptr`): treated as invalid selection.
- Negative remaining values are clamped with `qMax(0, ...)`.
- Edit mode with zero remaining is still allowed when not increasing assignment count.

---

## 2) `loadAffectationTable()` (starts around line 1033)

### Objective
Load affectation rows from DB into `ui->affTable`, render status + row actions (Edit/Delete), and wire action buttons to edit/delete logic.

### Inputs / outputs
- Input: data from `EMP_MACH` + joined tables.
- Output:
  - fully rendered rows in `affTable`
  - per-row action widgets (Edit/Delete)
  - callback wiring for form prefill and row deletion

### Data mapping used
- Query columns map to table columns as follows:
  - DB `id_emp` -> UI col 0
  - full employee name -> col 1
  - series name -> col 2
  - machine name -> col 3
  - poste -> col 4
  - date_debut string -> col 5
  - date_fin string -> col 6
  - computed state -> col 7
  - action widget -> col 8

### Line-by-line

- **1033** function declaration.
- **1034** body start.
- **1041** `QTableWidget* t = ui->affTable;`
  - Table pointer.
- **1042** `t->setRowCount(0);`
  - Clears rows.
- **1043** `t->setSortingEnabled(false);`
  - Temporarily disables sorting during insertion.
- **1044** `t->setColumnCount(9);`
  - Sets expected table schema.
- **1045–1048** set header labels.

#### DB query for affectation listing

- **1053–1067** `QSqlQuery q("SELECT ...")`
  - Joins `EMP_MACH`, `EMPLOYE`, `SERIE_MACHINE`, `MACHINE`.
  - Includes formatted dates + computed status (`ACTIVE`/`TERMINEE`).

#### Local helper for non-editable cell insertion

- **1069–1073** lambda `setCell`
  - Creates non-editable `QTableWidgetItem` and inserts it.

#### Populate rows + status styling

- **1075** loop `while (q.next())`
  - Iterates DB rows.
- **1076–1077** append one table row.
- **1079–1087** fill columns from query values.
- **1088–1093** normalize status text, center align, and color:
  - green for active, red for terminated.

#### Build action buttons cell (Edit/Delete)

- **1097–1115** create action container, layout, two tool buttons.
- **1116–1118** add stretch + buttons.
- **1119** place widget in `Actions` column.

#### Wire Edit button behavior

- **1125** connect edit click lambda.
- **1127–1132** preselect employee + series in combos.
- **1134–1137** load `poste` from table and set combo index.
- **1139–1146** read displayed dates and set `affDateDebEdit`.
- **1147–1155** derive `openEnded` from date-fin emptiness and update checkbox/date-fin.
- **1157–1158** store original composite key into editing members.
- **1159** set save button text to `Modifier`.
- **1160** refresh remaining info label/button state.
- **1161** switch to form page (`affStack` index 0).
- **1162** end edit lambda.

##### Why row index is captured
The lambda captures `row` so it can read original visible values directly from the rendered table at click time (e.g., poste/date strings).

#### Wire Delete button behavior

- **1167** connect delete click lambda.
- **1168–1171** confirmation dialog; abort unless Yes.
- **1172–1178** prepare delete SQL by composite key.
- **1179–1180** bind `id_emp`, `id_serie`.
- **1181–1182** on success: remove row from table.
- **1183–1185** on failure: show SQL error.
- **1186** end delete lambda.
- **1187** end population loop.

##### Delete safety behavior
- Always asks confirmation first.
- Uses composite key (`id_emp`, `id_serie`) to target exactly one affectation relation.
- On SQL error, keeps UI unchanged and shows detailed error.

#### Final table presentation + filter sync

- **1190–1198** set header resize modes per column.
- **1199** re-enable sorting.
- **1202** `filterAffTable();`
  - Re-applies current client-side filters after reload.
- **1203** function end.

### Performance/behavior note
Sorting is disabled while inserting rows and re-enabled after population, preventing visual instability and index churn during row append operations.

---

## 3) `tryAutoAssignForSerie(int serieId, QString& detailMessage)` (starts around line 1586)

### Objective
Try to automatically assign a series to the **best available employee** (lowest active load, not already assigned to that series, below max limit), then return success/failure with a human-readable message.

### Inputs / outputs
- Input: `serieId`, current policy `m_maxAffectationsPerEmployee`
- Output:
  - return `true` on successful auto-insert, `false` otherwise
  - `detailMessage` always carries user-facing result reason

### Selection strategy (business meaning)
The candidate query enforces all of these simultaneously:
1. Employee is not already actively assigned to the same series.
2. Employee has active assignment count `< max limit`.
3. Among valid employees, choose smallest active count (`cnt ASC`).
4. Tie-break by smallest employee id (`id_emp ASC`) for deterministic choice.

### Line-by-line

- **1586** function declaration.
- **1587** body start.
- **1588–1592** algorithm comments.

#### Basic guards

- **1594** `detailMessage.clear();`
  - Resets output message.
- **1595** invalid series guard (`serieId <= 0`).
- **1596** set error message for invalid series.
- **1597** return false.
- **1600** invalid max-affectation limit guard (`<= 0`).
- **1601** set invalid-limit message.
- **1602** return false.

#### Pick best candidate employee

- **1605** `QSqlQuery pick;`
  - Query object for candidate selection.
- **1606–1621** prepare ranked SQL selection:
  - starts from `EMPLOYE e`
  - left joins `EMP_MACH em` to compute active count `cnt`
  - excludes employees already actively assigned to given series (`NOT EXISTS`)
  - keeps only employees with `cnt < :max_aff`
  - orders by lowest `cnt`, then lowest `id_emp`
- **1623** bind `:serie`.
- **1624** bind `:max_aff`.
- **1626–1629** execute failure handling.
  - Sets DB error detail in message and returns false.
- **1631–1634** no-candidate handling.
  - Message: no employee available.
- **1636** `empId = pick.value(0).toInt();`
  - Reads chosen employee id.
- **1637–1640** validates chosen id > 0.

##### SQL semantics note
`SUM(CASE WHEN ... date_fin IS NULL THEN 1 ELSE 0 END)` means only active affectations count toward load balancing and max-limit checks.

#### Insert the automatic affectation

- **1642** `QSqlQuery qIns;`
  - Insert query.
- **1643–1645** prepare insert into `EMP_MACH(id_serie,id_emp)`.
- **1646–1647** bind series and employee ids.
- **1649–1652** execute failure handling.
  - Message contains SQL error.

#### Build friendly success message

- **1654** `QString empName;`
  - Optional display name.
- **1655** `QSqlQuery qEmp;`
  - Query employee full name.
- **1656** prepare select full name.
- **1657** bind chosen employee id.
- **1658–1659** if query returns row, store trimmed full name.
- **1661–1664** set final success detail message:
  - if no name, fallback to employee id.
  - else include full name.
- **1666** `return true;`
  - Indicates success.
- **1667** function end.

### Failure cases and messages
- Invalid `serieId`.
- Invalid global limit configuration.
- Candidate search SQL execution failure.
- No available candidate under constraints.
- Invalid selected employee id.
- Insert failure in `EMP_MACH`.

All these return `false` and include precise `detailMessage` text.

---

## Notes

- `loadAffectationTable()` is a **render + wiring** function (UI and callbacks).
- `updateAffectationRemainingInfo()` is a **policy/state feedback** function (limit math + UX state).
- `tryAutoAssignForSerie()` is a **selection algorithm + DB commit** function.

Together they cover display, validation/guarding, and automatic decision logic for affectation management.

## Continue reading (Part 1)

For the line-by-line breakdown of the central save slot and helper insert/duplicate methods, open:

- [`AFFECTATION_FUNCTION_LINE_BY_LINE.md`](./AFFECTATION_FUNCTION_LINE_BY_LINE.md)
