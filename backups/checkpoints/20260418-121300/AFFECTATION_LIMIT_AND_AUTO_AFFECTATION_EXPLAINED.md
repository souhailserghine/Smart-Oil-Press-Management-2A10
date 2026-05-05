# Affectation Limit + Auto Affectation
## Full Code Dissection (Tiny-Part Explanation, Beginner Friendly)

You said: **"dissect the code and explain each tiny part"**.
This file does exactly that, in very simple language.

I will explain:
1. what each important variable means,
2. what each key function does,
3. what each important line/block is doing,
4. where this is in your code.

---

## 0) Quick translation of Qt words (so code is readable)

When you see these names:

- `QString` → text
- `QDate` → date
- `QSqlQuery` → object that runs SQL
- `QSqlError` → SQL error details
- `QVariant` → flexible box that can hold different types
- `QDataStream` → reads/writes binary file data
- `QFile` → file object
- `QComboBox` → dropdown
- `QCheckBox` → checkbox
- `QLabel` → text label
- `QMessageBox` → popup message

So when reading code, mentally replace:
- `QString` with “text”,
- `QSqlQuery` with “SQL command runner”.

---

## 1) Core variables (in `mainwindow.h`) — tiny explanation

### `int m_maxAffectationsPerEmployee = 3;`
- This is the **limit**.
- Default value is 3.
- Meaning: one employee can have at most 3 **active** assignments.

### `bool m_autoAssignFromStock = false;`
- ON/OFF switch for automatic assignment from stock.
- `false` by default = auto mode off.

### `int m_editingAffIdEmp = -1;`
### `int m_editingAffIdSerie = -1;`
- These two tell the app if user is editing an existing assignment.
- `-1` means “not editing” (new insert mode).

### `QCheckBox* m_settingsAutoAssignCheck`
- Pointer to auto-assign checkbox in Settings page.

### `QComboBox* m_stockSerieCombo`
- Pointer to series dropdown in stock form.

### `QComboBox* m_affStatusFilterCombo`
- Pointer to filter dropdown (`Toutes`, `Actives`, `Terminées`).

### `QCheckBox* m_affOpenEndedCheck`
- Pointer to checkbox “active assignment without end date”.

---

## 2) Startup wiring (constructor part in `mainwindow.cpp`)

In constructor, these calls are important:

- `setupAffectationStatusFilter();`
- `setupAffectationOpenEndedOption();`
- `setupSettingsAutoAssignOption();`
- `ensureStockSerieSelector();`
- `refreshStockSerieChoices();`
- `loadAffectationSettings();`

### Why this order matters
- First create needed UI controls.
- Then fill dropdown data.
- Then load saved settings (limit + auto mode).

If you load settings before controls exist, values cannot be displayed.

---

## 3) `populateAffCombos()` — dissected

Location: `mainwindow.cpp`

### Code intent
Fill the 2 dropdowns in affectation form:
- employee dropdown,
- series dropdown.

### Tiny breakdown

1. `ui->affEmpCombo->clear();`
   - remove old items (avoid duplicates).

2. SQL query loads employee id + full name.

3. `while (qEmp.next()) ... addItem(name, id)`
   - each row from DB becomes one dropdown item.
   - visible text = full name.
   - hidden value = employee ID.

4. Same concept for series dropdown:
   - query series,
   - display machine name + series name.

5. `updateAffectationRemainingInfo();`
   - after dropdown refresh, recalculate remaining slots.

---

## 4) `setupAffectationStatusFilter()` — dissected

Goal: add dropdown to filter table by status.

### Tiny breakdown

1. `if (m_affStatusFilterCombo) return;`
   - safety: don’t create it twice.

2. `if (!ui->affSearchRow) return;`
   - safety: if layout missing, stop.

3. Create dropdown and add:
   - `Toutes`
   - `Actives`
   - `Terminées`

4. Add dropdown into search row layout.

5. Connect change event:
   - whenever user changes status filter,
   - call `filterAffTable()`.

---

## 5) `setupAffectationOpenEndedOption()` — dissected

Goal: allow active assignment with no end date.

### Tiny breakdown

1. Skip if checkbox already exists.
2. Skip if required UI controls are missing.
3. Create checkbox “Affectation active (sans date fin)”.

4. Set it checked by default.
5. Add it to form grid.
6. Disable end-date input initially.

Then event connection:
- if checkbox is checked:
  - end-date input disabled.
- if unchecked:
  - end-date input enabled.
  - if date invalid, set a default date (`today + 1 month`).

---

## 6) `updateAffectationRemainingInfo()` — dissected line-by-line logic

This is one of the most important functions.

### Part A: Prepare state

- `hasRemainingLabel` checks if info label exists.
- `canSave = true` starts optimistic.

### Part B: Read selected employee

- `empId = current dropdown selected id`.
- if no employee selected:
  - show helper message,
  - disable save button,
  - return.

### Part C: Count active assignments for selected employee

SQL used:

```sql
SELECT COUNT(*)
FROM EMP_MACH
WHERE id_emp = :id_emp
  AND date_fin IS NULL
```

Meaning:
- only selected employee,
- only active rows (end date empty).

If SQL fails:
- show error text,
- disable save,
- return.

### Part D: Compute numbers

- `used` = SQL result.
- `remaining = max(0, limit - used)`.

Edit-mode logic:
- `isEditMode = true` when editing existing assignment.
- `increasesCount = true` if operation would add load.
- `limitBlocksSave = increasesCount && remaining <= 0`.

This is super important:
- if user edits without increasing count, save may still be allowed at limit.

### Part E: Update message color and text

- red when blocked,
- orange when edge case (edit allowed),
- green when normal.

### Part F: Enable/disable save button

- final source of truth: `ui->affSaveBtn->setEnabled(canSave)`.

---

## 7) `on_affSaveBtn_clicked()` — full dissection

This is the main manual save function.

### Step 1: Read form values

- employee id,
- series id,
- poste text,
- start date,
- end date,
- open-ended checkbox state.

### Step 2: Basic validation

- employee/series must be selected.
- if not open-ended, end date must be >= start date.

### Step 3: Build `dateFinValue`

- if open-ended:
  - store DB NULL for end date (active assignment).
- else:
  - store real end date.

### Step 4: Determine if operation increases load

- edit mode with same employee -> no increase.
- insert mode or reassignment to another employee -> increases.

### Step 5: enforce limit when increase is true

Query count active rows for target employee.
If current count >= limit -> block with warning.

### Step 6: choose save path

#### Path A: Edit mode + same composite key
- run `UPDATE EMP_MACH` for fields (`poste`, dates).

#### Path B: Edit mode + changed key
- check duplicate `(id_emp, id_serie)`.
- if duplicate -> block.
- else delete old row.
- then insert new row.

#### Path C: Insert mode
- check duplicate first.
- if no duplicate -> insert row.

### Step 7: final execution and UI refresh

- if SQL fails -> show error popup.
- else:
  - reset edit mode markers to `-1`,
  - set button text back to “Affecter”,
  - reload table,
  - recalculate remaining info,
  - return to table page,
  - show success popup.

---

## 8) `loadAffectationSettings()` — dissected

Goal: read `settings.dat` and apply saved values.

### Tiny breakdown

1. Get app data folder path.
2. Ensure folder exists (`mkpath`).
3. Build full path to `settings.dat`.

If file does not exist:
- keep defaults,
- push defaults into UI,
- update labels,
- return.

If file cannot open:
- same fallback behavior.

If file opens:
- read with `QDataStream`.
- read fields in order:
  - magic number,
  - version,
  - max limit,
  - optional auto flag (for version >= 2).

Then sanity check:
- magic must match expected value,
- version must be valid,
- max limit must be > 0.

If valid:
- apply loaded values to variables.

Finally:
- update UI widgets (spinbox/checkbox),
- update limit label,
- recalc remaining info.

---

## 9) `saveAffectationSettings()` — dissected

Goal: save current settings to `settings.dat`.

### Tiny breakdown

1. Read UI values:
   - spinbox -> limit,
   - checkbox -> auto mode.

2. Build app data folder/path.
3. Open file in write mode (truncate old content).
4. Write binary data:
   - magic number,
   - version = 2,
   - limit,
   - auto flag.

5. Return true only if stream status is OK.

---

## 10) `on_settingsSaveBtn_clicked()` — dissected

1. Calls `saveAffectationSettings()`.
2. If save fails -> error popup.
3. If save success:
   - updates limit label,
   - updates remaining info,
   - shows success popup with limit + auto mode state.

---

## 11) `setupSettingsAutoAssignOption()` — dissected

Goal: add auto-assign option in settings page dynamically.

### Tiny breakdown

1. If checkbox already exists -> return.
2. If settings form layout missing -> return.
3. Create label text.
4. Create checkbox text.
5. Add tooltip to explain behavior.
6. Insert both into settings form row.

Why dynamic creation?
- avoids heavy `.ui` file migration.

---

## 12) `ensureStockSerieSelector()` — dissected

Goal: guarantee stock form has series dropdown.

### Tiny breakdown

1. If dropdown already exists -> return.
2. If form layout missing -> return.
3. Try to find existing dropdown in UI by object name.
4. If found, reuse it.
5. If not found:
   - create label,
   - create dropdown,
   - set object name and tooltip,
   - insert into form row,
   - save pointer in `m_stockSerieCombo`.

---

## 13) `refreshStockSerieChoices()` — dissected

Goal: fill stock series dropdown from DB.

### Tiny breakdown

1. Ensure dropdown exists first.
2. Save previous selected value.
3. Clear dropdown and add placeholder item.
4. Query series table ordered by name/id.
5. If query has error:
   - add "error loading series" item,
   - return.
6. For each row:
   - add item with display text `name (ID x)`,
   - hidden data = id.
7. Restore previous selection when possible.

---

## 14) `on_ajouterqtoliveBtn_clicked()` — dissected

This is stock add + optional auto assignment entry point.

### Step-by-step tiny breakdown

1. Ensure series dropdown exists.
2. Read all form values:
   - stock name,
   - farmer first name text,
   - category,
   - date,
   - quantity text,
   - description,
   - selected series id.

3. Convert quantity text to number (`double`).
4. Validate:
   - stock name not empty,
   - quantity valid and positive,
   - series selected.

5. Optional farmer resolution:
   - create typed null integer for `id_agri`,
   - query `AGRICULTEUR` by name/prenom,
   - if found, replace null with real id.

6. Insert stock row into `STOCK` with all values.
7. If insert fails -> error popup, stop.

8. If auto mode is ON:
   - call `tryAutoAssignForSerie(serieId, detail)`.
   - build message text depending success/failure.

9. Clear form fields.
10. Reload stock table.
11. Show success popup, including auto-assign detail.

Important behavior:
- Stock insert remains successful even if auto assignment fails.

---

 ## 15) `tryAutoAssignForSerie(int serieId, QString& detailMessage)` — deep dissection

This is the automatic employee picker.

### Step A: early validation

- if series id invalid (`<=0`) -> return false + reason.
- if limit invalid (`<=0`) -> return false + reason.

### Step B: candidate selection SQL

The query searches one employee who is:
1. not already actively assigned to this series,
2. under max active limit,
3. with the smallest active count,
4. tie-break by smallest employee id.

So selection is fair and deterministic.

### Step C: execute query

- if SQL execution fails -> false + DB error message.
- if no row -> false + “no available employee”.
- if row has invalid id -> false.

### Step D: insert assignment row

Insert into `EMP_MACH (id_serie, id_emp)`.

- if insert fails -> false + reason.

### Step E: fetch employee display name

- query full name by id (for friendly popup text).

### Step F: return success

- message says either employee name or ID.
- function returns true.

---

## 16) What exactly counts as “active” everywhere

Single rule used in all critical places:

> Active assignment = `date_fin IS NULL`

Used in:
- limit counting (`updateAffectationRemainingInfo`, `on_affSaveBtn_clicked`),
- auto candidate selection (`tryAutoAssignForSerie`),
- table status display (`loadAffectationTable`).

If you change this rule, you must change all these places together.

---

## 17) Mini dissection of table status and filtering

### In `loadAffectationTable()`
- SQL builds status text:
  - if end date empty -> `ACTIVE`,
  - else -> `TERMINEE`.
- UI colors:
  - green for active,
  - red for terminated.

### In `filterAffTable()`
- Reads search text.
- Reads selected state filter.
- For each row:
  - checks if row is active (end-date cell empty),
  - checks if text matches employee/series/machine/poste,
  - hides row when conditions not met.

---

## 18) Common beginner confusion clarified

### “Why sometimes save button is disabled?”
Because `updateAffectationRemainingInfo()` can disable it when:
- no employee selected, or
- limit reached for operation that increases load.

### “Why editing is allowed when limit is full?”
Because editing same employee may not increase load.

### “Why stock can save but auto assignment fails?”
By design: stock insertion is primary; auto assignment is secondary.

### “What is `settings.dat` exactly?”
A local small binary file storing:
- limit number,
- auto mode on/off.

---

## 19) Practical maintenance checklist (when you edit this feature)

If you change business rules, verify these functions:

1. `updateAffectationRemainingInfo()`
2. `on_affSaveBtn_clicked()`
3. `tryAutoAssignForSerie(...)`
4. `loadAffectationTable()`
5. `loadAffectationSettings()` / `saveAffectationSettings()`

If one is forgotten, UI and behavior become inconsistent.

---

## 20) Final one-paragraph summary

Your limit/auto system is centered in `MainWindow`. The limit value and auto mode are stored in `m_maxAffectationsPerEmployee` and `m_autoAssignFromStock`, persisted in `settings.dat`, and loaded on startup. Manual saves check whether the operation increases active load and block only if the configured limit would be exceeded. Stock add can trigger automatic assignment through `tryAutoAssignForSerie`, which picks the least-loaded eligible employee not already active on the same series. Across all flows, the same definition is used: active assignment means `date_fin` is empty.
