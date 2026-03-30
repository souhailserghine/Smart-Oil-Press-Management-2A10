#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>

#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QIcon>
#include <QDate>
#include <QSize>
#include <QPainter>
#include <QAbstractItemView>

#include <QDoubleValidator>
#include <algorithm>
#include <QRegularExpressionValidator>

#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QPieSeries>
#include <QtCharts/QLegend>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupModule5();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ===================== Setup =====================

void MainWindow::setupModule5()
{
    ensureMachineTopBarVisible();
    ensureSerieComboInMachineForm();
    ensureSerieUiInAdvanced();
    ensureMachineTableColumns();
    setupValidatorsModule5();

    // تصحيح Label غلط في UI (statusmachine هو "Etat" مش "Serie")
    if (auto *lbl = findChild<QLabel*>("statusmachineLabel")) {
        lbl->setText("Etat machine");
    }

    // default: go consulter
    if (ui->metierspersonnel_2 && ui->consulterpersonnel_3)
        ui->metierspersonnel_2->setCurrentWidget(ui->consulterpersonnel_3);

    // connect runtime serie add
    if (btnAddSerie) {
        connect(btnAddSerie, &QPushButton::clicked, this, [this]() { addSerieFromForm(); });
    }
    if (btnRefreshAdvanced) {
        connect(btnRefreshAdvanced, &QPushButton::clicked, this, [this]() { refreshAdvancedAnalytics(); });
    }
    if (btnExportAdvanced) {
        connect(btnExportAdvanced, &QPushButton::clicked, this, [this]() {
            exportTableToCsv(advMachineTable, "analyse_machines_avancee.csv");
        });
    }

    if (dbOpen()) {
        fillSeriesCombo();
        loadMachines();
        updateMachineCharts();
        refreshAdvancedAnalytics();
    } else {
        QMessageBox::warning(this, "DB", "Connexion DB fermée. (Connection::create() ?)");
    }
}


void MainWindow::ensureMachineTopBarVisible()
{
    if (auto *top = findChild<QWidget*>("horizontalLayoutWidget_7")) {
        top->setGeometry(QRect(430, 10, 430, 60));
        if (auto *lay = qobject_cast<QHBoxLayout*>(top->layout())) {
            lay->setContentsMargins(0, 0, 0, 0);
            lay->setSpacing(14);
        }
    }

    const QList<QToolButton*> buttons = {
        findChild<QToolButton*>("btnConsulterMachines"),
        findChild<QToolButton*>("btnAjouterMachines"),
        findChild<QToolButton*>("btnStatMachines"),
        findChild<QToolButton*>("btnAvanceMachines")
    };

    for (auto *btn : buttons) {
        if (!btn) continue;
        btn->setMinimumWidth(84);
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setIconSize(QSize(24, 24));
    }
}

void MainWindow::setupValidatorsModule5()
{
    // Série: capacité numérique
    if (auto *cap = findChild<QLineEdit*>("capacitelineprod")) {
        auto *v = new QDoubleValidator(0, 1e12, 2, cap);
        v->setNotation(QDoubleValidator::StandardNotation);
        cap->setValidator(v);
        cap->setPlaceholderText("Ex: 1500");
    }

    // Machine + Série names: trim spaces (مش validator صارم باش ما يضايقش)
    auto trimOnEditFinished = [](QLineEdit* le){
        if (!le) return;
        QObject::connect(le, &QLineEdit::editingFinished, le, [le](){
            le->setText(le->text().trimmed());
        });
    };

    trimOnEditFinished(findChild<QLineEdit*>("nommachine"));
    trimOnEditFinished(findChild<QLineEdit*>("nomserielinemachine_2"));
    trimOnEditFinished(findChild<QLineEdit*>("nommachine_6"));

    // Date série
    if (auto *d = findChild<QDateEdit*>("datefonctioserienmachine_2")) {
        d->setCalendarPopup(true);
        if (!d->date().isValid()) d->setDate(QDate::currentDate());
    }
}

void MainWindow::ensureSerieComboInMachineForm()
{
    if (!ui->ajoutpersonnel_3) return;

    // formLayout_6 موجودة في UI متاعك
    auto *form = ui->ajoutpersonnel_3->findChild<QFormLayout*>("formLayout_6");
    if (!form) return;

    cbSerieMachine = ui->ajoutpersonnel_3->findChild<QComboBox*>("cbSerieMachine");
    if (!cbSerieMachine) {
        cbSerieMachine = new QComboBox(ui->ajoutpersonnel_3);
        cbSerieMachine->setObjectName("cbSerieMachine");
        form->addRow(new QLabel("Série machine", ui->ajoutpersonnel_3), cbSerieMachine);
    }
}

void MainWindow::ensureSerieUiInAdvanced()
{
    if (!ui->metieravancee_4) return;

    if (ui->metieravancee_4->layout()) {
        advTotalValue = ui->metieravancee_4->findChild<QLabel*>("advTotalValue");
        advActiveValue = ui->metieravancee_4->findChild<QLabel*>("advActiveValue");
        advCriticalValue = ui->metieravancee_4->findChild<QLabel*>("advCriticalValue");
        advAvgPerfValue = ui->metieravancee_4->findChild<QLabel*>("advAvgPerfValue");
        advMachineTable = ui->metieravancee_4->findChild<QTableWidget*>("advMachineTable");
        btnRefreshAdvanced = ui->metieravancee_4->findChild<QPushButton*>("btnRefreshAdvanced");
        btnExportAdvanced = ui->metieravancee_4->findChild<QPushButton*>("btnExportAdvanced");
        return;
    }

    auto *root = new QVBoxLayout(ui->metieravancee_4);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(12);

    auto *title = new QLabel("Métiers avancés : maintenance prédictive et analyse de performance", ui->metieravancee_4);
    title->setWordWrap(true);
    title->setStyleSheet("font-size:16px; font-weight:600;");
    root->addWidget(title);

    auto makeValue = [this](const QString &objectName) {
        auto *lbl = new QLabel("0", ui->metieravancee_4);
        lbl->setObjectName(objectName);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setMinimumHeight(46);
        lbl->setStyleSheet("font-size:20px; font-weight:700; background:#f5f7ef; border:1px solid #d9dfc8; border-radius:10px; padding:8px;");
        return lbl;
    };

    auto *kpiBox = new QGroupBox("Résumé intelligent", ui->metieravancee_4);
    auto *kpiGrid = new QGridLayout(kpiBox);
    advTotalValue = makeValue("advTotalValue");
    advActiveValue = makeValue("advActiveValue");
    advCriticalValue = makeValue("advCriticalValue");
    advAvgPerfValue = makeValue("advAvgPerfValue");

    kpiGrid->addWidget(new QLabel("Total machines", kpiBox), 0, 0);
    kpiGrid->addWidget(new QLabel("Machines actives", kpiBox), 0, 1);
    kpiGrid->addWidget(new QLabel("Risque critique", kpiBox), 0, 2);
    kpiGrid->addWidget(new QLabel("Performance moyenne", kpiBox), 0, 3);
    kpiGrid->addWidget(advTotalValue, 1, 0);
    kpiGrid->addWidget(advActiveValue, 1, 1);
    kpiGrid->addWidget(advCriticalValue, 1, 2);
    kpiGrid->addWidget(advAvgPerfValue, 1, 3);
    root->addWidget(kpiBox);

    auto *analysisBox = new QGroupBox("Analyse prédictive des machines", ui->metieravancee_4);
    auto *analysisLayout = new QVBoxLayout(analysisBox);

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRefreshAdvanced = new QPushButton("Actualiser l'analyse", analysisBox);
    btnRefreshAdvanced->setObjectName("btnRefreshAdvanced");
    btnExportAdvanced = new QPushButton("Exporter l'analyse", analysisBox);
    btnExportAdvanced->setObjectName("btnExportAdvanced");
    btnRow->addWidget(btnRefreshAdvanced);
    btnRow->addWidget(btnExportAdvanced);
    analysisLayout->addLayout(btnRow);

    advMachineTable = new QTableWidget(analysisBox);
    advMachineTable->setObjectName("advMachineTable");
    advMachineTable->setColumnCount(10);
    advMachineTable->setHorizontalHeaderLabels({"ID", "Machine", "Série", "Heures", "Température", "Âge (jours)", "Risque", "Performance", "Action prédite", "Recommandation"});
    advMachineTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    advMachineTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    advMachineTable->setSelectionMode(QAbstractItemView::SingleSelection);
    advMachineTable->horizontalHeader()->setStretchLastSection(true);
    for (int i = 0; i < advMachineTable->columnCount(); ++i)
        advMachineTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    analysisLayout->addWidget(advMachineTable);

    auto *legend = new QLabel("Risque critique : score >= 70 | Performance calculée à partir de l'état, température, heures de fonctionnement et ancienneté.", analysisBox);
    legend->setWordWrap(true);
    analysisLayout->addWidget(legend);

    root->addWidget(analysisBox);
}

void MainWindow::ensureMachineTableColumns()
{
    if (!ui->tablemachine) return;

    // columns: Id, Nom, Type, Date, Etat, Série, Actions
    ui->tablemachine->setColumnCount(7);
    ui->tablemachine->setHorizontalHeaderLabels(
        {"Id","Nom","Type","Date","Etat","Série","Actions"}
        );

    ui->tablemachine->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tablemachine->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tablemachine->setSelectionMode(QAbstractItemView::SingleSelection);

    for (int c = 0; c < 6; ++c)
        ui->tablemachine->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);

    ui->tablemachine->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    ui->tablemachine->setColumnWidth(6, 120);
}

// ===================== DB helpers =====================

bool MainWindow::dbOpen() const
{
    auto db = QSqlDatabase::database();
    return db.isValid() && db.isOpen();
}

int MainWindow::nextId(const QString& table, const QString& col) const
{
    QSqlQuery q;
    q.prepare("SELECT NVL(MAX(" + col + "),0)+1 FROM " + table);
    if(!q.exec() || !q.next()) return 1;
    return q.value(0).toInt();
}

QString MainWindow::mapMachineEtatToDb(const QString& uiText) const
{
    QString t = uiText.trimmed().toLower();
    if (t.startsWith("act")) return "ACTIVE";
    if (t.contains("maint")) return "MAINTENANCE";
    return "PANNE";
}

// ===================== robust finders =====================

QLineEdit* MainWindow::findMachineNameEdit() const
{
    if (auto *w = findChild<QLineEdit*>("nommachine")) return w;
    if (auto *w = findChild<QLineEdit*>("nommachine_15")) return w;
    if (auto *w = findChild<QLineEdit*>("nommachine_13")) return w;
    return ui->ajoutpersonnel_3 ? ui->ajoutpersonnel_3->findChild<QLineEdit*>() : nullptr;
}

QComboBox* MainWindow::findMachineTypeCombo() const
{
    if (auto *w = findChild<QComboBox*>("fonctionmachine")) return w;
    if (auto *w = findChild<QComboBox*>("fonctionmachine_8")) return w;
    return ui->ajoutpersonnel_3 ? ui->ajoutpersonnel_3->findChild<QComboBox*>() : nullptr;
}

QComboBox* MainWindow::findMachineEtatCombo() const
{
    // status combobox name changed many times in your UI history
    if (auto *w = findChild<QComboBox*>("statusmachine")) return w;
    if (auto *w = findChild<QComboBox*>("statusmachine_8")) return w;
    return nullptr; // if not present, we'll use default ACTIVE
}

QDateEdit* MainWindow::findMachineDateEdit() const
{
    if (auto *w = findChild<QDateEdit*>("datefonctionmachine")) return w;
    if (auto *w = findChild<QDateEdit*>("datefonctionmachine_5")) return w;
    if (auto *w = findChild<QDateEdit*>("datefonctionmachine_2")) return w;
    return ui->ajoutpersonnel_3 ? ui->ajoutpersonnel_3->findChild<QDateEdit*>() : nullptr;
}

// ===================== Serie combo fill =====================

void MainWindow::fillSeriesCombo()
{
    if (!cbSerieMachine) return;
    cbSerieMachine->clear();

    if (!dbOpen()) {
        cbSerieMachine->addItem("-- DB closed --", -1);
        return;
    }

    QSqlQuery q;
    if(!q.exec("SELECT id_serie, nom_serie FROM SERIE_MACHINE ORDER BY nom_serie")) {
        QMessageBox::critical(this, "SQL", q.lastError().text());
        cbSerieMachine->addItem("-- erreur --", -1);
        return;
    }

    while(q.next()) {
        cbSerieMachine->addItem(q.value(1).toString(), q.value(0).toInt());
    }

    if(cbSerieMachine->count() == 0)
        cbSerieMachine->addItem("-- aucun serie --", -1);
}

// ===================== Load machines + action buttons =====================

void MainWindow::loadMachines(const QString& whereSql, const QVariantList& binds)
{
    if (!dbOpen() || !ui->tablemachine) return;

    QString sql =
        "SELECT m.id_machine, m.nom_machine, m.type_machine, m.date_installation, m.etat_machine, s.nom_serie "
        "FROM MACHINE m JOIN SERIE_MACHINE s ON m.id_serie = s.id_serie ";

    if (!whereSql.trimmed().isEmpty())
        sql += whereSql + " ";

    sql += "ORDER BY " + orderByMachine;

    QSqlQuery q;
    q.prepare(sql);
    for (const auto &b : binds) q.addBindValue(b);

    if(!q.exec()) {
        QMessageBox::critical(this, "SQL", q.lastError().text());
        return;
    }

    ui->tablemachine->setRowCount(0);
    int row = 0;

    while(q.next()) {
        ui->tablemachine->insertRow(row);

        ui->tablemachine->setItem(row,0,new QTableWidgetItem(q.value(0).toString()));
        ui->tablemachine->setItem(row,1,new QTableWidgetItem(q.value(1).toString()));
        ui->tablemachine->setItem(row,2,new QTableWidgetItem(q.value(2).toString()));
        QDate d = q.value(3).toDate();
        ui->tablemachine->setItem(row,3,new QTableWidgetItem(d.isValid()? d.toString("yyyy-MM-dd") : ""));
        ui->tablemachine->setItem(row,4,new QTableWidgetItem(q.value(4).toString()));
        ui->tablemachine->setItem(row,5,new QTableWidgetItem(q.value(5).toString()));

        int idMachine = q.value(0).toInt();
        addActionsToMachineRow(row, idMachine);

        row++;
    }
}

void MainWindow::addActionsToMachineRow(int row, int idMachine)
{
    QWidget *container = new QWidget(ui->tablemachine);
    auto *h = new QHBoxLayout(container);
    h->setContentsMargins(0,0,0,0);
    h->setSpacing(6);
    h->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto *btnEdit = new QPushButton(container);
    btnEdit->setToolTip("Edit");
    btnEdit->setIcon(QIcon(":/img/edit.svg"));
    btnEdit->setIconSize(QSize(16,16));
    btnEdit->setFixedSize(32,24);
    btnEdit->setFocusPolicy(Qt::NoFocus);

    auto *btnDel = new QPushButton(container);
    btnDel->setToolTip("Delete");
    btnDel->setIcon(QIcon(":/img/delete.svg"));
    btnDel->setIconSize(QSize(16,16));
    btnDel->setFixedSize(32,24);
    btnDel->setFocusPolicy(Qt::NoFocus);

    h->addWidget(btnEdit);
    h->addWidget(btnDel);

    ui->tablemachine->setCellWidget(row, 6, container);
    ui->tablemachine->setRowHeight(row, 34);

    connect(btnEdit, &QPushButton::clicked, this, [this, idMachine](){
        handleEditMachine(idMachine);
    });
    connect(btnDel, &QPushButton::clicked, this, [this, idMachine](){
        handleDeleteMachine(idMachine);
    });
}

// ===================== Machine CRUD =====================

void MainWindow::saveMachineFromForm()
{
    if (busyMachine) return;
    busyMachine = true;

    if (!dbOpen()) {
        QMessageBox::warning(this, "DB", "Connexion fermée.");
        busyMachine = false;
        return;
    }

    auto *leNom = findMachineNameEdit();
    auto *cbType = findMachineTypeCombo();
    auto *cbEtatUi = findMachineEtatCombo();
    auto *de = findMachineDateEdit();

    QString nom = leNom ? leNom->text().trimmed() : "";
    QString type = cbType ? cbType->currentText().trimmed() : "";
    QDate dateInstall = de ? de->date() : QDate::currentDate();
    QString etatDb = mapMachineEtatToDb(cbEtatUi ? cbEtatUi->currentText() : "Actif");

    int serieId = cbSerieMachine ? cbSerieMachine->currentData().toInt() : -1;

    if (nom.isEmpty()) {
        QMessageBox::warning(this, "Machine", "Nom machine obligatoire.");
        if (leNom) leNom->setFocus();
        busyMachine = false;
        return;
    }
    if (type.isEmpty()) {
        QMessageBox::warning(this, "Machine", "Fonction/Type obligatoire.");
        if (cbType) cbType->setFocus();
        busyMachine = false;
        return;
    }
    if (serieId <= 0) {
        QMessageBox::warning(this, "Machine", "Ikhtar série machine 9bal.");
        if (cbSerieMachine) cbSerieMachine->setFocus();
        busyMachine = false;
        return;
    }

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    QSqlQuery q;
    bool ok = false;

    if (editMachineId != -1) {
        q.prepare("UPDATE MACHINE SET nom_machine=?, type_machine=?, etat_machine=?, date_installation=?, id_serie=? "
                  "WHERE id_machine=?");
        q.addBindValue(nom);
        q.addBindValue(type);
        q.addBindValue(etatDb);
        q.addBindValue(dateInstall);
        q.addBindValue(serieId);
        q.addBindValue(editMachineId);
        ok = q.exec();
    } else {
        q.prepare("INSERT INTO MACHINE (nom_machine, type_machine, etat_machine, date_installation, heures_fonctionnement, temperature_actuelle, id_serie) "
                  "VALUES (?, ?, ?, ?, 0, 0, ?)");
        q.addBindValue(nom);
        q.addBindValue(type);
        q.addBindValue(etatDb);
        q.addBindValue(dateInstall);
        q.addBindValue(serieId);
        ok = q.exec();

        if(!ok && q.lastError().text().toUpper().contains("ID_MACHINE")){
            int id = nextId("MACHINE","id_machine");
            q.clear();
            q.prepare("INSERT INTO MACHINE (id_machine, nom_machine, type_machine, etat_machine, date_installation, heures_fonctionnement, temperature_actuelle, id_serie) "
                      "VALUES (?, ?, ?, ?, ?, 0, 0, ?)");
            q.addBindValue(id);
            q.addBindValue(nom);
            q.addBindValue(type);
            q.addBindValue(etatDb);
            q.addBindValue(dateInstall);
            q.addBindValue(serieId);
            ok = q.exec();
        }
    }

    if(!ok) {
        db.rollback();
        QMessageBox::critical(this, "SQL", q.lastError().text());
        busyMachine = false;
        return;
    }

    db.commit();
    QMessageBox::information(this, "OK", (editMachineId==-1) ? "Machine ajoutée ✅" : "Machine modifiée ✅");

    editMachineId = -1;
    if (ui->ajoutermachine) ui->ajoutermachine->setText("Ajouter");
    if (leNom) leNom->clear();

    loadMachines();
    updateMachineCharts();
    refreshAdvancedAnalytics();

    busyMachine = false;
}

void MainWindow::handleEditMachine(int idMachine)
{
    if (!dbOpen()) return;

    QSqlQuery q;
    q.prepare("SELECT nom_machine, type_machine, etat_machine, date_installation, id_serie "
              "FROM MACHINE WHERE id_machine=?");
    q.addBindValue(idMachine);

    if(!q.exec() || !q.next()){
        QMessageBox::critical(this, "SQL", q.lastError().text());
        return;
    }

    if (ui->metierspersonnel_2 && ui->ajoutpersonnel_3)
        ui->metierspersonnel_2->setCurrentWidget(ui->ajoutpersonnel_3);

    auto *leNom = findMachineNameEdit();
    auto *cbType = findMachineTypeCombo();
    auto *cbEtatUi = findMachineEtatCombo();
    auto *de = findMachineDateEdit();

    if (leNom) leNom->setText(q.value(0).toString());
    if (cbType) {
        int idxT = cbType->findText(q.value(1).toString());
        cbType->setCurrentIndex(idxT >= 0 ? idxT : 0);
    }

    QString etDb = q.value(2).toString().toUpper();
    if (cbEtatUi) {
        int idxE = cbEtatUi->findText(etDb=="ACTIVE" ? "Actif" : "Non actif");
        if (idxE < 0) idxE = 0;
        cbEtatUi->setCurrentIndex(idxE);
    }

    QDate d = q.value(3).toDate();
    if (de && d.isValid()) de->setDate(d);

    fillSeriesCombo();
    int serieId = q.value(4).toInt();
    int idxS = cbSerieMachine->findData(serieId);
    cbSerieMachine->setCurrentIndex(idxS >= 0 ? idxS : 0);

    editMachineId = idMachine;
    if (ui->ajoutermachine) ui->ajoutermachine->setText("Enregistrer");
}

void MainWindow::handleDeleteMachine(int idMachine)
{
    auto reply = QMessageBox::question(this, "Delete",
                                       QString("Supprimer machine ID=%1 ?").arg(idMachine));
    if (reply != QMessageBox::Yes) return;

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    QSqlQuery q;
    q.prepare("DELETE FROM MACHINE WHERE id_machine=?");
    q.addBindValue(idMachine);

    if(!q.exec()){
        db.rollback();
        QMessageBox::critical(this, "SQL", q.lastError().text());
        return;
    }

    db.commit();
    QMessageBox::information(this, "OK", "Machine supprimée ✅");

    loadMachines();
    updateMachineCharts();
    refreshAdvancedAnalytics();
}

// ===================== Series insert (Advanced page) =====================

void MainWindow::addSerieFromForm()
{
    if (busySerie) return;
    busySerie = true;

    if (!dbOpen()) {
        QMessageBox::warning(this, "DB", "Connexion fermée.");
        busySerie = false;
        return;
    }

    QString nom = serieNom ? serieNom->text().trimmed() : "";
    bool okCap=false;
    double cap = serieCap ? serieCap->text().trimmed().toDouble(&okCap) : 0;
    QDate d = serieDate ? serieDate->date() : QDate::currentDate();
    QString etat = serieEtat ? serieEtat->currentText().trimmed() : "Actif";
    QString resp = serieResp ? serieResp->text().trimmed() : "";
    QString desc = serieDesc ? serieDesc->text().trimmed() : "";

    if (nom.isEmpty()) {
        QMessageBox::warning(this, "Serie", "Nom série obligatoire.");
        busySerie = false;
        return;
    }
    if (!okCap) {
        QMessageBox::warning(this, "Serie", "Capacité لازم تكون nombre.");
        busySerie = false;
        return;
    }
    if (resp.isEmpty()) resp = "Admin";

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    QSqlQuery q;
    bool ok=false;

    q.prepare("INSERT INTO SERIE_MACHINE (nom_serie, capacite_production, date_mise_service, etat_serie, responsable, description) "
              "VALUES (?, ?, ?, ?, ?, ?)");
    q.addBindValue(nom);
    q.addBindValue(cap);
    q.addBindValue(d);
    q.addBindValue(etat.toUpper());
    q.addBindValue(resp);
    q.addBindValue(desc.isEmpty() ? QVariant() : desc);
    ok = q.exec();

    if(!ok && q.lastError().text().toUpper().contains("ID_SERIE")){
        int id = nextId("SERIE_MACHINE","id_serie");
        q.clear();
        q.prepare("INSERT INTO SERIE_MACHINE (id_serie, nom_serie, capacite_production, date_mise_service, etat_serie, responsable, description) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?)");
        q.addBindValue(id);
        q.addBindValue(nom);
        q.addBindValue(cap);
        q.addBindValue(d);
        q.addBindValue(etat.toUpper());
        q.addBindValue(resp);
        q.addBindValue(desc.isEmpty() ? QVariant() : desc);
        ok = q.exec();
    }

    if(!ok){
        db.rollback();
        QMessageBox::critical(this, "SQL", q.lastError().text());
        busySerie = false;
        return;
    }

    db.commit();
    QMessageBox::information(this, "OK", "Série ajoutée ✅");

    fillSeriesCombo();
    busySerie = false;
}

// ===================== Series insert (Ajouter page - right form) =====================

void MainWindow::addSerieFromAddPage()
{
    if (busySerie) return;
    busySerie = true;

    if (!dbOpen()) {
        QMessageBox::warning(this, "DB", "Connexion fermée.");
        busySerie = false;
        return;
    }

    auto *leNom = findChild<QLineEdit*>("nomserielinemachine_2");
    auto *leCap = findChild<QLineEdit*>("capacitelineprod");
    auto *deDate = findChild<QDateEdit*>("datefonctioserienmachine_2");
    auto *cbEtat = findChild<QComboBox*>("statulineseriesmachine_2");

    QString nom = leNom ? leNom->text().trimmed() : "";

    bool okCap=false;
    double cap = leCap ? leCap->text().trimmed().toDouble(&okCap) : 0;
    QDate d = deDate ? deDate->date() : QDate::currentDate();
    QString etat = cbEtat ? cbEtat->currentText().trimmed() : "Actif";

    // ===== Control saisie =====
    if (nom.isEmpty()) {
        QMessageBox::warning(this, "Série", "Nom série obligatoire.");
        if (leNom) leNom->setFocus();
        busySerie = false;
        return;
    }
    if (!okCap || cap <= 0) {
        QMessageBox::warning(this, "Série", "Capacité لازم تكون nombre > 0.");
        if (leCap) leCap->setFocus();
        busySerie = false;
        return;
    }
    if (!d.isValid()) {
        QMessageBox::warning(this, "Série", "Date غير صحيحة.");
        if (deDate) deDate->setFocus();
        busySerie = false;
        return;
    }

    // default fields not موجودين في الصفحة
    QString resp = "Admin";
    QVariant desc = QVariant();

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    QSqlQuery q;
    bool ok=false;

    q.prepare("INSERT INTO SERIE_MACHINE (nom_serie, capacite_production, date_mise_service, etat_serie, responsable, description) "
              "VALUES (?, ?, ?, ?, ?, ?)");
    q.addBindValue(nom);
    q.addBindValue(cap);
    q.addBindValue(d);
    q.addBindValue(etat.toUpper());
    q.addBindValue(resp);
    q.addBindValue(desc);
    ok = q.exec();

    if(!ok && q.lastError().text().toUpper().contains("ID_SERIE")){
        int id = nextId("SERIE_MACHINE","id_serie");
        q.clear();
        q.prepare("INSERT INTO SERIE_MACHINE (id_serie, nom_serie, capacite_production, date_mise_service, etat_serie, responsable, description) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?)");
        q.addBindValue(id);
        q.addBindValue(nom);
        q.addBindValue(cap);
        q.addBindValue(d);
        q.addBindValue(etat.toUpper());
        q.addBindValue(resp);
        q.addBindValue(desc);
        ok = q.exec();
    }

    if(!ok){
        db.rollback();
        QMessageBox::critical(this, "SQL", q.lastError().text());
        busySerie = false;
        return;
    }

    db.commit();
    QMessageBox::information(this, "OK", "Série ajoutée ✅");

    // refresh combo + clear fields
    fillSeriesCombo();
    if (leNom) leNom->clear();
    if (leCap) leCap->clear();
    if (deDate) deDate->setDate(QDate::currentDate());
    if (cbEtat) cbEtat->setCurrentIndex(0);

    busySerie = false;
}

void MainWindow::refreshAdvancedAnalytics()
{
    if (!advMachineTable) return;

    advMachineTable->setRowCount(0);

    if (!dbOpen()) {
        if (advTotalValue) advTotalValue->setText("0");
        if (advActiveValue) advActiveValue->setText("0");
        if (advCriticalValue) advCriticalValue->setText("0");
        if (advAvgPerfValue) advAvgPerfValue->setText("0 %");
        return;
    }

    QSqlQuery q;
    QString sql =
        "SELECT m.id_machine, m.nom_machine, NVL(s.nom_serie, '-'), NVL(m.heures_fonctionnement,0), NVL(m.temperature_actuelle,0), m.date_installation, NVL(m.etat_machine,'ACTIVE') "
        "FROM MACHINE m LEFT JOIN SERIE_MACHINE s ON m.id_serie = s.id_serie ORDER BY m.id_machine";

    bool ok = q.exec(sql);
    if (!ok) {
        q.clear();
        ok = q.exec(
            "SELECT id_machine, nom_machine, '-' AS nom_serie, NVL(heures_fonctionnement,0), NVL(temperature_actuelle,0), date_installation, NVL(etat_machine,'ACTIVE') "
            "FROM MACHINE ORDER BY id_machine");
    }

    if (!ok) {
        QMessageBox::warning(this,
                             "Analyse avancee",
                             QString("Impossible de charger l'analyse avancee :\n%1")
                                 .arg(q.lastError().text()));
        return;
    }

    int total = 0;
    int active = 0;
    int critical = 0;
    int perfSum = 0;

    while (q.next()) {
        const int id = q.value(0).toInt();
        const QString machine = q.value(1).toString();
        const QString serie = q.value(2).toString();
        const double hours = q.value(3).toDouble();
        const double temp = q.value(4).toDouble();
        const QDate installDate = q.value(5).toDate();
        const QString etat = q.value(6).toString().trimmed().toUpper();
        const int ageDays = installDate.isValid() ? installDate.daysTo(QDate::currentDate()) : 0;

        int risk = 0;
        if (etat != "ACTIVE") risk += 30;
        if (temp >= 80) risk += 30;
        else if (temp >= 60) risk += 18;
        else if (temp >= 45) risk += 8;

        if (hours >= 5000) risk += 30;
        else if (hours >= 2500) risk += 20;
        else if (hours >= 1000) risk += 10;

        if (ageDays >= 3650) risk += 20;
        else if (ageDays >= 1825) risk += 10;

        risk = std::clamp(risk, 0, 100);

        int perf = 100;
        if (etat != "ACTIVE") perf -= 25;
        if (temp >= 80) perf -= 25;
        else if (temp >= 60) perf -= 15;
        else if (temp >= 45) perf -= 5;

        if (hours >= 5000) perf -= 25;
        else if (hours >= 2500) perf -= 15;
        else if (hours >= 1000) perf -= 8;

        if (ageDays >= 3650) perf -= 12;
        else if (ageDays >= 1825) perf -= 6;

        perf = std::clamp(perf, 0, 100);

        QString predictedAction;
        QString recommendation;
        if (risk >= 70) {
            predictedAction = "Maintenance urgente";
            recommendation = "Planifier une intervention immédiate et vérifier les pièces critiques.";
        } else if (risk >= 45) {
            predictedAction = "Contrôle planifié";
            recommendation = "Prévoir une maintenance préventive lors du prochain arrêt.";
        } else {
            predictedAction = "RAS";
            recommendation = "Surveillance normale, machine stable.";
        }

        const int row = advMachineTable->rowCount();
        advMachineTable->insertRow(row);
        advMachineTable->setItem(row, 0, new QTableWidgetItem(QString::number(id)));
        advMachineTable->setItem(row, 1, new QTableWidgetItem(machine));
        advMachineTable->setItem(row, 2, new QTableWidgetItem(serie));
        advMachineTable->setItem(row, 3, new QTableWidgetItem(QString::number(hours, 'f', 0)));
        advMachineTable->setItem(row, 4, new QTableWidgetItem(QString::number(temp, 'f', 1) + " °C"));
        advMachineTable->setItem(row, 5, new QTableWidgetItem(QString::number(ageDays)));
        advMachineTable->setItem(row, 6, new QTableWidgetItem(QString::number(risk) + " /100"));
        advMachineTable->setItem(row, 7, new QTableWidgetItem(QString::number(perf) + " %"));
        advMachineTable->setItem(row, 8, new QTableWidgetItem(predictedAction));
        advMachineTable->setItem(row, 9, new QTableWidgetItem(recommendation));

        total++;
        if (etat == "ACTIVE") active++;
        if (risk >= 70) critical++;
        perfSum += perf;
    }

    if (advTotalValue) advTotalValue->setText(QString::number(total));
    if (advActiveValue) advActiveValue->setText(QString::number(active));
    if (advCriticalValue) advCriticalValue->setText(QString::number(critical));
    if (advAvgPerfValue) advAvgPerfValue->setText(total > 0 ? QString::number(perfSum / total) + " %" : "0 %");
}

// ===================== Export =====================

void MainWindow::exportTableToCsv(QTableWidget* t, const QString& defaultName) const
{
    if (!t) return;

    QString path = QFileDialog::getSaveFileName(nullptr, "Export CSV", defaultName, "CSV (*.csv)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "Export", "Ma njamtch n7ell el fichier.");
        return;
    }

    QTextStream out(&f);

    int lastCol = t->columnCount() - 1; // exclude Actions
    if (lastCol < 0) lastCol = t->columnCount();

    for(int c=0;c<lastCol;++c){
        auto *h = t->horizontalHeaderItem(c);
        out << (h ? h->text() : "");
        if (c < lastCol-1) out << ";";
    }
    out << "\n";

    for(int r=0;r<t->rowCount();++r){
        for(int c=0;c<lastCol;++c){
            auto *it = t->item(r,c);
            out << (it ? it->text() : "");
            if (c < lastCol-1) out << ";";
        }
        out << "\n";
    }

    f.close();
    QMessageBox::information(nullptr, "Export", "Export terminé ✅");
}

// ===================== Charts (Stat page) =====================

void MainWindow::updateMachineCharts()
{
    if (!dbOpen()) return;

    QWidget* host = findChild<QWidget*>("chartStatusContainer_5");
    if (!host) host = ui->statPersonnel_3;
    if (!host) return;

    if (!host->layout()) {
        auto *lay = new QVBoxLayout(host);
        lay->setContentsMargins(10,10,10,10);
        lay->setSpacing(10);

        chartStatusView = new QChartView(host);
        lay->addWidget(chartStatusView);
    }

    if (!chartStatusView) {
        chartStatusView = host->findChild<QChartView*>();
        if (!chartStatusView) return;
    }

    QSqlQuery q;
    if(!q.exec("SELECT etat_machine, COUNT(*) FROM MACHINE GROUP BY etat_machine")) return;

    auto *series = new QPieSeries();
    while(q.next()){
        series->append(q.value(0).toString(), q.value(1).toInt());
    }

    auto *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Répartition des machines par état");
    chart->legend()->setAlignment(Qt::AlignBottom);

    chartStatusView->setChart(chart);
    chartStatusView->setRenderHint(QPainter::Antialiasing);
}

// ===================== Slots from UI =====================

void MainWindow::on_btnConsulterMachines_clicked()
{
    if (ui->metierspersonnel_2 && ui->consulterpersonnel_3)
        ui->metierspersonnel_2->setCurrentWidget(ui->consulterpersonnel_3);

    if (dbOpen()) loadMachines();
}

void MainWindow::on_btnAjouterMachines_clicked()
{
    if (ui->metierspersonnel_2 && ui->ajoutpersonnel_3)
        ui->metierspersonnel_2->setCurrentWidget(ui->ajoutpersonnel_3);

    if (dbOpen()) fillSeriesCombo();
}

void MainWindow::on_btnStatMachines_clicked()
{
    if (ui->metierspersonnel_2 && ui->statPersonnel_3)
        ui->metierspersonnel_2->setCurrentWidget(ui->statPersonnel_3);

    updateMachineCharts();
}

void MainWindow::on_btnAvanceMachines_clicked()
{
    if (ui->metierspersonnel_2 && ui->metieravancee_4)
        ui->metierspersonnel_2->setCurrentWidget(ui->metieravancee_4);

    refreshAdvancedAnalytics();
}

void MainWindow::on_ajoutermachine_clicked()
{
    saveMachineFromForm();
}

void MainWindow::on_rechrchemahine_clicked()
{
    QString txt = ui->recherchemachine ? ui->recherchemachine->text().trimmed() : "";
    if (txt.isEmpty()) { loadMachines(); return; }

    QString mode = ui->datmachinne ? ui->datmachinne->currentText().trimmed() : "Name";

    if (mode == "Date") {
        loadMachines("WHERE TO_CHAR(m.date_installation,'YYYY-MM-DD') = ?", {txt});
    } else if (mode == "Name") {
        loadMachines("WHERE UPPER(m.nom_machine) LIKE UPPER(?)", { "%" + txt + "%" });
    } else {
        loadMachines("WHERE UPPER(m.etat_machine) LIKE UPPER(?)", { "%" + txt + "%" });
    }
}

void MainWindow::on_filtrer_clicked()
{
    if (ui->recherchemachine) ui->recherchemachine->clear();
    loadMachines();
}

void MainWindow::on_oktrie_clicked()
{
    QString v = ui->trie ? ui->trie->currentText().trimmed().toLower() : "id";

    if (v == "id") orderByMachine = "m.id_machine";
    else if (v == "nom") orderByMachine = "m.nom_machine";
    else if (v == "datefonctionnement") orderByMachine = "m.date_installation";
    else if (v == "etat") orderByMachine = "m.etat_machine";
    else if (v == "type") orderByMachine = "m.type_machine";
    else orderByMachine = "m.id_machine";

    loadMachines();
}

void MainWindow::on_exportmachine_clicked()
{
    exportTableToCsv(ui->tablemachine, "machines.csv");
}

void MainWindow::on_exportermachinne_clicked()
{
    exportTableToCsv(ui->tablemachine, "machines.csv");
}

void MainWindow::on_ajouterlineseriemachine_2_clicked()
{
    addSerieFromAddPage();
}
