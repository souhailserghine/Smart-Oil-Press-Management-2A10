# Additional Refactoring Opportunities: MainWindow → Employe Model

## Analysis of Business Logic Still in MainWindow

After reviewing `mainwindow.cpp`, I identified **7 major areas** where database queries and business logic should be moved to the `Employe` model layer.

### 1. **Employee Name Lookup by ID** (Line 519-525)
**Current Location**: `mainwindow.cpp` - in login success handler  
**Query**: `SELECT nom_emp, prenom_emp FROM EMPLOYE WHERE id_emp = :id`

```cpp
// MainWindow (BAD - business logic in UI)
QSqlQuery q;
q.prepare("SELECT nom_emp, prenom_emp FROM EMPLOYE WHERE id_emp = :id");
q.bindValue(":id", userId);
if (q.exec() && q.next()) {
    const QString fullName = q.value(0).toString() + " " + q.value(1).toString();
}
```

**Should Move To**: `Employe::getFullNameById(int id) -> QString`

```cpp
// Employe Model (GOOD - business logic in model)
QString Employe::getFullNameById(int id)
{
    if (id <= 0 || !QSqlDatabase::database().isOpen()) return QString();
    QSqlQuery q;
    q.prepare("SELECT nom_emp, prenom_emp FROM EMPLOYE WHERE id_emp = :id");
    q.bindValue(":id", id);
    if (q.exec() && q.next()) {
        return (q.value(0).toString() + " " + q.value(1).toString()).trimmed();
    }
    m_lastError = q.lastError();
    return QString();
}
```

---

### 2. **Employee Combo Box Population** (Line 753-761)
**Current Location**: `mainwindow.cpp` - in `populateAffCombos()`  
**Query**: Joins EMPLOYE table, concatenates names

```cpp
// MainWindow (BAD - schema knowledge in UI)
QSqlQuery qEmp(
    "SELECT id_emp, nom_emp || ' ' || prenom_emp "
    "FROM EMPLOYE "
    "ORDER BY nom_emp, prenom_emp"
);
```

**Should Move To**: `Employe::getAllEmployeesWithNames() -> QList<QPair<int, QString>>`

---

### 3. **Affectation Assignment Count Check** (Line 834-853)
**Current Location**: `mainwindow.cpp` - in `updateAffectationRemainingInfo()`  
**Query**: `SELECT COUNT(*) FROM EMP_MACH WHERE id_emp = :id_emp`

```cpp
// MainWindow (BAD - business logic in UI)
QSqlQuery q;
q.prepare(
    "SELECT COUNT(*) "
    "FROM EMP_MACH "
    "WHERE id_emp = :id_emp"
);
q.bindValue(":id_emp", empId);
```

**Should Move To**: `Employe::countAssignments(int employeeId) -> int`

---

### 4. **Duplicate Assignment Check** (Line 895-902)
**Current Location**: `mainwindow.cpp` - in `hasDuplicateAffectation()`  
**Query**: `SELECT COUNT(*) FROM EMP_MACH WHERE id_emp = :id_emp AND id_serie = :id_serie`

```cpp
// MainWindow (BAD - duplicate check logic in UI)
QSqlQuery qCheck;
qCheck.prepare(
    "SELECT COUNT(*) FROM EMP_MACH "
    "WHERE id_emp = :id_emp AND id_serie = :id_serie"
);
qCheck.bindValue(":id_emp", empId);
qCheck.bindValue(":id_serie", serieId);
```

**Should Move To**: `Employe::hasAffectationFor(int empId, int serieId) -> bool`

---

### 5. **Assignment Query Preparation** (Line 905-921)
**Current Location**: `mainwindow.cpp` - in `prepareInsertAffectationQuery()`  
**Query**: INSERT into EMP_MACH with parameters

```cpp
// MainWindow (BAD - SQL construction in UI)
void MainWindow::prepareInsertAffectationQuery(QSqlQuery& query,
                                               int empId,
                                               int serieId,
                                               const QString& poste,
                                               const QDate& dateDeb,
                                               const QVariant& dateFinValue) const
{
    query.prepare(
        "INSERT INTO EMP_MACH (id_emp, id_serie, poste, date_debut, date_fin) "
        "VALUES (:id_emp, :id_serie, :poste, :date_debut, :date_fin)"
    );
    query.bindValue(":id_emp", empId);
    // ...
}
```

**Should Move To**: New class `EmployeeAssignment` or method in `Employe`

---

### 6. **Series/Machine Combo Population** (Line 763-770)
**Current Location**: `mainwindow.cpp` - in `populateAffCombos()`  
**Query**: Complex join with SERIE_MACHINE, MACHINE, EMPLOYE

```cpp
// MainWindow (BAD - complex schema join in UI)
QSqlQuery qSerie(
    "SELECT s.id_serie, "
    "       m.machine_name, "
    "       COUNT(em.id_emp) OVER (PARTITION BY s.id_serie) "
    "FROM SERIE_MACHINE s "
    "LEFT JOIN MACHINE m ON s.id_machine = m.id_machine "
    "LEFT JOIN EMP_MACH em ON s.id_serie = em.id_serie "
    "ORDER BY m.machine_name"
);
```

**Should Move To**: New `MachineAssignment` class or `getMachineSeriesWithCounts() -> QList`

---

### 7. **Affectation Table Loading** (Line ~1027+)
**Current Location**: `mainwindow.cpp` - in `loadAffectationTable()`  
**Query**: Multi-table join to display affectation records

```cpp
// MainWindow (BAD - complex reporting query in UI)
QSqlQuery q(
    "SELECT em.id_emp, em.nom_emp, em.prenom_emp, "
    "       s.id_serie, m.machine_name, "
    "       ea.poste, ea.date_debut, ea.date_fin "
    "FROM EMP_MACH ea "
    "JOIN EMPLOYE em ON ea.id_emp = em.id_emp "
    "JOIN SERIE_MACHINE s ON ea.id_serie = s.id_serie "
    "LEFT JOIN MACHINE m ON s.id_machine = m.id_machine"
);
```

**Should Move To**: `Employe::getAffectationRecords() -> QSqlQueryModel*`

---

## Summary Table

| ID | What | Current Location | Should Move To | Priority |
|----|----|------------------|-----------------|----------|
| 1 | Get employee full name by ID | mainwindow.cpp:519-525 | `Employe::getFullNameById(int)` | **HIGH** |
| 2 | Populate employee combo | mainwindow.cpp:753-761 | `Employe::getAllEmployees()` | **HIGH** |
| 3 | Count assignments | mainwindow.cpp:834-853 | `Employe::countAssignments(int)` | **HIGH** |
| 4 | Check duplicate assignment | mainwindow.cpp:895-902 | `Employe::hasAffectationFor(int,int)` | **HIGH** |
| 5 | Insert assignment query prep | mainwindow.cpp:905-921 | New `EmployeeAssignment` class | **MEDIUM** |
| 6 | Get machine series | mainwindow.cpp:763-770 | New `MachineAssignment::getSeries()` | **MEDIUM** |
| 7 | Load affectation table | mainwindow.cpp:~1027+ | `Employe::getAffectationRecords()` | **MEDIUM** |

---

## Benefits of Refactoring

| Benefit | Current | After Refactoring |
|---------|---------|-------------------|
| **MainWindow LOC** | 3293 | ~2900 |
| **DB Queries in UI** | 7 locations | 1 location (display/reporting only) |
| **Business Logic Centralization** | Scattered | All in model layer |
| **Testability** | Hard (requires UI) | Easy (just Employe class) |
| **Code Reuse** | Limited | Any view can use these queries |
| **Maintenance** | Schema changes = edit MainWindow | Schema changes = edit Employe |

---

## Recommended Refactoring Priority

### **Phase 1: HIGH PRIORITY** (Do first - improves maintainability immediately)
1. `Employe::getFullNameById(int id)` — Used in login flow
2. `Employe::countAssignments(int empId)` — Used in affectation management
3. `Employe::hasAffectationFor(int empId, int serieId)` — Prevents duplicates

### **Phase 2: MEDIUM PRIORITY** (Do next - cleans up UI)
4. `Employe::getAllEmployees()` — Combo box population
5. `Employe::getAffectationRecords()` — Table loading

### **Phase 3: LOW PRIORITY** (Optional - if database structure changes)
6. Create `EmployeeAssignment` helper class for complex joins
7. Create `MachineAssignment` for machine/series queries

---

## Implementation Strategy

### Before Making Changes
```bash
git tag backup/pre-additional-refactor-2026-05-01 HEAD
```

### After Implementation
- Remove 80+ lines from MainWindow
- Add 200-250 lines to Employe (all well-organized methods)
- Update MainWindow to call new Employe methods
- Run existing tests (or create new ones)

### Expected Outcome
- MainWindow becomes purely UI/navigation logic
- Employe becomes comprehensive data access layer
- Entire application follows MVC properly
- Very easy to add new views/modules that need employee data
