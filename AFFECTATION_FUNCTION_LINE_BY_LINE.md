# Affectation Function — Line-by-Line Logic (from `mainwindow.cpp`)

## Navigation

- Part 1 (this file): `AFFECTATION_FUNCTION_LINE_BY_LINE.md`
- Part 2 (table rendering / remaining info / auto-assign): [`AFFECTATION_FUNCTION_LINE_BY_LINE_PART2.md`](./AFFECTATION_FUNCTION_LINE_BY_LINE_PART2.md)

This document explains, **line by line**, the logic of the main affectation save flow:

- `void MainWindow::on_affSaveBtn_clicked()` (lines ~1278–1418)
- helper `bool MainWindow::hasDuplicateAffectation(...) const` (lines ~1003–1012)
- helper `void MainWindow::prepareInsertAffectationQuery(...) const` (lines ~1014–1028)

> Note: line numbers refer to the current version of `mainwindow.cpp` in this workspace.

---

## 1) `hasDuplicateAffectation(int empId, int serieId) const`

### Code intent
Checks if row `(id_emp, id_serie)` already exists in `EMP_MACH`.

### Objective
Prevent creating duplicate affectations for the same employee-series pair before any insert/reinsert path.

### Inputs / outputs
- Input: `empId`, `serieId`
- Output: `true` if duplicate exists, else `false`

### Why this matters
Because `EMP_MACH` behavior in this flow treats `(id_emp, id_serie)` as a logical unique pair, this check avoids:
- duplicate UI rows,
- inconsistent edit behavior,
- avoidable DB errors on insert.

### Line-by-line

- **1003** `bool MainWindow::hasDuplicateAffectation(int empId, int serieId) const`
  - Declares helper returning `true` if duplicate composite key exists.
- **1004** `{`
  - Function body starts.
- **1005** `QSqlQuery qCheck;`
  - Creates SQL query object.
- **1006–1008** `qCheck.prepare("SELECT COUNT(*) ...")`
  - Prepares SQL counting matching rows for given employee + series.
- **1009** `qCheck.bindValue(":id_emp", empId);`
  - Binds employee id parameter.
- **1010** `qCheck.bindValue(":id_serie", serieId);`
  - Binds series id parameter.
- **1011** `qCheck.exec();`
  - Executes SQL query.
- **1012** `return qCheck.next() && qCheck.value(0).toInt() > 0;`
  - Returns `true` only if first row exists and count > 0.
- **1013** `}`
  - Function ends.

---

## 2) `prepareInsertAffectationQuery(...) const`

### Code intent
Prepares and binds a reusable INSERT into `EMP_MACH`.

### Objective
Centralize and standardize insert SQL binding so both:
- edit-with-key-change path,
- pure insert path

use the exact same SQL parameter mapping.

### Inputs / outputs
- Input: mutable `QSqlQuery& query`, `empId`, `serieId`, `poste`, `dateDeb`, `dateFinValue`
- Output: no direct return value; query becomes prepared + bound and ready for `exec()`.

### Why `dateFinValue` is `QVariant`
It allows this function to support both:
- concrete date (`QDate`) when fixed end-date is selected,
- typed NULL date when open-ended mode is enabled.

### Line-by-line

- **1015** `void MainWindow::prepareInsertAffectationQuery(QSqlQuery& query, ... ) const`
  - Declares helper that mutates passed query object.
- **1020** `{`
  - Function body starts.
- **1021–1023** `query.prepare("INSERT INTO EMP_MACH ...")`
  - Prepares insert statement with placeholders.
- **1024** `query.bindValue(":id_emp", empId);`
  - Binds employee id.
- **1025** `query.bindValue(":id_serie", serieId);`
  - Binds series id.
- **1026** `query.bindValue(":poste", poste);`
  - Binds position text.
- **1027** `query.bindValue(":date_debut", dateDeb);`
  - Binds start date.
- **1028** `query.bindValue(":date_fin", dateFinValue);`
  - Binds end date or typed NULL variant.
- **1029** `}`
  - Function ends.

---

## 3) `on_affSaveBtn_clicked()` — main affectation save flow

### High-level contract
- Input: form state (`affEmpCombo`, `affSerieCombo`, `affPosteCombo`, date widgets, open-ended checkbox).
- Output: persists assignment to `EMP_MACH` (insert/update/delete+insert depending on mode).
- Guards:
  1. Required employee + series
  2. Date consistency
  3. Max active affectations limit
  4. Duplicate `(id_emp, id_serie)` prevention
- Success path: reset edit state, reload table, refresh limit info, switch back to table page.

### Objective
Persist one affectation form submission safely while enforcing all business guards (required fields, date consistency, max active assignments, and duplicate pair protection).

### State model used by this function
- **Insert mode**: `m_editingAffIdEmp <= 0` or `m_editingAffIdSerie <= 0`
- **Edit mode**: both edit ids are set (> 0)
- **Open-ended affectation**: `openEnded == true`, therefore `date_fin = NULL`

### Decision table (core mode logic)

| Condition | DB action |
|---|---|
| Insert mode + non-duplicate | `INSERT` |
| Edit mode + same composite key | `UPDATE` non-key columns |
| Edit mode + changed composite key + non-duplicate | `DELETE` old key row then `INSERT` new key row |
| Any mode + duplicate target pair | Abort + warning |

### Line-by-line

- **1278** `void MainWindow::on_affSaveBtn_clicked()`
  - Slot called when user clicks affectation save button.
- **1279** `{`
  - Function body starts.
- **1281–1286** comment block
  - Documents insert/edit mode and composite key behavior.

#### Read current form values

- **1288** `int newEmpId = ui->affEmpCombo->currentData().toInt();`
  - Reads selected employee id from combo user data.
- **1289** `int newSerieId = ui->affSerieCombo->currentData().toInt();`
  - Reads selected series id from combo user data.
- **1290** `QString poste = ui->affPosteCombo->currentText();`
  - Reads selected post label.
- **1291** `QDate dateDeb = ui->affDateDebEdit->date();`
  - Reads start date.
- **1292** `QDate dateFin = ui->affDateFinEdit->date();`
  - Reads end date.
- **1293** `const bool openEnded = (...)`
  - True when checkbox means no end date.

#### Validate mandatory fields and dates

- **1295** `if (newEmpId <= 0 || newSerieId <= 0) {`
  - Rejects invalid or missing selections.
- **1296–1297** `QMessageBox::warning(..."Champs requis"...)`
  - Shows user message for missing required selections.
- **1298** `return;`
  - Stops save flow.
- **1299** `}`
  - End first validation block.
- **1300** `if (!openEnded && dateFin < dateDeb) {`
  - If end date is active, enforce `dateFin >= dateDeb`.
- **1301–1302** warning message
  - Explains invalid date ordering.
- **1303** `return;`
  - Stops save flow.
- **1304** `}`
  - End second validation block.

#### Convert date-fin into DB value (real date or typed NULL)

- **1306** `const QVariant dateFinValue = openEnded`
  - Starts ternary assignment.
- **1307** `? QVariant(QMetaType(QMetaType::QDate))`
  - Uses typed null for date when open-ended.
- **1308** `: QVariant(dateFin);`
  - Otherwise uses chosen end date.

#### Decide if operation increases employee active assignment count

- **1312** `const bool isEditMode = (...)`
  - Edit mode if original composite key tracking is set.
- **1313** `const bool increasesTargetEmployeeCount = ...`
  - In edit mode, only true when reassigned to another employee; always true in insert mode.

#### Enforce max active affectations limit when relevant

- **1315** `if (increasesTargetEmployeeCount && m_maxAffectationsPerEmployee > 0) {`
  - Checks limit only when this operation would increase active count.
- **1316** `QSqlQuery qCount;`
  - Query object for counting active assignments.
- **1317–1321** `qCount.prepare("SELECT COUNT(*) ... date_fin IS NULL")`
  - SQL count of active assignments for selected employee.
- **1322** `qCount.bindValue(":id_emp", newEmpId);`
  - Binds target employee id.
- **1323** `if (!qCount.exec() || !qCount.next()) {`
  - Handles DB failure / empty result.
- **1324–1326** critical message
  - Shows detailed SQL error text.
- **1327** `return;`
  - Stops save flow on DB error.
- **1328** `}`
  - End DB error block.
- **1330** `const int currentCount = qCount.value(0).toInt();`
  - Reads current active count.
- **1331** `if (currentCount >= m_maxAffectationsPerEmployee) {`
  - Checks if configured limit already reached/exceeded.
- **1332–1336** warning message
  - Shows current count + configured maximum.
- **1337** `return;`
  - Stops save flow due to limit.
- **1338** `}`
  - End limit block.
- **1339** `}`
  - End max-limit section.

##### Why limit is conditional (`increasesTargetEmployeeCount`)
If user edits an existing row without moving it to another employee, the active count for that employee does not increase. Blocking that edit would be wrong. So limit check only blocks true count-increase operations.

#### Prepare final SQL action

- **1341** `QSqlQuery q;`
  - Main query object for update or insert.

##### EDIT mode branch

- **1343** `if (isEditMode) {`
  - Enter edit flow.
- **1347** `if (newEmpId == m_editingAffIdEmp && newSerieId == m_editingAffIdSerie) {`
  - Case A: composite key unchanged.
- **1349–1355** `q.prepare("UPDATE EMP_MACH SET ... WHERE id_emp...AND id_serie...")`
  - Prepares in-place update for non-key fields.
- **1356** `q.bindValue(":poste", poste);`
  - Binds post.
- **1357** `q.bindValue(":date_debut", dateDeb);`
  - Binds start date.
- **1358** `q.bindValue(":date_fin", dateFinValue);`
  - Binds end date/null.
- **1359** `q.bindValue(":id_emp", m_editingAffIdEmp);`
  - Binds original employee key.
- **1360** `q.bindValue(":id_serie", m_editingAffIdSerie);`
  - Binds original series key.
- **1361** `} else {`
  - Case B: composite key changed.
- **1363** `if (hasDuplicateAffectation(newEmpId, newSerieId)) {`
  - Prevent duplicate destination pair.
- **1364–1365** warning duplicate message
  - Tells user assignment already exists.
- **1366** `return;`
  - Stops flow on duplicate.
- **1367** `}`
  - End duplicate check.
- **1369** `QSqlQuery qDel;`
  - Query object for deleting old key row.
- **1370–1372** `qDel.prepare("DELETE FROM EMP_MACH WHERE id_emp...id_serie...")`
  - Prepares delete of old composite key row.
- **1373** `qDel.bindValue(":old_emp", m_editingAffIdEmp);`
  - Binds old employee key.
- **1374** `qDel.bindValue(":old_serie", m_editingAffIdSerie);`
  - Binds old series key.
- **1375** `if (!qDel.exec()) {`
  - Executes delete and checks failure.
- **1376–1378** critical message
  - Shows SQL error if old row deletion fails.
- **1379** `return;`
  - Stops flow on delete error.
- **1380** `}`
  - End delete error block.
- **1382** `prepareInsertAffectationQuery(q, newEmpId, newSerieId, poste, dateDeb, dateFinValue);`
  - Reuses helper to prepare insert for new key row.
- **1383** `}`
  - End changed-key branch.
- **1384** `} else {`
  - End edit mode, begin insert mode.

##### INSERT mode branch

- **1387** `if (hasDuplicateAffectation(newEmpId, newSerieId)) {`
  - Duplicate guard before insert.
- **1388–1389** warning duplicate message
  - Tells user pair already exists.
- **1390** `return;`
  - Stops flow.
- **1391** `}`
  - End duplicate block.
- **1392** `prepareInsertAffectationQuery(q, newEmpId, newSerieId, poste, dateDeb, dateFinValue);`
  - Prepares insert SQL and bindings.
- **1393** `}`
  - End insert mode branch.

#### Execute prepared query and finalize UI

- **1395** `if (!q.exec()) {`
  - Executes prepared update/insert and checks failure.
- **1396–1397** critical message
  - Displays DB error text.
- **1398** `return;`
  - Stops on SQL execution error.
- **1399** `}`
  - End SQL failure block.
- **1401** `bool wasEdit = (m_editingAffIdEmp > 0);`
  - Captures previous mode for success message text.
- **1402** `resetAffectationEditState();`
  - Clears edit markers and restores save-button label.
- **1403** `loadAffectationTable();`
  - Reloads table data from DB.
- **1404** `updateAffectationRemainingInfo();`
  - Recomputes remaining slots and save availability.
- **1405** `ui->affStack->setCurrentIndex(1);`
  - Returns to table page.
- **1406–1408** success message
  - Shows mode-specific success text (`modifiée` vs `enregistrée`).
- **1409** `}`
  - Function ends.

### Failure modes handled explicitly
1. Missing employee/series selection.
2. Invalid date order when end date is used.
3. DB error when counting current active affectations.
4. Max affectation policy reached.
5. Duplicate employee-series pair.
6. DB error deleting old row (edit with key change).
7. DB error on final update/insert execution.

### UI side-effects on success
- Edit mode markers are reset (`resetAffectationEditState`).
- Table is reloaded from DB (`loadAffectationTable`).
- Remaining capacity label/button are recomputed (`updateAffectationRemainingInfo`).
- View returns to list page (`affStack` index 1).
- User receives mode-aware success message.

---

## 4) Why this logic is safe

- It enforces business constraints before writing:
  - required data,
  - date integrity,
  - max active affectations,
  - duplicate prevention.
- It handles the composite key correctly:
  - same key => `UPDATE`,
  - key changed => `DELETE` old + `INSERT` new.
- It keeps UI in sync after success:
  - resets mode,
  - reloads table,
  - recomputes remaining capacity.

### Additional implementation notes
- The function currently performs multiple DB operations (delete then insert) without an explicit transaction block. If needed for stricter atomicity, it can be wrapped in a transaction (`db.transaction()/commit()/rollback()`).
- Existing logic is still robust for most single-user UI workflows because each failure path aborts immediately and informs the user.

---

## 5) Continue reading (Part 2)

For detailed line-by-line analysis of:

- `loadAffectationTable()` (row rendering + edit/delete button wiring),
- `updateAffectationRemainingInfo()` (limit label + save button state),
- `tryAutoAssignForSerie()` (auto affectation selection algorithm),

open: [`AFFECTATION_FUNCTION_LINE_BY_LINE_PART2.md`](./AFFECTATION_FUNCTION_LINE_BY_LINE_PART2.md)
