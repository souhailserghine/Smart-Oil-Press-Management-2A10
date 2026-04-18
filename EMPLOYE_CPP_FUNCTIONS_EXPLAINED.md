# `employe.cpp` — Full Function-by-Function Explanation

This document explains **all functions** found in `employe.cpp`.
It is written in simple language and is beginner-friendly.

---

## Quick role of this file

`employe.cpp` is the implementation of class `Employe`.
This class is responsible for:

- storing employee data in memory,
- inserting/updating/deleting employee rows in database,
- searching/listing employees,
- authenticating login by email + password,
- exposing last SQL error.

---

## Tiny dictionary (for Qt words used in this file)

 - `QString` = text
- `QDate` = date
- `QByteArray` = binary data (images/fingerprints/face model)
- `QSqlQuery` = object that runs SQL statements
- `QSqlQueryModel` = table-like model used to show query results in UI
- `QSqlError` = SQL/database error object
- `QVariant` = generic container for values (including NULL)

---

## Constructors

### 1) `Employe::Employe()`

**Type:** default constructor  
**Purpose:** creates an empty employee object with safe defaults.

### What it initializes

- `m_idEmp = -1` → means “not saved / no real id yet”.
- text fields (`m_nomEmp`, `m_prenomEmp`, `m_email`, `m_role`, `m_mdp`) empty.
- `m_dateEmbauche` empty date.
- binary fields (`m_photo`, `m_empreinte`, `m_modeleFaciale`) empty.

### Why this is useful

It gives a clean object state before filling data from UI/database.

---

### 2) `Employe::Employe(int id_emp, const QString& nom_emp, ... )`

**Type:** parameterized constructor  
**Purpose:** creates an employee object with full data immediately.

### Input

- all important employee fields (id, name, email, role, password, date, photo, fingerprint, face model).

### Behavior

Stores each input directly in member variables.

### Why this is useful

When you already have full employee data and want one-line object creation.

---

## Getters

Getters return current member values; they do not modify anything.

### 3) `int getIdEmp() const`
Returns employee id.

### 4) `QString getNomEmp() const`
Returns last name.

### 5) `QString getPrenomEmp() const`
Returns first name.

### 6) `QString getEmail() const`
Returns email.

### 7) `QString getRole() const`
Returns role.

### 8) `QString getMdp() const`
Returns password value currently in object.

### 9) `QDate getDateEmbauche() const`
Returns hire date.

### 10) `QByteArray getPhoto() const`
Returns stored photo bytes.

### 11) `QByteArray getEmpreinte() const`
Returns fingerprint bytes.

### 12) `QByteArray getModeleFaciale() const`
Returns face model bytes.

---

## Setters

Setters update one member field in memory.

### 13) `void setIdEmp(int id)`
Sets employee id.

### 14) `void setNomEmp(const QString& nom)`
Sets last name.

### 15) `void setPrenomEmp(const QString& prenom)`
Sets first name.

### 16) `void setEmail(const QString& email)`
Sets email.

### 17) `void setRole(const QString& role)`
Sets role.

### 18) `void setMdp(const QString& mdp)`
Sets password.

### 19) `void setDateEmbauche(const QDate& date)`
Sets hire date.

### 20) `void setPhoto(const QByteArray& photo)`
Sets photo bytes.

### 21) `void setEmpreinte(const QByteArray& emp)`
Sets fingerprint bytes.

### 22) `void setModeleFaciale(const QByteArray& m)`
Sets face model bytes.

---

## Error access

### 23) `QSqlError lastError() const`

**Purpose:** returns the last database error saved inside this object (`m_lastError`).

**When used:** after a function fails (`ajouter`, `modifier`, `supprimer`, etc.) to know exact reason.

---

## CRUD functions

---

### 24) `bool ajouter()`

**Purpose:** insert a new employee row into table `EMPLOYE`.

### Step-by-step behavior

1. Builds `INSERT INTO EMPLOYE (...) VALUES (...)` query.
2. Binds text fields from object members.
3. Handles binary fields carefully:
   - if photo is empty → bind typed SQL NULL,
   - else bind photo bytes.
4. Same logic for facial model (`modele_faciale`).
5. Executes query.
6. If execution fails:
   - saves `m_lastError`,
   - prints debug details,
   - returns `false`.
7. If insert succeeds:
   - runs `SELECT seq_emp.CURRVAL FROM DUAL` to get generated id,
   - stores id in `m_idEmp`,
   - returns `true`.

### Important details

- `id_emp` is generated in DB by Oracle trigger/sequence.
- This function intentionally does **not** insert fingerprint in shown SQL (only fields listed in query).

---

### 25) `bool supprimer(int id_emp)`

**Purpose:** delete one employee row by id.

### Step-by-step behavior

1. Prepares `DELETE FROM EMPLOYE WHERE id_emp = :id_emp`.
2. Binds provided id.
3. Executes query.
4. On failure:
   - stores `m_lastError`,
   - returns `false`.
5. On success returns `true`.

### Note

This function trusts caller to pass correct id.

---

### 26) `bool modifier()`

**Purpose:** update existing employee data in database.

### Special behavior about password

This function has two SQL paths:

#### Path A: `m_mdp` is empty
- It does **not** update password column.
- Keeps existing DB password unchanged.

#### Path B: `m_mdp` is not empty
- Includes `mdp = :mdp` in update query.
- Password is updated.

### Common update fields

Both paths update:
- `nom_emp`, `prenom_emp`, `email`, `role`, `photo`, `modele_faciale`.

### Binary handling

Same null-safe logic as `ajouter()`:
- empty binary -> typed NULL,
- non-empty -> real bytes.

### End behavior

- Execute update query.
- If fail: save `m_lastError`, debug-print details, return `false`.
- If success: return `true`.

---

### 27) `QSqlQueryModel* afficher()`

**Purpose:** load all employees (basic columns) into a model for table display.

### SQL used

- Selects: `id_emp, nom_emp, prenom_emp, email, role`
- Sorts by: `nom_emp, prenom_emp`

### Behavior

1. Creates new `QSqlQueryModel`.
2. Sets query.
3. Sets human-friendly headers (`ID`, `Nom`, `Prénom`, `Email`, `Rôle`).
4. If model has SQL error, stores `m_lastError`.
5. Returns model pointer.

### Note

BLOB fields are intentionally excluded (photo/fingerprint/face model not shown in list table).

---

### 28) `QSqlQueryModel* rechercher(const QString& critere, const QString& valeur)`

**Purpose:** search/filter employees based on selected criterion + search text.

### Internal mapping logic

It uses a map from user-visible names to real DB columns.
Examples:
- `Nom` -> `nom_emp`
- `Prénom` -> `prenom_emp`
- `Email` -> `email`
- `Rôle` -> `role`
- also supports English labels (`Name`, `Status`).

If criterion not recognized, defaults to `nom_emp`.

### SQL behavior

Builds query dynamically with chosen column:

- `WHERE UPPER(TO_CHAR(column)) LIKE UPPER(:val)`

This makes search:
- case-insensitive,
- partial match (`%text%`).

### Steps

1. Determine actual column from map.
2. Prepare SQL with that column.
3. Bind value as `%<valeur>%`.
4. Execute query.
5. Put query into new model.
6. Set headers.
7. Save `m_lastError` if model has error.
8. Return model.

---

## Authentication function

### 29) `int authenticate(const QString& email, const QString& mdp)`

**Purpose:** verify login credentials.

### SQL logic

- `SELECT id_emp FROM EMPLOYE WHERE email = :email AND mdp = :mdp`

### Behavior

1. Prepare query with email and password.
2. Execute query.
3. If one row found:
   - return employee id.
4. Else:
   - save `m_lastError`,
   - return `-1`.

### Meaning of return value

- `>= 0` → authentication success (employee id returned)
- `-1` → authentication failed or query issue

---

## Practical summary by category

### In-memory data management
- Constructors
- Getters
- Setters

### Database write operations
- `ajouter()`
- `modifier()`
- `supprimer(int)`

### Database read operations
- `afficher()`
- `rechercher(...)`
- `authenticate(...)`

### Error inspection
- `lastError()`

---

## Common edge cases this file handles

1. **Empty photo / face model**
   - Writes typed NULL instead of invalid binary.

2. **Password left empty during edit**
   - Keeps old password in DB.

3. **Unknown search criterion text**
   - Falls back safely to `nom_emp`.

4. **Insert generated id retrieval**
   - Reads `seq_emp.CURRVAL` after successful insert.

---

## One-line takeaway

`employe.cpp` is a complete employee data layer: it stores employee fields, performs CRUD and search/auth SQL operations, and exposes database errors in a reusable, UI-friendly way.

---

## Deep dissection (code-level details)

This part goes deeper than the summary above and explains the *exact behavior style* used in this file.

## A) Design style used in `Employe`

The class mixes two responsibilities:

1. **Entity/data object**
    - stores employee values in members (`m_nomEmp`, `m_email`, ...).
2. **Data access object (DAO)**
    - directly runs SQL for insert/update/delete/select/auth.

That means one instance can hold values *and* execute persistence operations from these values.

---

## B) Function contracts (input/output/error)

### `ajouter()`
- **Input source:** class members already set before call.
- **Output:** `true` on successful insert, `false` on failure.
- **Side effects:**
   - writes to DB table `EMPLOYE`,
   - may update `m_idEmp` with generated sequence value,
   - updates `m_lastError` on failure.

### `supprimer(int id_emp)`
- **Input:** explicit employee id.
- **Output:** bool success/failure.
- **Side effects:** deletes row, updates `m_lastError` if failure.

### `modifier()`
- **Input source:** class members (including `m_idEmp` target).
- **Output:** bool success/failure.
- **Side effects:** updates DB row, updates `m_lastError` on failure.

### `afficher()` / `rechercher()`
- **Output:** pointer to `QSqlQueryModel`.
- **Side effects:** caller owns/uses returned model pointer; errors copied into `m_lastError` if model reports error.

### `authenticate(email, mdp)`
- **Output:** employee id on success, `-1` otherwise.
- **Side effects:** sets `m_lastError` when no success path.

---

## C) Constructor dissection details

### Default constructor rationale

- `m_idEmp = -1` is a sentinel (placeholder id, not valid DB id).
- empty initialization avoids undefined values and accidental stale memory reads.

### Parameterized constructor rationale

- gives deterministic object state in one call,
- useful when loading one employee from a query and converting row → object.

---

## D) Getter/Setter section: why it exists though simple

Even if each method is one line, it provides:

- encapsulation (you can add validation later without changing callers),
- clearer API for UI/forms,
- future audit hooks (e.g., tracking changed fields).

---

## E) `ajouter()` ultra-detailed walkthrough

### 1) Prepared SQL

It uses placeholders (`:nom_emp`, etc.) instead of concatenating text.
Benefits:
- safer (no SQL injection from values),
- correct escaping handled by driver,
- reusable execution plan in DB engines.

### 2) Binding text values

The call sequence binds class fields directly.
If any required column is empty, DB constraints (if any) will decide failure/success.

### 3) Binary nullable handling

Code pattern:
- if byte array empty => bind *typed NULL* (`QVariant(QMetaType(QMetaType::QByteArray))`)
- else bind real bytes.

Why typed NULL matters:
- some DB drivers need explicit type for null to avoid ambiguity.

### 4) Execution and failure path

- if `exec()` fails, it copies `lastError()` into member `m_lastError`.
- debug log includes:
   - generic text,
   - driver-specific text,
   - database-specific text.

This helps identify whether issue is app-side bind, driver issue, or DB constraint.

### 5) Generated id retrieval

After success, function runs:
- `SELECT seq_emp.CURRVAL FROM DUAL`

Assumption:
- insert happened in same DB session/connection where sequence advanced.

Risk note:
- if trigger/sequence setup changes, this part can fail silently (insert still done, id not updated in object).

---

## F) `supprimer(int)` deep notes

Simple and direct.

Potential runtime outcomes:
- returns `true` even when id does not exist (DELETE affected 0 rows is not treated as error in many DBs).

If you need strict behavior “must delete exactly one row”, you’d check affected rows count.

---

## G) `modifier()` ultra-detailed walkthrough

This is the most nuanced CRUD method.

### 1) Password-preserving behavior

If `m_mdp` is empty:
- generated SQL omits `mdp` column,
- DB keeps previous password value.

This avoids accidentally clearing password when UI leaves password field blank during edit.

### 2) Dual-query-structure approach

The method prepares one of two SQL strings, then binds shared fields.
That keeps logic explicit and easier to audit than dynamic fragment concatenation.

### 3) Binary null handling repeated

Exactly same typed-null strategy as insert.

### 4) Target row selection

`WHERE id_emp = :id_emp` using member `m_idEmp`.

Important implication:
- caller must set `m_idEmp` correctly before calling `modifier()`.
- wrong id means wrong row update or no row update.

---

## H) `afficher()` deep notes

### Why `QSqlQueryModel*` is returned

Qt views (like table views) can consume model directly.

### Why select excludes BLOBs

Photo/fingerprint/face model are binary and heavy:
- expensive to transfer for list view,
- not readable in table cells,
- can slow UI.

### Header labels

`setHeaderData()` changes visible column titles without changing SQL schema.

---

## I) `rechercher()` deep notes

### 1) Criteria mapping (`QMap`)

Protects query from arbitrary UI labels:
- only known labels map to known DB columns.
- unknown key defaults to safe column (`nom_emp`).

### 2) Dynamic column insertion in SQL string

Only column name is dynamic (from controlled map), value is still bound parameter.

This is safer than full raw query concatenation.

### 3) `UPPER(TO_CHAR(...)) LIKE UPPER(:val)`

Behavior:
- case-insensitive matching,
- allows searching even if column type would need conversion to text.

Trade-off:
- may reduce index usage compared to exact-case indexed search.

### 4) `model->setQuery(std::move(query))`

Transfers query into model cleanly and avoids unnecessary copy semantics.

---

## J) `authenticate()` deep notes

### Logic

Checks email+password exact match.

### Security note (important)

Current code compares plaintext password in DB (`mdp = :mdp`).
That means passwords appear to be stored unhashed.

Recommended future hardening:
- store hash (e.g., bcrypt/argon2),
- compare hash server-side/application-side,
- never store raw password.

### Return convention

- success: valid `id_emp` integer
- fail: `-1`

This convention is easy for UI login flow (`if id < 0 -> reject`).

---

## K) Error handling pattern across file

Common pattern in DB methods:

1. execute query,
2. if fail => `m_lastError = query.lastError();`
3. return failure marker (`false` or `-1`).

This gives two channels:
- simple boolean/int for flow control,
- detailed `lastError()` for diagnostics.

---

## L) Hidden assumptions in this file

1. Active DB connection already exists before calling these methods.
2. Oracle sequence/trigger (`seq_emp`, `trg_emp`) are configured correctly.
3. Table/column names are stable and exactly match SQL strings.
4. Caller handles model memory/lifecycle correctly in UI layer.

---

## M) Suggested mental model for reading this file

For each method, ask these 4 questions:

1. Where does input come from? (member fields or method params)
2. What SQL is executed?
3. What is returned on success/failure?
4. What side effect happens on object state (`m_idEmp`, `m_lastError`)?

If you apply these 4 to every method above, the full file becomes very predictable.

