#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "citernes.h"
#include "stocks.h"
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QChart>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QHorizontalBarSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QIcon>
#include <QSize>
#include <QSizePolicy>
#include <QAbstractItemView>
#include <QAbstractButton>
#include <QString>
#include <QDialog>
#include <QLabel>
#include <QToolButton>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QParallelAnimationGroup>
#include <QGuiApplication>
#include <QScreen>
#include <QSqlQuery>
#include <QColorDialog>
#include <QSqlError>
#include <QTableWidgetItem>
#include <QDate>
#include <QRegularExpression>
#include <QDebug>
#include <QFile>
#include <QMenu>
#include <QTextStream>
#include <QFileDialog>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QHorizontalBarSeries>
#include <QtCharts/QPieSlice>
#include <QSqlDatabase>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QDateEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDoubleValidator>
#include <QScrollArea>
#include <QPrinter>
#include <QTextDocument>
#include <QDateTime>
#include <QMap>
#include <QPen>
#include <QPageSize>
#include <QPageLayout>
#include <algorithm>
#include <cmath>
#include <limits>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    machineSerial = new QSerialPort(this);
    connect(machineSerial, &QSerialPort::readyRead, this, &MainWindow::readMachineSerialData);

    // Bridge UI for Stocks advanced page so the integrated interface is not empty
    if (ui->metieravancee_2 && !ui->metieravancee_2->layout()) {
        auto *bridgeLayout = new QVBoxLayout(ui->metieravancee_2);
        bridgeLayout->setContentsMargins(24, 24, 24, 24);
        bridgeLayout->setSpacing(16);

        auto *title = new QLabel("Métiers avancés du stock", ui->metieravancee_2);
        title->setStyleSheet("font-size: 20px; font-weight: 700; color: #8a9b5f;");
        bridgeLayout->addWidget(title);

        auto *desc = new QLabel("Ce module est intégré dans une fenêtre Stocks harmonisée avec l'interface principale. Utilisez les actions ci-dessous pour ouvrir directement la section voulue.", ui->metieravancee_2);
        desc->setWordWrap(true);
        desc->setStyleSheet("color: #5a5d60; font-size: 11pt;");
        bridgeLayout->addWidget(desc);

        auto *row = new QHBoxLayout();
        auto *btnAdd = new QPushButton(QIcon(":/img/add.svg"), "Ajouter", ui->metieravancee_2);
        auto *btnConsult = new QPushButton(QIcon(":/img/search.svg"), "Consulter", ui->metieravancee_2);
        auto *btnStats = new QPushButton(QIcon(":/img/chart.svg"), "Statistiques", ui->metieravancee_2);
        auto *btnAdv = new QPushButton(QIcon(":/img/settings.svg"), "Avancé", ui->metieravancee_2);
        for (auto *b : {btnAdd, btnConsult, btnStats, btnAdv}) {
            b->setProperty("type", "primary");
            row->addWidget(b);
        }
        row->addStretch();
        bridgeLayout->addLayout(row);
        bridgeLayout->addStretch();

        QObject::connect(btnAdd, &QPushButton::clicked, this, [this]() { openStocksWindow(0); });
        QObject::connect(btnConsult, &QPushButton::clicked, this, [this]() { openStocksWindow(1); });
        QObject::connect(btnStats, &QPushButton::clicked, this, [this]() { openStocksWindow(2); });
        QObject::connect(btnAdv, &QPushButton::clicked, this, [this]() { openStocksWindow(3); });
    }

    // Layout the sidebar and modules side-by-side to avoid overlap
    {
        auto mainLayout = new QHBoxLayout(ui->mainprogram);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
        // Respect intended sidebar width constraints and prevent layout from squashing it
        ui->sidebar->setMinimumWidth(200);
        ui->sidebar->setMaximumWidth(220);
        ui->sidebar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        mainLayout->addWidget(ui->sidebar);
        // Ensure button text is never elided: compute required width per button via style and set minimums
        // Wrap the modules in a content area that applies offsets (drop + right shift)
        auto* contentArea = new QWidget(ui->mainprogram);
        auto* contentLayout = new QVBoxLayout(contentArea);
        // Keep margins simple and predictable; no need for DPI-based mm conversion here.
        contentLayout->setContentsMargins(16, 8, 0, 0);
        contentLayout->setSpacing(8); // small gap between user info and content

        // Place the user info bar above modules within the content area so it's layout-managed
        if (ui->userInfoContainer) {
            ui->userInfoContainer->setProperty("role", "panel");
            ui->userInfoContainer->setMinimumHeight(56);
            ui->userInfoContainer->setMaximumHeight(56);
            ui->userInfoContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            contentLayout->addWidget(ui->userInfoContainer);
        }

        // Modules sit below the user info bar
        contentLayout->addWidget(ui->modules);
        mainLayout->addWidget(contentArea);
    }

    // Sidebar buttons: make checkable for active state highlight
    if (ui->sidebar) {
        ui->btnmod1->setCheckable(true);
        ui->btnmod2->setCheckable(true);
        ui->btnmod3->setCheckable(true);
        ui->btnmod4->setCheckable(true);
        ui->btnmod5->setCheckable(true);
        ui->btnmod6->setCheckable(true);
        setActiveModuleButton(0);
    }

    // Initialize the personnel statistics chart
    setupPersonnelChart();
    setupStatHuilePage();
    setupAdvancedHuilePage();
    // Initialize the personnel table with action buttons
    setupPersonnelTable();
    // Add sidebar toggle button (hamburger) and interaction hooks
    setupInteractiveHooks();
    connect(ui->btnStatHuile, &QToolButton::clicked, this, &MainWindow::on_btnStatHuile_clicked);
    setupModule5();
    // Palette de couleur — QLineEdit ne supporte pas clicked(), connexion manuelle
    connect(ui->codecouleurLineEdit, &QLineEdit::selectionChanged, this, [this]() {
        // ne rien faire ici
    });
    ui->codecouleurLineEdit->installEventFilter(this);

    // Citernes module: ouvre la tache integree dans une fenetre dediee
    QObject::connect(ui->AjoutCiterne, &QToolButton::clicked, this, [this]() { openCiternesWindow(0); });
    QObject::connect(ui->ConsulterCiterne, &QToolButton::clicked, this, [this]() { openCiternesWindow(1); });
    QObject::connect(ui->StatistiqueCiterne, &QToolButton::clicked, this, [this]() { openCiternesWindow(2); });
    QObject::connect(ui->MetierAvanceCiterne, &QToolButton::clicked, this, [this]() { openCiternesWindow(3); });
    if (ui->exporterListeCiterne) {
        QObject::connect(ui->exporterListeCiterne, &QPushButton::clicked, this, [this]() { openCiternesWindow(1); });
    }
}

void MainWindow::openCiternesWindow(int pageIndex)
{
    if (!m_citernesWindow) {
        m_citernesWindow = new Citernes(this);
        m_citernesWindow->setAttribute(Qt::WA_DeleteOnClose, false);
        m_citernesWindow->setWindowTitle("Gestion des citernes");
    }

    if (pageIndex >= 0) {
        if (auto* stack = m_citernesWindow->findChild<QStackedWidget*>(QStringLiteral("metiersCiternes"))) {
            int safeIndex = std::clamp(pageIndex, 0, stack->count() - 1);
            stack->setCurrentIndex(safeIndex);
        }
    }

    m_citernesWindow->show();
    m_citernesWindow->raise();
    m_citernesWindow->activateWindow();
}

QByteArray MainWindow::encodeFaceFromFile(const QString& imagePath)
{
    return m_faceService ? m_faceService->encodeFaceFromFile(imagePath) : QByteArray{};
}

void MainWindow::loadFaceEmbeddings()
{
    if (m_faceService) m_faceService->loadFaceEmbeddings();
}

int MainWindow::matchFaceEmbedding(const QByteArray& embeddingBlob)
{
    return m_faceService ? m_faceService->matchFaceEmbeddingBlob(embeddingBlob) : -1;
}

void MainWindow::on_toolButton_clicked()
{
    // Slot required by Qt auto-connect (on_<objectName>_clicked).
    // If this button is not used anymore, remove/rename it in `mainwindow.ui`.
}

MainWindow::~MainWindow()
{
    if (machineSerial && machineSerial->isOpen())
        machineSerial->close();
    delete ui;
}

// ════════════════════════════════════════════════════════════════════════════
//  LOGIN
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::on_loginbtn_clicked()
{
    ui->MainStacked->setCurrentIndex(1);
}

// ════════════════════════════════════════════════════════════════════════════
//  PERSONNEL NAV
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::on_btnAjouterEmp_clicked()
{
    if (ui->modules->currentIndex() != 0)
        crossFadeToIndex(ui->modules, 0);
    crossFadeToIndex(ui->metierspersonnel, 0);
    validateEmployeeForm(false);
}

void MainWindow::on_ajouterEmpBtn_clicked()
{
    // ── Read form fields ────────────────────────────────────────────────────
    QString nom    = ui->nomLineEdit->text().trimmed();
    QString prenom = ui->prNomLineEdit->text().trimmed();
    QString email  = ui->emailLineEdit->text().trimmed();
    QString role   = ui->roleComboBox->currentText().trimmed();
    QString mdp    = ui->mdpLineEdit->text();

    if (role.isEmpty())
        role = "Technicien";

    // ── Check if we are editing an existing employee ────────────────────────
    QVariant editingIdVar = ui->ajouterEmpBtn->property("editingId");
    bool isEditing = editingIdVar.isValid() && editingIdVar.toInt() > 0;
    int editingId  = isEditing ? editingIdVar.toInt() : 0;

    // On explicit submit, mark all fields as touched so full feedback is shown.
    ui->nomLineEdit->setProperty("touched", true);
    ui->prNomLineEdit->setProperty("touched", true);
    ui->emailLineEdit->setProperty("touched", true);
    ui->roleComboBox->setProperty("touched", true);
    ui->mdpLineEdit->setProperty("touched", true);

    // ── Input validation with live green/red feedback ──────────────────────
    if (!validateEmployeeForm(true)) {
        QMessageBox::warning(this, tr("Validation"),
                             tr("Veuillez corriger les champs en rouge avant de continuer."));
        return;
    }

    if (isEditing) {
        // ── UPDATE mode ─────────────────────────────────────────────────────
        // Priority: live webcam capture > photo-file encoding > keep existing in DB
        QByteArray faceBlob = m_capturedFaceBlob;
        if (faceBlob.isEmpty() && !ui->photoPathLineEdit->text().isEmpty())
            faceBlob = encodeFaceFromFile(ui->photoPathLineEdit->text());

        Employe emp(editingId, nom, prenom, email, role, mdp, QDate(),
                    m_selectedPhoto, QByteArray(), faceBlob);
        if (!emp.modifier()) {
            QMessageBox::critical(this, tr("Erreur de modification"),
                tr("Impossible de modifier l'employé :\n%1").arg(emp.lastError().text()));
            return;
        }
        QMessageBox::information(this, tr("Succès"),
            tr("L'employé (ID : %1) a été modifié avec succès.").arg(editingId));

        // Reset button to Add mode
        ui->ajouterEmpBtn->setProperty("editingId", QVariant());
        ui->ajouterEmpBtn->setText(tr("Ajouter"));

    } else {
        // ── INSERT mode ─────────────────────────────────────────────────────
        // Priority: live webcam capture > photo-file encoding
        QByteArray faceBlob = m_capturedFaceBlob;
        if (faceBlob.isEmpty() && !ui->photoPathLineEdit->text().isEmpty())
            faceBlob = encodeFaceFromFile(ui->photoPathLineEdit->text());

        Employe emp(0, nom, prenom, email, role, mdp, QDate(),
                    m_selectedPhoto, QByteArray(), faceBlob);
        if (!emp.ajouter()) {
            QMessageBox::critical(this, tr("Erreur d'ajout"),
                tr("Impossible d'ajouter l'employé :\n%1").arg(emp.lastError().text()));
            return;
        }
        QMessageBox::information(this, tr("Succès"),
            tr("L'employé a été ajouté avec succès (ID : %1).").arg(emp.getIdEmp()));
    }

    // ── Clear the form ──────────────────────────────────────────────────────
    ui->nomLineEdit->clear();
    ui->prNomLineEdit->clear();
    ui->emailLineEdit->clear();
    ui->roleComboBox->setCurrentIndex(0);
    ui->mdpLineEdit->clear();
    ui->photoPathLineEdit->clear();
    m_selectedPhoto.clear();
    m_capturedFaceBlob.clear();
    ui->faceStatusLabel->setText(tr("Aucun visage capturé"));
    ui->faceStatusLabel->setStyleSheet("");
    validateEmployeeForm(false);
}

void MainWindow::on_parcourirPhotoBtn_clicked()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        tr("Choisir une photo"),
        QString(),
        tr("Images (*.png *.jpg *.jpeg *.bmp)"));

    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;

    m_selectedPhoto = file.readAll();
    file.close();

    ui->photoPathLineEdit->setText(path);

    // If no live capture done yet, try to encode the face from the chosen photo
    // so the status label gives instant feedback.
    if (m_capturedFaceBlob.isEmpty() && m_faceService && m_faceService->isAvailable()) {
        const QByteArray blob = encodeFaceFromFile(path);
        if (!blob.isEmpty()) {
            m_capturedFaceBlob = blob;
            ui->faceStatusLabel->setText(tr("✔ Visage détecté depuis la photo"));
            ui->faceStatusLabel->setStyleSheet("color: #2e7d32;");
        }
    }
}

void MainWindow::on_captureFaceBtn_clicked()
{
    if (!m_faceService || !m_faceService->isAvailable()) {
        QMessageBox::warning(this, tr("Indisponible"),
            tr("Les modèles de reconnaissance faciale n'ont pas pu être chargés.\n"
               "Vérifiez que les fichiers .onnx sont présents à côté de l'exécutable."));
        return;
    }

    FaceCaptureDialog dlg(m_faceService, this);
    const QByteArray blob = dlg.execAndGetEmbeddingBlob();
    if (blob.isEmpty()) return;

    m_capturedFaceBlob = blob;
    ui->faceStatusLabel->setText(tr("✔ Visage capturé avec succès"));
    ui->faceStatusLabel->setStyleSheet("color: #2e7d32; font-weight: bold;");
}

void MainWindow::on_btnConsulterEmp_clicked()
{
    if (ui->modules->currentIndex() != 0)
        crossFadeToIndex(ui->modules, 0);
    crossFadeToIndex(ui->metierspersonnel, 1);
}

void MainWindow::on_btnStatEmp_clicked()
{
    if (ui->modules->currentIndex() != 0)
        crossFadeToIndex(ui->modules, 0);
    crossFadeToIndex(ui->metierspersonnel, 2);
    loadEmployeeStats();
}

void MainWindow::on_btnAdvEmp_clicked()
{
    if (ui->modules->currentIndex() != 0)
        crossFadeToIndex(ui->modules, 0);
    crossFadeToIndex(ui->metierspersonnel, 3);
    populateAffCombos();
    loadAffectationTable();
    // Start on the table view
    ui->affStack->setCurrentIndex(1);
}

// ════════════════════════════════════════════════════════════════════════════
//  SIDEBAR MODULE NAVIGATION
// ════════════════════════════════════════════════════════════════════════════


void MainWindow::openStocksWindow(int pageIndex)
{
    if (!m_stocksWindow) {
        m_stocksWindow = new Stocks(this);
        m_stocksWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    }
    m_stocksWindow->navigateToSection(pageIndex);
    m_stocksWindow->show();
    m_stocksWindow->raise();
    m_stocksWindow->activateWindow();
}

void MainWindow::on_btnmod1_clicked()
{
    ui->MainStacked->setCurrentIndex(1);
    ui->modules->setCurrentIndex(0);
    setActiveModuleButton(0);
}

void MainWindow::on_btnmod2_clicked()
{
    ui->MainStacked->setCurrentIndex(1);
    ui->modules->setCurrentIndex(5);
    ui->metiersstocks->setCurrentIndex(3);
    setActiveModuleButton(5);
    openStocksWindow(1);
}


void MainWindow::on_btnAjouterstc_clicked()
{
    if (ui->modules->currentIndex() != 5)
        crossFadeToIndex(ui->modules, 5);
    crossFadeToIndex(ui->metiersstocks, 3);
    openStocksWindow(0);
}

void MainWindow::on_btnConsulterstc_clicked()
{
    if (ui->modules->currentIndex() != 5)
        crossFadeToIndex(ui->modules, 5);
    crossFadeToIndex(ui->metiersstocks, 3);
    openStocksWindow(1);
}

void MainWindow::on_btnStatstc_clicked()
{
    if (ui->modules->currentIndex() != 5)
        crossFadeToIndex(ui->modules, 5);
    crossFadeToIndex(ui->metiersstocks, 3);
    openStocksWindow(2);
}

void MainWindow::on_toolButton_5_clicked()
{
    if (ui->modules->currentIndex() != 5)
        crossFadeToIndex(ui->modules, 5);
    crossFadeToIndex(ui->metiersstocks, 3);
    openStocksWindow(3);
}

void MainWindow::on_btnmod3_clicked()
{
    ui->MainStacked->setCurrentIndex(1);
    ui->modules->setCurrentIndex(1);
    setActiveModuleButton(1);
    openCiternesWindow(1);
}

void MainWindow::on_btnmod4_clicked()
{
    ui->MainStacked->setCurrentIndex(1);
    ui->modules->setCurrentIndex(2);
    setActiveModuleButton(2);
}

void MainWindow::on_btnmod5_clicked()
{
    ui->MainStacked->setCurrentIndex(1);
    ui->modules->setCurrentIndex(3);
    setActiveModuleButton(3);
}

void MainWindow::on_btnmod6_clicked()
{
    ui->MainStacked->setCurrentIndex(1);
    ui->modules->setCurrentIndex(4);
    setActiveModuleButton(4);
}

// ════════════════════════════════════════════════════════════════════════════
//  HUILE MODULE — NAVBAR
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::on_btnAjouterQualite_clicked()
{
    // L'ID lot est gere automatiquement cote base (sequence + trigger)
    ui->idlotLineEdit->clear();
    ui->idlotLineEdit->setPlaceholderText("Automatique");
    ui->idlotLineEdit->setReadOnly(true);
    ui->idlotLineEdit->setStyleSheet("background-color: #f0f0f0; color: #888;");

    m_editingIdLot = -1;
    ui->ajouterHuileBtn->setText("Ajouter");
    ui->ajouterHuileBtn->disconnect();
    QObject::connect(ui->ajouterHuileBtn, &QPushButton::clicked,
                     this, &MainWindow::on_ajouterHuileBtn_clicked);

    ui->metiershuile->setCurrentIndex(0);
}
void MainWindow::on_btnConsulterQualite_clicked()
{
    ui->metiershuile->setCurrentIndex(1);

    QSqlQuery query;
    query.prepare("SELECT id_lot, responsable_controle, date_production, "
                  "quantite_produite, ph, acidite, amerture, statut_qualite, "
                  "id_stock, code_couleur "
                  "FROM QUALITE ORDER BY id_lot");

    ui->tableWidget_4->setRowCount(0);

    if (!query.exec()) {
        qDebug() << "Select error:" << query.lastError().text();
        return;
    }

    int row = 0;
    while (query.next()) {
        ui->tableWidget_4->insertRow(row);
        ui->tableWidget_4->setItem(row, 0, new QTableWidgetItem(query.value("id_lot").toString()));
        ui->tableWidget_4->setItem(row, 1, new QTableWidgetItem(query.value("responsable_controle").toString()));
        ui->tableWidget_4->setItem(row, 2, new QTableWidgetItem(query.value("date_production").toString()));
        ui->tableWidget_4->setItem(row, 3, new QTableWidgetItem(query.value("quantite_produite").toString()));
        ui->tableWidget_4->setItem(row, 4, new QTableWidgetItem(query.value("ph").toString()));
        ui->tableWidget_4->setItem(row, 5, new QTableWidgetItem(query.value("acidite").toString()));
        ui->tableWidget_4->setItem(row, 6, new QTableWidgetItem(query.value("amerture").toString()));
        ui->tableWidget_4->setItem(row, 7, new QTableWidgetItem(query.value("statut_qualite").toString()));
        ui->tableWidget_4->setItem(row, 8, new QTableWidgetItem(query.value("id_stock").toString()));  // ← NOUVEAU
        ui->tableWidget_4->setItem(row, 9, new QTableWidgetItem(query.value("code_couleur").toString()));
        addHuileActionButtonsToRow(row);
        row++;
    }
}
void MainWindow::on_amertureLineEdit_textChanged(const QString &text)
{
    bool ok;
    QString normalized = text;
    normalized.replace(',', '.');
    double val = normalized.toDouble(&ok);

    if (!ok || text.trimmed().isEmpty()) {
        ui->typeamerture->setText("");
        return;
    }

    if (val <= 0.10) {
        ui->typeamerture->setText("amertume faible");
        ui->typeamerture->setStyleSheet("color: green; font-weight: bold;");
    } else if (val >= 0.20 && val <= 0.30) {
        ui->typeamerture->setText("amertume moyenne");
        ui->typeamerture->setStyleSheet("color: orange; font-weight: bold;");
    } else if (val >= 0.35) {
        ui->typeamerture->setText("amertume forte");
        ui->typeamerture->setStyleSheet("color: red; font-weight: bold;");
    } else {
        ui->typeamerture->setText("");
    }
}
// ════════════════════════════════════════════════════════════════════════════
//  HUILE MODULE — INSERT AVEC CONTRÔLES DE SAISIE
// ════════════════════════════════════════════════════════════════════════════

namespace {
static double aciditeComboToValue(const QString &text)
{
    const QString v = text.trimmed().toLower();
    if (v == "acide") return 0.8;
    if (v == "neutre") return 0.5;
    if (v == "basique") return 0.2;

    bool ok = false;
    const double parsed = text.toDouble(&ok);
    return ok ? parsed : 0.0;
}
}

void MainWindow::on_ajouterHuileBtn_clicked()
{
    // --- Récupération des valeurs ---
    QString nomResp     = ui->nomresponsableLineEdit->text().trimmed();
    QString dateStr     = ui->dateprodDateEdit->date().toString("yyyy-MM-dd");
    QString statut      = ui->statutComboBox->currentText();
    QString quantite    = ui->quantiteLineEdit->text().trimmed();
    QString maxqt       = ui->maxqtLineEdit->text().trimmed();
    QString ph          = ui->phLineEdit->text().trimmed();
    QString idStock     = ui->idstockLineEdit->text().trimmed();  // ← NOUVEAU
    QString aciditeText = ui->aciditeComboBox->currentText();
    double aciditeVal   = aciditeComboToValue(aciditeText);
    QString codeCouleur = ui->codecouleurLineEdit->text().trimmed();
    QString amerture    = ui->amertureLineEdit->text().trimmed();

    // --- CONTROLE 1 : Champs vides ---
    if (nomResp.isEmpty() || quantite.isEmpty() ||
        maxqt.isEmpty() || ph.isEmpty() || idStock.isEmpty() ||  // ← idStock obligatoire
        codeCouleur.isEmpty() || amerture.isEmpty())
    {
        QMessageBox::warning(this, "Champs vides", "Veuillez remplir tous les champs.");
        return;
    }

    // --- Vérifier que l'ID Stock existe ---
    QSqlQuery checkStock;
    checkStock.prepare("SELECT id_stock FROM STOCK WHERE id_stock = ?");
    checkStock.addBindValue(idStock.toInt());
    if (!checkStock.exec() || !checkStock.next()) {
        QMessageBox::warning(this, "ID Stock invalide",
                             "L'ID Stock n'existe pas dans la table STOCK.\n"
                             "Veuillez entrer un ID valide.");
        ui->idstockLineEdit->setFocus();
        ui->idstockLineEdit->setStyleSheet("border: 2px solid red;");
        return;
    }
    ui->idstockLineEdit->setStyleSheet("");

    // --- CONTROLE 2 : Date de production <= aujourd'hui ---
    QDate dateProd = ui->dateprodDateEdit->date();
    if (dateProd > QDate::currentDate()) {
        QMessageBox::warning(this, "Date invalide", "La date ne peut pas être dans le futur.");
        ui->dateprodDateEdit->setFocus();
        return;
    }

    // --- CONTROLE 3 : Quantité positive ---
    bool okQt;
    double qtVal = quantite.toDouble(&okQt);
    if (!okQt || qtVal <= 0) {
        QMessageBox::warning(this, "Quantité invalide", "La quantité doit être un nombre positif.");
        ui->quantiteLineEdit->setFocus();
        return;
    }

    // --- CONTROLE 4 : Quantité <= Max quantite ---
    bool okMaxQt;
    double maxQtVal = maxqt.toDouble(&okMaxQt);
    if (!okMaxQt || maxQtVal <= 0) {
        QMessageBox::warning(this, "Max quantité invalide", "La max quantité doit être un nombre positif.");
        ui->maxqtLineEdit->setFocus();
        return;
    }
    if (qtVal > maxQtVal) {
        QMessageBox::warning(this, "Dépassement", QString("Quantité (%1 L) > capacité max (%2 L)").arg(qtVal).arg(maxQtVal));
        ui->quantiteLineEdit->setFocus();
        return;
    }

    // --- CONTROLE 5 : pH entre 0 et 14 ---
    bool okPh;
    double phVal = ph.toDouble(&okPh);
    if (!okPh || phVal < 0.0 || phVal > 14.0) {
        QMessageBox::warning(this, "pH invalide", "Le pH doit être entre 0 et 14.");
        ui->phLineEdit->setFocus();
        return;
    }

    // --- CONTROLE 6 : Amerture entre 0 et 1 ---
    bool okAm;
    double amVal = amerture.toDouble(&okAm);
    if (!okAm || amVal < 0.0 || amVal > 1.0) {
        QMessageBox::warning(this, "Amerture invalide", "L'amerture doit être entre 0 et 1.");
        ui->amertureLineEdit->setFocus();
        return;
    }

    // --- INSERT avec id_stock ---
    QSqlQuery query;
    query.prepare("INSERT INTO QUALITE "
                  "(date_production, quantite_produite, "
                  "ph, acidite, amerture, code_couleur, statut_qualite, "
                  "responsable_controle, max_quantite, id_stock) "
                  "VALUES "
                  "(TO_DATE(?, 'YYYY-MM-DD'), ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    query.addBindValue(dateStr);
    query.addBindValue(quantite.toDouble());
    query.addBindValue(ph.toDouble());
    query.addBindValue(aciditeVal);
    query.addBindValue(amVal);
    query.addBindValue(codeCouleur);
    query.addBindValue(statut);
    query.addBindValue(nomResp);
    query.addBindValue(maxqt.toDouble());
    query.addBindValue(idStock.toInt());  // ← NOUVEAU

    if (query.exec()) {
        QMessageBox::information(this, "Succès", "Huile ajoutée avec succès !");

        // Vider les champs
        ui->idlotLineEdit->clear();
        ui->idlotLineEdit->setPlaceholderText("Automatique");
        ui->nomresponsableLineEdit->clear();
        ui->quantiteLineEdit->clear();
        ui->phLineEdit->clear();
        ui->idstockLineEdit->clear();  // ← NOUVEAU
        ui->codecouleurLineEdit->clear();
        ui->amertureLineEdit->clear();
        ui->maxqtLineEdit->clear();
        ui->typeamerture->clear();
        ui->dateprodDateEdit->setDate(QDate(2000, 1, 1));
        ui->statutComboBox->setCurrentIndex(0);
        ui->aciditeComboBox->setCurrentIndex(0);

        on_btnConsulterQualite_clicked();
    } else {
        QMessageBox::critical(this, "Erreur", "Échec de l'ajout :\n" + query.lastError().text());
        qDebug() << "SQL Error:" << query.lastError().text();
    }
}
void MainWindow::setActiveModuleButton(int index)
{
    QPushButton* buttons[6] = { ui->btnmod1, ui->btnmod2, ui->btnmod3,
                               ui->btnmod4, ui->btnmod5, ui->btnmod6 };
    for (auto* b : buttons)
        b->setChecked(false);

    switch (index) {
    case 0: ui->btnmod1->setChecked(true); break;
    case 1: ui->btnmod3->setChecked(true); break;
    case 2: ui->btnmod4->setChecked(true); break;
    case 3: ui->btnmod5->setChecked(true); break;
    case 4: ui->btnmod6->setChecked(true); break;
    case 5: ui->btnmod2->setChecked(true); break;
    default: ui->btnmod1->setChecked(true); break;
    }
}

void MainWindow::crossFadeToIndex(QStackedWidget* stack, int newIndex)
{
    if (!stack || newIndex < 0 || newIndex >= stack->count())
        return;

    QWidget* current = stack->currentWidget();
    QWidget* next = stack->widget(newIndex);

    // If already on the target page, just make sure it's enabled and return
    if (!current || !next || current == next) {
        stack->setCurrentIndex(newIndex);
        stack->setEnabled(true);
        return;
    }

    // Create overlays sized to the stacked widget area
    const QRect area = stack->rect();
    auto* currentOverlay = new QLabel(stack);
    auto* nextOverlay    = new QLabel(stack);
    currentOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    nextOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    currentOverlay->setGeometry(area);
    nextOverlay->setGeometry(area);

    // Render snapshots of both pages (works even if 'next' is not visible)
    QPixmap currentShot(area.size());
    currentShot.fill(Qt::transparent);
    current->update();
    current->render(&currentShot, QPoint(), QRegion(), QWidget::DrawChildren);

    QPixmap nextShot(area.size());
    nextShot.fill(Qt::transparent);
    next->update();
    next->render(&nextShot, QPoint(), QRegion(), QWidget::DrawChildren);

    currentOverlay->setPixmap(currentShot);
    nextOverlay->setPixmap(nextShot);
    currentOverlay->raise();
    nextOverlay->raise();

    // Prepare opacity effects for parallel fade
    auto* currEff = new QGraphicsOpacityEffect(currentOverlay);
    auto* nextEff = new QGraphicsOpacityEffect(nextOverlay);
    currentOverlay->setGraphicsEffect(currEff);
    nextOverlay->setGraphicsEffect(nextEff);
    currEff->setOpacity(1.0);
    nextEff->setOpacity(0.0);

    // Show target page beneath overlays to avoid a blank gap
    stack->setCurrentIndex(newIndex);
}

void MainWindow::animateSidebarToggle(bool collapse)
{
    int from = ui->sidebar->width();
    int to = collapse ? 48 : 200;
    auto* anim = new QPropertyAnimation(ui->sidebar, "minimumWidth", this);
    anim->setDuration(220);
    anim->setStartValue(from);
    anim->setEndValue(to);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(anim, &QPropertyAnimation::valueChanged, this, [this](const QVariant&) {
        ui->sidebar->setMaximumWidth(ui->sidebar->minimumWidth());
    });
    QObject::connect(anim, &QPropertyAnimation::finished, this, [this, collapse]() {
        m_sidebarCollapsed = collapse;
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::setupInteractiveHooks()
{
    if (auto* headerLayout = ui->centralwidget->findChild<QHBoxLayout*>(QStringLiteral("logoandnamesidebar"))) {
        auto* toggleBtn = new QToolButton(ui->sidebar);
        toggleBtn->setAutoRaise(true);
    toggleBtn->setToolTip(tr("Réduire / étendre la barre latérale"));
        toggleBtn->setIcon(QIcon(QStringLiteral(":/img/menu.svg")));
        toggleBtn->setIconSize(QSize(18, 18));
        headerLayout->addStretch();
        headerLayout->addWidget(toggleBtn);
        QObject::connect(toggleBtn, &QToolButton::clicked, this, [this]() {
            animateSidebarToggle(!m_sidebarCollapsed);
        });
    }

    if (ui->lineEdit && ui->comboBox && ui->tableWidget_4) {
        QObject::connect(ui->lineEdit, &QLineEdit::textChanged, this, [this](const QString&) { filterPersonnelTable(); });
        QObject::connect(ui->comboBox, &QComboBox::currentTextChanged, this, [this](const QString&) { filterPersonnelTable(); });
    }

    // Live remaining-slots info for affectation form
    if (ui->affEmpCombo) {
        QObject::connect(ui->affEmpCombo, &QComboBox::currentIndexChanged, this,
            [this](int){ updateAffectationRemainingInfo(); });
    }
}

void MainWindow::setupEmployeeFormValidation()
{
    if (!ui->formLayout || !ui->formLayoutWidget)
        return;

    // Keep the form compact enough so the submit row is visible without excessive clipping.
    ui->formLayout->setVerticalSpacing(22);

    // Prevent placeholder/text clipping on dense DPI/font setups.
    const int fieldMinH = 34;
    ui->nomLineEdit->setMinimumHeight(fieldMinH);
    ui->prNomLineEdit->setMinimumHeight(fieldMinH);
    ui->emailLineEdit->setMinimumHeight(fieldMinH);
    ui->mdpLineEdit->setMinimumHeight(fieldMinH);
    ui->roleComboBox->setMinimumHeight(fieldMinH);
    ui->dateDEmbaucheDateEdit->setMinimumHeight(fieldMinH);
    if (ui->ajouterEmpBtn) {
        ui->ajouterEmpBtn->setMinimumHeight(36);
        ui->ajouterEmpBtn->show();
        ui->ajouterEmpBtn->raise();
    }

    QWidget* validationHost = ui->module1 ? static_cast<QWidget*>(ui->module1)
                                          : (ui->ajoutpersonnel ? static_cast<QWidget*>(ui->ajoutpersonnel)
                                                                : ui->formLayoutWidget->parentWidget());
    if (!validationHost)
        validationHost = ui->formLayoutWidget;

    QLabel* feedback = validationHost->findChild<QLabel*>(QStringLiteral("employeeValidationLabel"));
    if (!feedback) {
        feedback = new QLabel(validationHost);
        feedback->setObjectName(QStringLiteral("employeeValidationLabel"));
        feedback->setWordWrap(true);
    }

    // Keep validation text in the right-side free space of the employee page.
    feedback->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    feedback->setTextFormat(Qt::PlainText);
    feedback->setMargin(0);
    feedback->setMinimumHeight(36);
    feedback->setMinimumWidth(160);
    feedback->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    if (validationHost && ui->formLayoutWidget) {
        const QPoint formTopLeft = ui->formLayoutWidget->mapTo(validationHost, QPoint(0, 0));
        const QRect formRect(formTopLeft, ui->formLayoutWidget->size());
        const int margin = 16;
    const int desiredRightW = 200;
        const int availableRightW = validationHost->width() - (formRect.right() + margin) - margin;

        if (availableRightW >= 120) {
            const int sideX = formRect.right() + margin;
            const int sideY = formRect.y() + 8;
            const int sideW = qMin(qMax(120, availableRightW), desiredRightW);
            const int sideH = qMax(120, validationHost->height() - sideY - margin);
            feedback->setGeometry(sideX, sideY, sideW, sideH);
        } else {
            // Not enough right-side room: place feedback below form to keep it fully visible.
            const int belowX = formRect.x();
            const int belowY = formRect.bottom() + 12;
            const int belowW = qMax(320, qMin(formRect.width(), validationHost->width() - (2 * margin)));
            const int belowH = qMax(96, validationHost->height() - belowY - margin);
            feedback->setGeometry(belowX, belowY, belowW, belowH);
        }
    }
    auto refreshFeedbackVisibility = [this]() {
        QWidget* host = ui->module1 ? static_cast<QWidget*>(ui->module1)
                                    : (ui->ajoutpersonnel ? static_cast<QWidget*>(ui->ajoutpersonnel)
                                                          : ui->formLayoutWidget->parentWidget());
        if (!host) return;
        QLabel* fb = host->findChild<QLabel*>(QStringLiteral("employeeValidationLabel"));
        if (!fb) return;

        const bool onPersonnelAddPage =
            ui->MainStacked && ui->MainStacked->currentIndex() == 1
            && ui->modules && ui->metierspersonnel
            && ui->modules->currentWidget() == ui->module1
            && ui->metierspersonnel->currentIndex() == 0;
        fb->setVisible(onPersonnelAddPage);
    };
    refreshFeedbackVisibility();

    feedback->setText(tr("Remplissez le formulaire pour vérifier la validité des champs."));
    feedback->setStyleSheet(QStringLiteral("color: #546e7a;"));

    // The form container is absolute-positioned in the UI; keep enough room for all rows.
    if (QLayout* fl = ui->formLayoutWidget->layout()) {
        fl->activate();
        const QSize needed = fl->sizeHint() + QSize(24, 24);
        if (needed.height() > ui->formLayoutWidget->minimumHeight()) {
            ui->formLayoutWidget->setMinimumHeight(needed.height());
        }
        if (needed.width() > ui->formLayoutWidget->minimumWidth()) {
            ui->formLayoutWidget->setMinimumWidth(needed.width());
        }
        ui->formLayoutWidget->resize(
            qMax(ui->formLayoutWidget->width(), needed.width()),
            qMax(ui->formLayoutWidget->height(), needed.height()));

        // Also refresh parent page minimum size so the wrapping QScrollArea can actually scroll to it.
        if (QWidget* page = ui->formLayoutWidget->parentWidget()) {
            QRect bounds;
            const auto children = page->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
            for (QWidget* child : children) {
                if (!child) continue;
                bounds = bounds.united(child->geometry());
            }
            if (!bounds.isNull()) {
                const QSize pageMin(bounds.right() + 1 + 24, bounds.bottom() + 1 + 32);
                page->setMinimumSize(pageMin);
                page->resize(qMax(page->width(), pageMin.width()), qMax(page->height(), pageMin.height()));
            }
        }
    }

    // Touched-state starts false; colors appear only after focus/interaction.
    ui->nomLineEdit->setProperty("touched", false);
    ui->prNomLineEdit->setProperty("touched", false);
    ui->emailLineEdit->setProperty("touched", false);
    ui->roleComboBox->setProperty("touched", false);
    ui->mdpLineEdit->setProperty("touched", false);

    ui->nomLineEdit->installEventFilter(this);
    ui->prNomLineEdit->installEventFilter(this);
    ui->emailLineEdit->installEventFilter(this);
    ui->roleComboBox->installEventFilter(this);
    ui->mdpLineEdit->installEventFilter(this);

    auto liveValidate = [this]() { validateEmployeeForm(true); };
    QObject::connect(ui->nomLineEdit, &QLineEdit::textChanged, this, [liveValidate](const QString&) { liveValidate(); });
    QObject::connect(ui->prNomLineEdit, &QLineEdit::textChanged, this, [liveValidate](const QString&) { liveValidate(); });
    QObject::connect(ui->emailLineEdit, &QLineEdit::textChanged, this, [liveValidate](const QString&) { liveValidate(); });
    QObject::connect(ui->mdpLineEdit, &QLineEdit::textChanged, this, [liveValidate](const QString&) { liveValidate(); });
    QObject::connect(ui->roleComboBox, &QComboBox::currentTextChanged, this, [liveValidate](const QString&) { liveValidate(); });

    // Keep helper text visible only on Personnel > Ajouter, even when navigating via toolbar/sidebar.
    if (ui->modules) {
        QObject::connect(ui->modules, &QStackedWidget::currentChanged, this,
            [refreshFeedbackVisibility](int) { refreshFeedbackVisibility(); });
    }
    if (ui->metierspersonnel) {
        QObject::connect(ui->metierspersonnel, &QStackedWidget::currentChanged, this,
            [refreshFeedbackVisibility](int) { refreshFeedbackVisibility(); });
    }
    if (ui->MainStacked) {
        QObject::connect(ui->MainStacked, &QStackedWidget::currentChanged, this,
            [refreshFeedbackVisibility](int) { refreshFeedbackVisibility(); });
    }

    validateEmployeeForm(false);
}

bool MainWindow::validateEmployeeForm(bool showFeedbackText)
{
    const QString nom    = ui->nomLineEdit->text().trimmed();
    const QString prenom = ui->prNomLineEdit->text().trimmed();
    const QString email  = ui->emailLineEdit->text().trimmed();
    const QString role   = ui->roleComboBox->currentText().trimmed();
    const QString mdp    = ui->mdpLineEdit->text();

    const QVariant editingIdVar = ui->ajouterEmpBtn->property("editingId");
    const bool isEditing = editingIdVar.isValid() && editingIdVar.toInt() > 0;

    static const QRegularExpression nameRegex(QStringLiteral("^[A-Za-zÀ-ÖØ-öø-ÿ'\\- ]{2,}$"));
    static const QRegularExpression emailRegex(
        QStringLiteral("^[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,}$"),
        QRegularExpression::CaseInsensitiveOption);

    const bool nomValid = nameRegex.match(nom).hasMatch();
    const bool prenomValid = nameRegex.match(prenom).hasMatch();
    const bool emailValid = emailRegex.match(email).hasMatch();
    const bool roleValid = !role.isEmpty();
    const bool mdpValid = isEditing ? (mdp.isEmpty() || mdp.size() >= 6) : (mdp.size() >= 6);

    const bool nomTouched = ui->nomLineEdit->property("touched").toBool();
    const bool prenomTouched = ui->prNomLineEdit->property("touched").toBool();
    const bool emailTouched = ui->emailLineEdit->property("touched").toBool();
    const bool roleTouched = ui->roleComboBox->property("touched").toBool();
    const bool mdpTouched = ui->mdpLineEdit->property("touched").toBool();

    const auto applyState = [](QWidget* w, bool ok) {
        if (!w) return;
        w->setStyleSheet(ok
            ? QStringLiteral("background-color:#ffffff; color:#2b2d2f; border: 1px solid #2e7d32; border-radius: 8px; padding: 6px 10px;")
            : QStringLiteral("background-color:#ffffff; color:#2b2d2f; border: 1px solid #c62828; border-radius: 8px; padding: 6px 10px;"));
    };

    if (showFeedbackText) {
        if (nomTouched) applyState(ui->nomLineEdit, nomValid); else ui->nomLineEdit->setStyleSheet(QString());
        if (prenomTouched) applyState(ui->prNomLineEdit, prenomValid); else ui->prNomLineEdit->setStyleSheet(QString());
        if (emailTouched) applyState(ui->emailLineEdit, emailValid); else ui->emailLineEdit->setStyleSheet(QString());
        if (roleTouched) applyState(ui->roleComboBox, roleValid); else ui->roleComboBox->setStyleSheet(QString());
        if (mdpTouched) applyState(ui->mdpLineEdit, mdpValid); else ui->mdpLineEdit->setStyleSheet(QString());
    } else {
        ui->nomLineEdit->setStyleSheet(QString());
        ui->prNomLineEdit->setStyleSheet(QString());
        ui->emailLineEdit->setStyleSheet(QString());
        ui->roleComboBox->setStyleSheet(QString());
        ui->mdpLineEdit->setStyleSheet(QString());

        ui->nomLineEdit->setProperty("touched", false);
        ui->prNomLineEdit->setProperty("touched", false);
        ui->emailLineEdit->setProperty("touched", false);
        ui->roleComboBox->setProperty("touched", false);
        ui->mdpLineEdit->setProperty("touched", false);
    }

    QStringList errors;
    if (nomTouched && !nomValid)
        errors << tr("Nom invalide (min. 2 caractères, lettres uniquement).");
    if (prenomTouched && !prenomValid)
        errors << tr("Prénom invalide (min. 2 caractères, lettres uniquement).");
    if (emailTouched && !emailValid)
        errors << tr("Email invalide (format attendu: nom@domaine.com).");
    if (roleTouched && !roleValid)
        errors << tr("Rôle obligatoire.");
    if (mdpTouched && !mdpValid) {
        errors << (isEditing
            ? tr("Mot de passe: laissez vide ou utilisez au moins 6 caractères.")
            : tr("Mot de passe obligatoire (au moins 6 caractères)."));
    }

    QWidget* validationHost = ui->module1 ? static_cast<QWidget*>(ui->module1)
                                          : (ui->ajoutpersonnel ? static_cast<QWidget*>(ui->ajoutpersonnel)
                                                                : ui->formLayoutWidget->parentWidget());
    QLabel* feedback = validationHost
        ? validationHost->findChild<QLabel*>(QStringLiteral("employeeValidationLabel"))
        : nullptr;
    if (feedback) {
        const bool onPersonnelAddPage =
            ui->modules && ui->metierspersonnel
            && ui->modules->currentWidget() == ui->module1
            && ui->metierspersonnel->currentIndex() == 0;
        if (!onPersonnelAddPage) {
            feedback->hide();
            return errors.isEmpty();
        }
        feedback->show();

        if (validationHost && ui->formLayoutWidget) {
            const QPoint formTopLeft = ui->formLayoutWidget->mapTo(validationHost, QPoint(0, 0));
            const QRect formRect(formTopLeft, ui->formLayoutWidget->size());
            const int margin = 16;
            const int desiredRightW = 200;
            const int availableRightW = validationHost->width() - (formRect.right() + margin) - margin;

            if (availableRightW >= 120) {
                const int sideX = formRect.right() + margin;
                const int sideY = formRect.y() + 8;
                const int sideW = qMin(qMax(120, availableRightW), desiredRightW);
                const int sideH = qMax(120, validationHost->height() - sideY - margin);
                feedback->setGeometry(sideX, sideY, sideW, sideH);
            } else {
                const int belowX = formRect.x();
                const int belowY = formRect.bottom() + 12;
                const int belowW = qMax(320, qMin(formRect.width(), validationHost->width() - (2 * margin)));
                const int belowH = qMax(96, validationHost->height() - belowY - margin);
                feedback->setGeometry(belowX, belowY, belowW, belowH);
            }
        }

        if (!showFeedbackText) {
            feedback->setText(tr("Remplissez le formulaire pour vérifier la validité des champs."));
            feedback->setStyleSheet(QStringLiteral("color: #546e7a;"));
        } else if (!nomTouched && !prenomTouched && !emailTouched && !roleTouched && !mdpTouched) {
            feedback->setText(tr("Cliquez sur un champ pour commencer la validation."));
            feedback->setStyleSheet(QStringLiteral("color: #546e7a;"));
        } else if (errors.isEmpty()) {
            feedback->setText(tr("✔ Formulaire valide. Vous pouvez enregistrer cet employé."));
            feedback->setStyleSheet(QStringLiteral("color: #2e7d32; font-weight: 600;"));
        } else {
            feedback->setText(QStringLiteral("✘ ") + errors.join(QStringLiteral("\n✘ ")));
            feedback->setStyleSheet(QStringLiteral("color: #c62828; font-weight: 600;"));
        }
    }

    return errors.isEmpty();
}

void MainWindow::filterPersonnelTable()
{
    if (!ui->tableWidget_4 || !ui->comboBox) return;
    QString needle = ui->lineEdit ? ui->lineEdit->text().trimmed() : QString();
    QString mode = ui->comboBox->currentText();
    int col = 0;
    if (mode.compare(QStringLiteral("Name"), Qt::CaseInsensitive) == 0 ||
        mode.compare(QStringLiteral("Nom"), Qt::CaseInsensitive) == 0) col = 1;
    else if (mode.compare(QStringLiteral("Status"), Qt::CaseInsensitive) == 0) col = 2;
    else col = 0;

    for (int r = 0; r < ui->tableWidget_4->rowCount(); ++r) {
        auto* item = ui->tableWidget_4->item(r, col);
        bool match = needle.isEmpty() || (item && item->text().contains(needle, Qt::CaseInsensitive));
        ui->tableWidget_4->setRowHidden(r, !match);
    }
}

void MainWindow::setupPersonnelChart()
{
    QPieSeries *series = new QPieSeries();
    series->append("Actifs", 42);
    series->append("En congé", 8);
    series->append("Suspendus", 3);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Répartition des employés");
    chart->legend()->setAlignment(Qt::AlignRight);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(ui->chartStatusContainer->layout());
    if (!layout) {
        layout = new QVBoxLayout(ui->chartStatusContainer);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(12);
    }
    layout->addWidget(chartView);

    QBarSet *setActifs = new QBarSet("Actifs");
    QBarSet *setConge  = new QBarSet("En congé");
    QBarSet *setSusp   = new QBarSet("Suspendus");
    *setActifs << 18 << 15 << 9;
    *setConge  << 3  << 2  << 3;
    *setSusp   << 1  << 1  << 1;

    QBarSeries *barSeries = new QBarSeries();
    barSeries->append(setActifs);
    barSeries->append(setConge);
    barSeries->append(setSusp);

    QChart *barChart = new QChart();
    barChart->addSeries(barSeries);
    barChart->setTitle("Statut par département");
    barChart->setAnimationOptions(QChart::AllAnimations);
    QStringList categories;
    categories << "Production" << "Qualité" << "Support";
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, 20);
    axisY->setTitleText("Employés");
    barChart->addAxis(axisX, Qt::AlignBottom);
    barChart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisX);
    barSeries->attachAxis(axisY);
    barChart->legend()->setVisible(true);
    barChart->legend()->setAlignment(Qt::AlignBottom);

    QChartView *barView = new QChartView(barChart);
    barView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(barView);

    QLineSeries *lineSeries = new QLineSeries();
    lineSeries->setName("Effectif total");
    lineSeries->append(0, 38);
    lineSeries->append(1, 39);
    lineSeries->append(2, 40);
    lineSeries->append(3, 41);
    lineSeries->append(4, 43);
    lineSeries->append(5, 45);

    QChart *lineChart = new QChart();
    lineChart->addSeries(lineSeries);
    lineChart->setTitle("Tendance de l'effectif (semestre)");
    lineChart->setAnimationOptions(QChart::AllAnimations);

    QValueAxis *xAxis = new QValueAxis();
    xAxis->setLabelFormat("%d");
    xAxis->setTitleText("Mois");
    xAxis->setTickCount(7);
    xAxis->setRange(0, 5);
    QValueAxis *yAxis = new QValueAxis();
    yAxis->setTitleText("Employés");
    yAxis->setRange(35, 50);
    lineChart->addAxis(xAxis, Qt::AlignBottom);
    lineChart->addAxis(yAxis, Qt::AlignLeft);
    lineSeries->attachAxis(xAxis);
    lineSeries->attachAxis(yAxis);
    lineChart->legend()->setVisible(false);

    QChartView *lineView = new QChartView(lineChart);
    lineView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(lineView);
}

void MainWindow::loadEmployeeTable()
{
    QTableWidget* table = ui->tableEmp;
    if (!table) return;

    // ── Query the DB ────────────────────────────────────────────────────────
    Employe emp;
    QSqlQueryModel* model = emp.afficher();

    // ── Set up columns (ID / Nom / Prénom / Email / Rôle / Actions) ─────────
    const int dataCols = 5; // id, nom, prenom, email, role
    table->setColumnCount(dataCols + 1);
    table->setHorizontalHeaderLabels({
        "ID", "Nom", QStringLiteral("Pr\u00e9nom"), "Email",
        QStringLiteral("R\u00f4le"), "Actions"
    });

    // ── Fill rows ───────────────────────────────────────────────────────────
    int rowCount = model->rowCount();
    table->setRowCount(rowCount);

    for (int r = 0; r < rowCount; ++r) {
        for (int c = 0; c < dataCols; ++c) {
            QString text = model->data(model->index(r, c)).toString();
            QTableWidgetItem* item = new QTableWidgetItem(text);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            table->setItem(r, c, item);
        }
        addActionButtonsToRow(table, r);
    }

    delete model;

    // ── Column sizing ───────────────────────────────────────────────────────
    if (table->horizontalHeader()) {
        table->horizontalHeader()->setStretchLastSection(false);
        // ID column: compact
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        // Data columns: stretch
        for (int c = 1; c < dataCols; ++c)
            table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
        // Actions column: fixed
        table->horizontalHeader()->setSectionResizeMode(dataCols, QHeaderView::Fixed);
    table->setColumnWidth(dataCols, 96);
    }
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    if (table->verticalHeader())
    table->verticalHeader()->setDefaultSectionSize(34);
}

void MainWindow::setupPersonnelTable()
{
    if (!ui->tableWidget_4)
        return;

    QTableWidget* table = ui->tableWidget_4;

    if (table->columnCount() > 0) {
        int last = table->columnCount() - 1;
        auto* hLast = table->horizontalHeaderItem(last);
        if (hLast && hLast->text().trimmed().compare(QStringLiteral("Actions"), Qt::CaseInsensitive) == 0) {
            for (int r = 0; r < table->rowCount(); ++r) {
                if (!table->cellWidget(r, last)) addActionButtonsToRow(r);
            }
            return;
        }
    }

    int actionsCol = table->columnCount();
    table->insertColumn(actionsCol);

    QStringList headers;
    for (int c = 0; c < table->columnCount(); ++c) {
        if (c == actionsCol) {
            headers << QStringLiteral("Actions");
        } else {
            auto* item = table->horizontalHeaderItem(c);
            headers << (item ? item->text() : QString("Col %1").arg(c));
        }
    }
    table->setHorizontalHeaderLabels(headers);

    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    if (table->verticalHeader())
        table->verticalHeader()->setDefaultSectionSize(32);

    for (int r = 0; r < table->rowCount(); ++r)
        addActionButtonsToRow(r);

    if (table->horizontalHeader()) {
        table->horizontalHeader()->setStretchLastSection(false);
        int last = table->columnCount() - 1;
        for (int c = 0; c < last; ++c)
            table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(last, QHeaderView::ResizeToContents);
        table->setColumnWidth(last, 96);
    }
}

void MainWindow::addActionButtonsToRow(int row)
{
    if (!ui->tableWidget_4) return;
    auto* table = ui->tableWidget_4;

    QWidget* container = new QWidget(table);
    auto* h = new QHBoxLayout(container);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(6);
    h->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    container->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    QPushButton* btnModify = new QPushButton(QString(), container);
    btnModify->setProperty("type", "warning");
    btnModify->setProperty("size", "small");
    btnModify->setObjectName("modifyBtn");
    btnModify->setFocusPolicy(Qt::NoFocus);
    btnModify->setToolTip(tr("Modifier"));
    btnModify->setIcon(QIcon(QStringLiteral(":/img/edit.svg")));
    btnModify->setIconSize(QSize(16, 16));
    btnModify->setFixedSize(28, 24);

    QPushButton* btnDelete = new QPushButton(QString(), container);
    btnDelete->setProperty("type", "danger");
    btnDelete->setProperty("size", "small");
    btnDelete->setObjectName("deleteBtn");
    btnDelete->setFocusPolicy(Qt::NoFocus);
    btnDelete->setToolTip(tr("Supprimer"));
    btnDelete->setIcon(QIcon(QStringLiteral(":/img/delete.svg")));
    btnDelete->setIconSize(QSize(16, 16));
    btnDelete->setFixedSize(28, 24);

    h->addWidget(btnModify);
    h->addWidget(btnDelete);
    container->setLayout(h);

    table->setCellWidget(row, table->columnCount() - 1, container);
    table->setRowHeight(row, 34);

    connect(btnModify, &QPushButton::clicked, this, [this]() {
        int row = findRowForButton(sender());
        if (row < 0) return;
        ui->tableWidget_4->selectRow(row);
        QMessageBox::information(this, tr("Modifier"), tr("Modifier la ligne %1").arg(row + 1));
    });

    connect(btnDelete, &QPushButton::clicked, this, [this]() {
        int row = findRowForButton(sender());
        if (row < 0) return;
        auto reply = QMessageBox::question(this, tr("Supprimer"), tr("Supprimer la ligne %1 ?").arg(row + 1));
        if (reply == QMessageBox::Yes)
            ui->tableWidget_4->removeRow(row);
    });
}

int MainWindow::findRowForButton(QObject* button) const
{
    if (!button || !ui->tableWidget_4) return -1;
    auto* table = ui->tableWidget_4;

    for (int r = 0; r < table->rowCount(); ++r) {
        QWidget* cell = table->cellWidget(r, table->columnCount() - 1);
        if (!cell) continue;
        auto* mod = cell->findChild<QPushButton*>("modifyBtn");
        auto* del = cell->findChild<QPushButton*>("deleteBtn");
        if (mod == button || del == button)
            return r;
    }
    return -1;
}

void MainWindow::on_faceBtn_clicked()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Reconnaissance Faciale"));
    dlg.resize(680, 560);

    auto* root    = new QVBoxLayout(&dlg);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    auto* camLbl = new QLabel(&dlg);
    camLbl->setAlignment(Qt::AlignCenter);
    camLbl->setMinimumSize(640, 480);
    camLbl->setStyleSheet("background:#000;");
    root->addWidget(camLbl, 1);

    auto* preview = new QWidget(&dlg);
    preview->setObjectName("facePreview");
    preview->setMinimumSize(560, 300);
    root->addWidget(preview, 1);

    auto* footer = new QHBoxLayout();
    auto* status = new QLabel(tr("Camera idle"), &dlg);
    status->setProperty("type", "subheading");
    auto* spacer = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto* btnStart = new QPushButton(tr("Start"), &dlg);
    auto* btnClose = new QPushButton(tr("Close"), &dlg);
    btnStart->setProperty("type", "primary");

    // ── Open webcam ─────────────────────────────────────────────────────────
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        QMessageBox::critical(this, tr("Erreur"),
            tr("Impossible d'ouvrir la webcam (index 0)."));
        return;
    }

    bool running = false;
    QObject::connect(btnStart, &QPushButton::clicked, &dlg, [&, status, btnStart]() mutable {
        running = !running;
        status->setText(running ? tr("Camera running…") : tr("Camera idle"));
        btnStart->setText(running ? tr("Stop") : tr("Start"));
    });
    QObject::connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::reject);

    dlg.exec();

    // Ensure resources freed even on Cancel
    frameTimer->stop();
    if (cap.isOpened()) cap.release();

    // ── Handle successful match ──────────────────────────────────────────────
    if (matchedId > 0) {
        m_loggedInId = matchedId;

        QSqlQuery q;
        q.prepare("SELECT nom_emp, prenom_emp FROM EMPLOYE WHERE id_emp = :id");
        q.bindValue(":id", matchedId);
        if (q.exec() && q.next()) {
            QString fullName = q.value(0).toString() + " " + q.value(1).toString();
            if (ui->userNameLabel)
                ui->userNameLabel->setText(fullName);
        }
        ui->MainStacked->setCurrentIndex(1);
    }
}

// ── Face helper implementations ─────────────────────────────────────────────

QByteArray MainWindow::encodeFaceFromFile(const QString& imagePath)
{
    if (!m_faceDetector || !m_faceRecognizer) return {};

    cv::Mat img = cv::imread(imagePath.toStdString());
    if (img.empty()) {
        qWarning() << "[FaceRecog] Cannot read image:" << imagePath;
        return {};
    }

    m_faceDetector->setInputSize(img.size());
    cv::Mat faces;
    m_faceDetector->detect(img, faces);
    if (faces.rows == 0) {
        qWarning() << "[FaceRecog] No face detected in:" << imagePath;
        return {};
    }

    // Use the first (best-scoring) detected face
    cv::Mat aligned, embedding;
    m_faceRecognizer->alignCrop(img, faces.row(0), aligned);
    m_faceRecognizer->feature(aligned, embedding); // 1×128 float32

    QByteArray blob(reinterpret_cast<const char*>(embedding.data),
                    static_cast<int>(embedding.total() * sizeof(float)));
    qDebug() << "[FaceRecog] Encoded face blob size:" << blob.size() << "bytes";
    return blob;
}

void MainWindow::loadFaceEmbeddings()
{
    m_faceEmbeddings.clear();
    if (!m_faceRecognizer) return;

    QSqlQuery q;
    q.prepare("SELECT nom_emp, prenom_emp FROM EMPLOYE WHERE id_emp = :id");
    q.bindValue(":id", matchedId);
    if (q.exec() && q.next()) {
        const QString fullName = q.value(0).toString() + " " + q.value(1).toString();
        if (ui->userNameLabel)
            ui->userNameLabel->setText(fullName);
    }

    ui->MainStacked->setCurrentIndex(1);
}
// ════════════════════════════════════════════════════════════════════════════
//  HUILE — BOUTONS ACTION (séparé du module Personnel)
// ════════════════════════════════════════════════════════════════════════════

int MainWindow::findRowForHuileButton(QObject* button) const
{
    if (!button || !ui->tableWidget_4) return -1;
    auto* table = ui->tableWidget_4;
    for (int r = 0; r < table->rowCount(); ++r) {
        QWidget* cell = table->cellWidget(r, table->columnCount() - 1);
        if (!cell) continue;
        auto* mod = cell->findChild<QPushButton*>("huileModifyBtn");
        auto* del = cell->findChild<QPushButton*>("huileDeleteBtn");
        if (mod == button || del == button)
            return r;
    }
    return -1;
}

void MainWindow::addHuileActionButtonsToRow(int row)
{
    if (!ui->tableWidget_4) return;
    auto* table = ui->tableWidget_4;
    int actionsCol = table->columnCount() - 1;

    QWidget* container = new QWidget(table);
    auto* h = new QHBoxLayout(container);
    h->setContentsMargins(2, 2, 2, 2);
    h->setSpacing(4);
    h->setAlignment(Qt::AlignCenter);
    ui->idstockLineEdit->setText(table->item(row, 8)->text());
    // Bouton Modifier (orange)
    QPushButton* btnModify = new QPushButton(container);
    btnModify->setObjectName("huileModifyBtn");
    btnModify->setText(QString::fromUtf8("Modifier"));
    btnModify->setFixedSize(100, 30);
    btnModify->setToolTip("Modifier");
    btnModify->setStyleSheet(
        "QPushButton#huileModifyBtn { background-color: #e6a817; color: #ffffff; border-radius: 4px; font-size: 11px; font-weight: bold; padding: 0px 4px; }"
        "QPushButton#huileModifyBtn:hover { background-color: #c8911a; }"
        );

    // Bouton Supprimer (rouge)
    QPushButton* btnDelete = new QPushButton(container);
    btnDelete->setObjectName("huileDeleteBtn");
    btnDelete->setText(QString::fromUtf8("Supprimer"));
    btnDelete->setFixedSize(100, 30);
    btnDelete->setToolTip("Supprimer");
    btnDelete->setStyleSheet(
        "QPushButton#huileDeleteBtn { background-color: #e53935; color: #ffffff; border-radius: 4px; font-size: 11px; font-weight: bold; padding: 0px 4px; }"
        "QPushButton#huileDeleteBtn:hover { background-color: #b71c1c; }"
        );

    h->addWidget(btnModify);
    h->addWidget(btnDelete);
    container->setLayout(h);
    table->setCellWidget(row, actionsCol, container);
    table->setRowHeight(row, 50);

    // ── SUPPRIMER ───────────────────────────────────────────────
    connect(btnDelete, &QPushButton::clicked, this, [this, table]() {
        int row = findRowForHuileButton(sender());
        if (row < 0) return;

        QString idLot = table->item(row, 0)->text();

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Confirmation de suppression");
        msgBox.setText("Voulez-vous vraiment supprimer cette huile ?");
        msgBox.setIcon(QMessageBox::Warning);
        QPushButton* ouiBtn = msgBox.addButton("  Oui  ", QMessageBox::YesRole);
        QPushButton* nonBtn = msgBox.addButton("  Non  ", QMessageBox::NoRole);
        ouiBtn->setStyleSheet(
            "background-color: #e53935; color: white; padding: 5px 15px; border-radius: 4px;");
        nonBtn->setStyleSheet(
            "background-color: #9e9e9e; color: white; padding: 5px 15px; border-radius: 4px;");
        msgBox.exec();

        if (msgBox.clickedButton() == ouiBtn) {
            QSqlQuery query;
            query.prepare("DELETE FROM QUALITE WHERE id_lot = ?");
            query.addBindValue(idLot.toInt());
            if (query.exec()) {
                table->removeRow(row);
                QMessageBox::information(this, "Succès", "Suppression avec succès !");
            } else {
                QMessageBox::critical(this, "Erreur",
                                      "Échec de la suppression :\n" + query.lastError().text());
            }
        }
    });

    // ── MODIFIER ────────────────────────────────────────────────
    connect(btnModify, &QPushButton::clicked, this, [this, table]() {
        int row = findRowForHuileButton(sender());
        if (row < 0) return;

        m_editingIdLot = table->item(row, 0)->text().toInt();

        ui->idlotLineEdit->setText(table->item(row, 0)->text());
        ui->nomresponsableLineEdit->setText(table->item(row, 1)->text());

        QString dateStr = table->item(row, 2)->text();
        QDate date = QDate::fromString(dateStr.left(10), "yyyy-MM-dd");
        if (!date.isValid()) date = QDate(2000, 1, 1);
        ui->dateprodDateEdit->setDate(date);

        ui->quantiteLineEdit->setText(table->item(row, 3)->text());
        ui->phLineEdit->setText(table->item(row, 4)->text());

        QString acidite = table->item(row, 5)->text();
        int acidIdx = ui->aciditeComboBox->findText(acidite, Qt::MatchContains);
        if (acidIdx >= 0) ui->aciditeComboBox->setCurrentIndex(acidIdx);

        ui->amertureLineEdit->setText(table->item(row, 6)->text());

        QString statut = table->item(row, 7)->text();
        int statIdx = ui->statutComboBox->findText(statut, Qt::MatchContains);
        if (statIdx >= 0) ui->statutComboBox->setCurrentIndex(statIdx);

        ui->idstockLineEdit->setText(table->item(row, 8)->text());
        ui->codecouleurLineEdit->setText(table->item(row, 9)->text());

        // Récupère max_quantite depuis la base car non affiché dans le tableau
        QSqlQuery qMax;
        qMax.prepare("SELECT max_quantite FROM QUALITE WHERE id_lot = ?");
        qMax.addBindValue(m_editingIdLot);
        if (qMax.exec() && qMax.next())
            ui->maxqtLineEdit->setText(qMax.value("max_quantite").toString());
        else
            ui->maxqtLineEdit->clear();

        // Change Ajouter -> Sauvegarder
        ui->ajouterHuileBtn->setText("Sauvegarder");
        ui->ajouterHuileBtn->disconnect();
        connect(ui->ajouterHuileBtn, &QPushButton::clicked,
                this, &MainWindow::sauvegarderHuile);

        ui->metiershuile->setCurrentIndex(0);
    });
}
void MainWindow::sauvegarderHuile()
{
    QString nomResp     = ui->nomresponsableLineEdit->text().trimmed();
    QString dateStr     = ui->dateprodDateEdit->date().toString("yyyy-MM-dd");
    QString statut      = ui->statutComboBox->currentText();
    QString quantite    = ui->quantiteLineEdit->text().trimmed();
    QString maxqt       = ui->maxqtLineEdit->text().trimmed();
    QString ph          = ui->phLineEdit->text().trimmed();
    QString idStock     = ui->idstockLineEdit->text().trimmed();  // ← NOUVEAU
    QString aciditeText = ui->aciditeComboBox->currentText();
    double aciditeVal   = aciditeComboToValue(aciditeText);
    QString codeCouleur = ui->codecouleurLineEdit->text().trimmed();
    QString amerture    = ui->amertureLineEdit->text().trimmed();

    // Validation
    if (nomResp.isEmpty() || quantite.isEmpty() || maxqt.isEmpty() ||
        ph.isEmpty() || idStock.isEmpty() || codeCouleur.isEmpty() || amerture.isEmpty())
    {
        QMessageBox::warning(this, "Champs vides", "Veuillez remplir tous les champs.");
        return;
    }

    // Vérifier l'ID Stock
    QSqlQuery checkStock;
    checkStock.prepare("SELECT id_stock FROM STOCK WHERE id_stock = ?");
    checkStock.addBindValue(idStock.toInt());
    if (!checkStock.exec() || !checkStock.next()) {
        QMessageBox::warning(this, "ID Stock invalide", "Cet ID Stock n'existe pas.");
        return;
    }

    // UPDATE
    QSqlQuery query;
    query.prepare("UPDATE QUALITE SET "
                  "date_production = TO_DATE(?, 'YYYY-MM-DD'), "
                  "quantite_produite = ?, "
                  "ph = ?, "
                  "acidite = ?, "
                  "amerture = ?, "
                  "code_couleur = ?, "
                  "statut_qualite = ?, "
                  "responsable_controle = ?, "
                  "max_quantite = ?, "
                  "id_stock = ? "
                  "WHERE id_lot = ?");

    query.addBindValue(dateStr);
    query.addBindValue(quantite.toDouble());
    query.addBindValue(ph.toDouble());
    query.addBindValue(aciditeVal);
    query.addBindValue(amerture.toDouble());
    query.addBindValue(codeCouleur);
    query.addBindValue(statut);
    query.addBindValue(nomResp);
    query.addBindValue(maxqt.toDouble());
    query.addBindValue(idStock.toInt());  // ← NOUVEAU
    query.addBindValue(m_editingIdLot);

    if (query.exec()) {
        QMessageBox::information(this, "Succès", "Modification avec succès !");
        // ... reset des champs ...
        on_btnConsulterQualite_clicked();
    } else {
        QMessageBox::critical(this, "Erreur", "Échec de la modification :\n" + query.lastError().text());
    }
}
void MainWindow::on_pushButton_3_clicked()
{
    if (!ui->tableWidget_4 || ui->tableWidget_4->rowCount() == 0) {
        QMessageBox::warning(this, "Export", "Aucune donnée à exporter.");
        return;
    }

    // Demande à l'utilisateur où sauvegarder le fichier
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Exporter les données",
        "qualite_huile.csv",
        "Fichiers CSV (*.csv)"
        );

    if (filePath.isEmpty()) return; // L'utilisateur a annulé

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Erreur", "Impossible de créer le fichier.");
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Écriture des en-têtes (sans la colonne Actions)
    QStringList headers;
    int colCount = ui->tableWidget_4->columnCount() - 1; // -1 pour ignorer Actions
    for (int c = 0; c < colCount; c++) {
        auto* header = ui->tableWidget_4->horizontalHeaderItem(c);
        headers << (header ? header->text() : QString("Col%1").arg(c));
    }
    out << headers.join(";") << "\n";

    // Écriture des données
    for (int r = 0; r < ui->tableWidget_4->rowCount(); r++) {
        QStringList rowData;
        for (int c = 0; c < colCount; c++) {
            auto* item = ui->tableWidget_4->item(r, c);
            QString val = item ? item->text() : "";
            // Si la valeur contient un point-virgule, entourer de guillemets
            if (val.contains(";")) val = "\"" + val + "\"";
            rowData << val;
        }
        out << rowData.join(";") << "\n";
    }

    file.close();
    QMessageBox::information(this, "Succès",
                             "Export réussi !\nFichier sauvegardé :\n" + filePath);
}
void MainWindow::on_recherchelot_textChanged(const QString &text)
{
    if (!ui->tableWidget_4) return;

    QString needle = text.trimmed();
    QString critere = ui->comboBoxrecherche->currentText();

    // Mapping critere -> colonne du tableau
    // col 0=ID lot, col 1=Nom responsable, col 2=Date production
    int col = -1; // -1 = cherche dans toutes les colonnes
    if (critere == "ID")
        col = 0;
    else if (critere == "Nom resp")
        col = 1;
    else if (critere == "Date prod")
        col = 2;

    for (int r = 0; r < ui->tableWidget_4->rowCount(); ++r) {
        bool found = false;

        if (needle.isEmpty()) {
            found = true;
        } else if (col >= 0) {
            // Recherche dans la colonne specifique
            auto* item = ui->tableWidget_4->item(r, col);
            found = item && item->text().contains(needle, Qt::CaseInsensitive);
        } else {
            // Recherche dans toutes les colonnes
            int colCount = ui->tableWidget_4->columnCount() - 1;
            for (int c = 0; c < colCount; ++c) {
                auto* item = ui->tableWidget_4->item(r, c);
                if (item && item->text().contains(needle, Qt::CaseInsensitive)) {
                    found = true;
                    break;
                }
            }
        }

        ui->tableWidget_4->setRowHidden(r, !found);
    }
}

void MainWindow::on_comboBoxrecherche_currentTextChanged(const QString &)
{
    // Quand le critere change, relance la recherche avec le texte actuel
    on_recherchelot_textChanged(ui->recherchelot->text());
}
// ════════════════════════════════════════════════════════════════════════════
//  STATISTIQUES HUILE
// ════════════════════════════════════════════════════════════════════════════


void MainWindow::setupStatHuilePage()
{
    QWidget *statWidget = new QWidget();
    QVBoxLayout *pageLayout = new QVBoxLayout(statWidget);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea(statWidget);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *container = new QWidget();
    auto *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(26, 24, 26, 24);
    mainLayout->setSpacing(18);

    auto createCardFrame = [](const QString &bgColor, const QString &borderColor) {
        auto *frame = new QFrame();
        frame->setStyleSheet(QString(
            "QFrame { background:%1; border:1px solid %2; border-radius:18px; }"
        ).arg(bgColor, borderColor));
        return frame;
    };

    auto *hero = createCardFrame("#f8fbf2", "#dbe6c3");
    hero->setMinimumHeight(120);
    auto *heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(24, 20, 24, 20);
    heroLayout->setSpacing(10);

    auto *heroTitle = new QLabel("Tableau de bord qualité de l'huile", hero);
    heroTitle->setStyleSheet("font-size:28px; font-weight:800; color:#253418; border:none; background:transparent;");
    heroTitle->setWordWrap(true);
    heroLayout->addWidget(heroTitle);

    auto *heroSub = new QLabel("Suivi visuel de la conformité, des quantités produites, du pH, de l'acidité et de la température de production des lots.", hero);
    heroSub->setStyleSheet("font-size:14px; color:#5f6f48; border:none; background:transparent;");
    heroSub->setWordWrap(true);
    heroLayout->addWidget(heroSub);
    mainLayout->addWidget(hero);

    auto createStatCard = [](const QString &title, const QString &badgeText, QLabel **valueLabel, const QString &accent, const QString &bg, const QString &subText) {
        auto *card = new QFrame();
        card->setMinimumSize(270, 140);
        card->setStyleSheet(QString(
            "QFrame { background:%1; border:1px solid #d7e2c5; border-radius:18px; }"
        ).arg(bg));
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(18, 16, 18, 16);
        layout->setSpacing(8);

        auto *topRow = new QHBoxLayout();
        topRow->setSpacing(8);

        auto *titleLabel = new QLabel(title, card);
        titleLabel->setWordWrap(true);
        titleLabel->setStyleSheet("font-size:15px; font-weight:700; color:#243418; border:none; background:transparent;");
        topRow->addWidget(titleLabel, 1);

        auto *badge = new QLabel(badgeText, card);
        badge->setAlignment(Qt::AlignCenter);
        badge->setMinimumSize(58, 28);
        badge->setStyleSheet(QString(
            "QLabel { background:%1; color:white; font-size:12px; font-weight:700; border:none; border-radius:14px; }"
        ).arg(accent));
        topRow->addWidget(badge, 0, Qt::AlignTop);
        layout->addLayout(topRow);

        *valueLabel = new QLabel("0", card);
        (*valueLabel)->setStyleSheet("font-size:34px; font-weight:800; color:#1f2d15; border:none; background:transparent;");
        (*valueLabel)->setWordWrap(true);
        layout->addWidget(*valueLabel);

        auto *subLabel = new QLabel(subText, card);
        subLabel->setWordWrap(true);
        subLabel->setStyleSheet("font-size:13px; color:#6d7b56; border:none; background:transparent;");
        layout->addWidget(subLabel);

        return card;
    };

    auto *cardsGrid = new QGridLayout();
    cardsGrid->setHorizontalSpacing(16);
    cardsGrid->setVerticalSpacing(16);
    cardsGrid->addWidget(createStatCard("Total des lots", "GLOBAL", &m_huileTotalLotsValue, "#556B2F", "#f4f8eb", "Nombre total des lots enregistrés dans le module qualité."), 0, 0);
    cardsGrid->addWidget(createStatCard("Lots acceptés", "OK", &m_huileAcceptedValue, "#2f8f63", "#eef9f3", "Lots conformes et prêts à être valorisés."), 0, 1);
    cardsGrid->addWidget(createStatCard("Lots en cours", "SUIVI", &m_huileEnCoursValue, "#d4af37", "#fff9ea", "Lots encore en contrôle ou en analyse."), 1, 0);
    cardsGrid->addWidget(createStatCard("Lots rejetés", "ALERTE", &m_huileRejectedValue, "#b2413e", "#fff1f1", "Lots non conformes nécessitant une action rapide."), 1, 1);
    cardsGrid->addWidget(createStatCard("Quantité totale", "VOLUME", &m_huileTotalQuantiteValue, "#7f5b3b", "#f9f3ee", "Somme des quantités produites sur l'ensemble des lots."), 2, 0);
    cardsGrid->addWidget(createStatCard("Taux de conformité", "QUALITÉ", &m_huileConformiteValue, "#4c83c3", "#f1f6ff", "Part des lots acceptés par rapport au total."), 2, 1);
    cardsGrid->addWidget(createStatCard("pH moyen", "CHIMIE", &m_huileAvgPhValue, "#6b4f3f", "#fbf6f2", "Indicateur global de l'équilibre chimique des lots."), 3, 0);
    cardsGrid->addWidget(createStatCard("Acidité moyenne", "PURETÉ", &m_huileAvgAciditeValue, "#8a6f30", "#fff8ef", "Mesure moyenne de l'acidité observée sur les lots."), 3, 1);
    cardsGrid->addWidget(createStatCard("Température moyenne", "THERMIQUE", &m_huileAvgTempValue, "#b45f06", "#fff6ec", "Vision rapide de la stabilité thermique pendant la production."), 4, 0, 1, 2);

    auto *cardsWidget = new QWidget(container);
    cardsWidget->setLayout(cardsGrid);
    mainLayout->addWidget(cardsWidget);

    auto createChartSection = [](const QString &title, const QString &subtitle, QChartView **viewPtr, int minHeight) {
        auto *frame = new QFrame();
        frame->setStyleSheet("QFrame { background:#ffffff; border:1px solid #dbe6c8; border-radius:20px; }");
        frame->setMinimumHeight(minHeight);
        auto *layout = new QVBoxLayout(frame);
        layout->setContentsMargins(18, 18, 18, 18);
        layout->setSpacing(10);

        auto *titleLabel = new QLabel(title, frame);
        titleLabel->setStyleSheet("font-size:17px; font-weight:800; color:#2a391f; border:none; background:transparent;");
        titleLabel->setWordWrap(true);
        layout->addWidget(titleLabel);

        auto *subLabel = new QLabel(subtitle, frame);
        subLabel->setStyleSheet("font-size:13px; color:#6d7b56; border:none; background:transparent;");
        subLabel->setWordWrap(true);
        layout->addWidget(subLabel);

        *viewPtr = new QChartView(frame);
        (*viewPtr)->setRenderHint(QPainter::Antialiasing);
        (*viewPtr)->setMinimumHeight(minHeight - 95);
        (*viewPtr)->setStyleSheet("background:transparent; border:none;");
        layout->addWidget(*viewPtr, 1);
        return frame;
    };

    auto *chartsRow = new QHBoxLayout();
    chartsRow->setSpacing(16);
    chartsRow->addWidget(createChartSection(
        "Répartition par statut",
        "Vue globale des lots acceptés, en cours et rejetés pour repérer rapidement le niveau de conformité.",
        &m_pieChartView,
        330), 1);
    chartsRow->addWidget(createChartSection(
        "Top lots par quantité",
        "Classement des lots les plus volumineux pour suivre la production et la traçabilité.",
        &m_barChartView,
        330), 1);

    auto *chartsRowWidget = new QWidget(container);
    chartsRowWidget->setLayout(chartsRow);
    mainLayout->addWidget(chartsRowWidget);

    mainLayout->addWidget(createChartSection(
        "Profil qualité par lot",
        "Comparaison du pH, de l'acidité et de la température de production pour chaque lot.",
        &m_lineChartView,
        360));

    auto *insightFrame = createCardFrame("#fdfdf8", "#dbe6c8");
    auto *insightLayout = new QVBoxLayout(insightFrame);
    insightLayout->setContentsMargins(20, 18, 20, 18);
    insightLayout->setSpacing(8);

    auto *insightTitle = new QLabel("Lecture rapide", insightFrame);
    insightTitle->setStyleSheet("font-size:16px; font-weight:800; color:#556B2F; border:none; background:transparent;");
    insightLayout->addWidget(insightTitle);

    m_huileInsightLabel = new QLabel("Les indicateurs qualité de l'huile s'affichent ici après chargement des données.", insightFrame);
    m_huileInsightLabel->setWordWrap(true);
    m_huileInsightLabel->setStyleSheet("font-size:14px; color:#4c5840; border:none; background:transparent;");
    insightLayout->addWidget(m_huileInsightLabel);
    mainLayout->addWidget(insightFrame);
    mainLayout->addStretch(1);

    scroll->setWidget(container);
    pageLayout->addWidget(scroll);

    QStackedWidget *metiersHuile = ui->metiershuile;
    if (metiersHuile && metiersHuile->count() > 2) {
        QWidget *oldWidget = metiersHuile->widget(2);
        metiersHuile->removeWidget(oldWidget);
        metiersHuile->insertWidget(2, statWidget);
        delete oldWidget;
    }
}

void MainWindow::chargerStatistiquesHuile()
{
    auto setHuileDefaults = [this]() {
        if (m_huileTotalLotsValue) m_huileTotalLotsValue->setText("0");
        if (m_huileAcceptedValue) m_huileAcceptedValue->setText("0");
        if (m_huileEnCoursValue) m_huileEnCoursValue->setText("0");
        if (m_huileRejectedValue) m_huileRejectedValue->setText("0");
        if (m_huileTotalQuantiteValue) m_huileTotalQuantiteValue->setText("0 L");
        if (m_huileConformiteValue) m_huileConformiteValue->setText("0 %");
        if (m_huileAvgPhValue) m_huileAvgPhValue->setText("0.00");
        if (m_huileAvgAciditeValue) m_huileAvgAciditeValue->setText("0.00");
        if (m_huileAvgTempValue) m_huileAvgTempValue->setText("0.0 °C");
        if (m_huileInsightLabel) m_huileInsightLabel->setText("Aucune donnée qualité disponible pour le moment.");
    };

    setHuileDefaults();

    if (!m_pieChartView || !m_barChartView || !m_lineChartView)
        return;

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) {
        if (m_huileInsightLabel) {
            m_huileInsightLabel->setText("La base Oracle n'est pas connectée. Ouvre la connexion puis recharge les statistiques.");
        }
        return;
    }

    auto normalizeStatus = [](QString value) {
        value = value.trimmed().toLower();
        value.replace("é", "e");
        value.replace("è", "e");
        value.replace("ê", "e");
        value.replace("à", "a");
        value.replace("ù", "u");
        value.replace("î", "i");
        return value;
    };

    int totalLots = 0;
    int acceptedLots = 0;
    int enCoursLots = 0;
    int rejectedLots = 0;
    double totalQuantite = 0.0;
    double avgPh = 0.0;
    double avgAcidite = 0.0;
    double avgTemp = 0.0;

    QSqlQuery queryAgg;
    queryAgg.prepare("SELECT COUNT(*) AS total_lots, NVL(SUM(quantite_produite),0) AS total_quantite, NVL(AVG(ph),0) AS avg_ph, NVL(AVG(acidite),0) AS avg_acidite, NVL(AVG(temperature_production),0) AS avg_temp FROM QUALITE");
    if (queryAgg.exec() && queryAgg.next()) {
        totalLots = queryAgg.value("total_lots").toInt();
        totalQuantite = queryAgg.value("total_quantite").toDouble();
        avgPh = queryAgg.value("avg_ph").toDouble();
        avgAcidite = queryAgg.value("avg_acidite").toDouble();
        avgTemp = queryAgg.value("avg_temp").toDouble();
    }

    QMap<QString, int> statusCounts;
    QSqlQuery queryStatus;
    queryStatus.prepare("SELECT NVL(statut_qualite, 'Non défini') AS statut, COUNT(*) AS count FROM QUALITE GROUP BY statut_qualite");
    if (queryStatus.exec()) {
        while (queryStatus.next()) {
            const QString rawStatus = queryStatus.value("statut").toString();
            const int count = queryStatus.value("count").toInt();
            statusCounts.insert(rawStatus, count);

            const QString normalized = normalizeStatus(rawStatus);
            if (normalized.contains("accept") || normalized.contains("valid"))
                acceptedLots += count;
            else if (normalized.contains("rejet") || normalized.contains("refus"))
                rejectedLots += count;
            else
                enCoursLots += count;
        }
    }

    const double conformityRate = totalLots > 0 ? (acceptedLots * 100.0 / totalLots) : 0.0;

    if (m_huileTotalLotsValue) m_huileTotalLotsValue->setText(QString::number(totalLots));
    if (m_huileAcceptedValue) m_huileAcceptedValue->setText(QString::number(acceptedLots));
    if (m_huileEnCoursValue) m_huileEnCoursValue->setText(QString::number(enCoursLots));
    if (m_huileRejectedValue) m_huileRejectedValue->setText(QString::number(rejectedLots));
    if (m_huileTotalQuantiteValue) m_huileTotalQuantiteValue->setText(QString::number(totalQuantite, 'f', totalQuantite < 1000 ? 1 : 0) + " L");
    if (m_huileConformiteValue) m_huileConformiteValue->setText(QString::number(conformityRate, 'f', 1) + " %");
    if (m_huileAvgPhValue) m_huileAvgPhValue->setText(QString::number(avgPh, 'f', 2));
    if (m_huileAvgAciditeValue) m_huileAvgAciditeValue->setText(QString::number(avgAcidite, 'f', 2));
    if (m_huileAvgTempValue) m_huileAvgTempValue->setText(QString::number(avgTemp, 'f', 1) + " °C");

    // 1) Diagramme donut - répartition par statut
    auto *pieSeries = new QPieSeries();
    pieSeries->setHoleSize(0.45);
    const QVector<QPair<QString, QColor>> preferredColors = {
        {"Accepté", QColor("#4e9f6d")},
        {"En cours", QColor("#d4af37")},
        {"Rejeté", QColor("#c1514b")},
        {"Non défini", QColor("#7d8b67")}
    };

    bool hasStatusData = false;
    for (auto it = statusCounts.constBegin(); it != statusCounts.constEnd(); ++it) {
        if (it.value() <= 0)
            continue;
        hasStatusData = true;
        auto *slice = pieSeries->append(it.key() + " (" + QString::number(it.value()) + ")", it.value());
        slice->setLabelVisible(true);
        slice->setLabelArmLengthFactor(0.15);
        const QString normalized = normalizeStatus(it.key());
        QColor color("#7d8b67");
        if (normalized.contains("accept") || normalized.contains("valid")) color = QColor("#4e9f6d");
        else if (normalized.contains("rejet") || normalized.contains("refus")) color = QColor("#c1514b");
        else if (normalized.contains("cours") || normalized.contains("anal")) color = QColor("#d4af37");
        slice->setBrush(color);
        slice->setPen(QPen(Qt::white, 2));
    }
    if (!hasStatusData) {
        auto *slice = pieSeries->append("Aucune donnée", 1);
        slice->setBrush(QColor("#dce6ca"));
        slice->setLabelVisible(true);
    }

    auto *pieChart = new QChart();
    pieChart->addSeries(pieSeries);
    pieChart->legend()->setAlignment(Qt::AlignBottom);
    pieChart->legend()->setLabelColor(QColor("#415033"));
    pieChart->setBackgroundVisible(false);
    pieChart->setMargins(QMargins(6, 6, 6, 6));
    pieChart->setAnimationOptions(QChart::SeriesAnimations);
    m_pieChartView->setChart(pieChart);

    // 2) Top lots par quantité
    auto *barSet = new QBarSet("Quantité (L)");
    barSet->setColor(QColor("#8aa14a"));
    QStringList categories;
    QSqlQuery queryTopLots;
    queryTopLots.prepare(
        "SELECT id_lot, quantite_produite FROM ("
        "SELECT id_lot, NVL(quantite_produite,0) AS quantite_produite FROM QUALITE ORDER BY quantite_produite DESC, id_lot"
        ") WHERE ROWNUM <= 8"
    );
    if (queryTopLots.exec()) {
        while (queryTopLots.next()) {
            categories << ("Lot " + queryTopLots.value("id_lot").toString());
            *barSet << queryTopLots.value("quantite_produite").toDouble();
        }
    }
    if (categories.isEmpty()) {
        categories << "Aucun lot";
        *barSet << 0;
    }

    auto *barSeries = new QHorizontalBarSeries();
    barSeries->append(barSet);

    auto *barChart = new QChart();
    barChart->addSeries(barSeries);
    barChart->setBackgroundVisible(false);
    barChart->setMargins(QMargins(12, 12, 12, 12));
    barChart->legend()->hide();
    barChart->setAnimationOptions(QChart::SeriesAnimations);

    auto *axisY = new QBarCategoryAxis();
    axisY->append(categories);
    auto *axisX = new QValueAxis();
    axisX->setLabelFormat("%.0f");
    axisX->setTitleText("Litres");
    axisX->setGridLineColor(QColor("#e6ecd8"));
    axisX->setLinePenColor(QColor("#74855c"));
    axisX->setLabelsColor(QColor("#556B2F"));

    barChart->addAxis(axisY, Qt::AlignLeft);
    barChart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisY);
    barSeries->attachAxis(axisX);
    m_barChartView->setChart(barChart);

    // 3) Profil qualité par lot
    auto *phSeries = new QLineSeries();
    phSeries->setName("pH");
    phSeries->setColor(QColor("#2f8f63"));

    auto *aciditeSeries = new QLineSeries();
    aciditeSeries->setName("Acidité");
    aciditeSeries->setColor(QColor("#b36b2c"));

    auto *tempSeries = new QLineSeries();
    tempSeries->setName("Température");
    tempSeries->setColor(QColor("#4c83c3"));

    QSqlQuery queryProfile;
    queryProfile.prepare("SELECT id_lot, NVL(ph,0) AS ph, NVL(acidite,0) AS acidite, NVL(temperature_production,0) AS temperature_production FROM QUALITE ORDER BY id_lot");

    int pointIndex = 0;
    double maxLeftAxis = 14.0;
    double maxTemp = 0.0;
    if (queryProfile.exec()) {
        while (queryProfile.next()) {
            const double ph = queryProfile.value("ph").toDouble();
            const double acidite = queryProfile.value("acidite").toDouble();
            const double temp = queryProfile.value("temperature_production").toDouble();
            phSeries->append(pointIndex, ph);
            aciditeSeries->append(pointIndex, acidite);
            tempSeries->append(pointIndex, temp);
            maxLeftAxis = std::max(maxLeftAxis, std::max(ph, acidite) + 1.0);
            maxTemp = std::max(maxTemp, temp);
            ++pointIndex;
        }
    }
    if (pointIndex == 0) {
        phSeries->append(0, 0);
        aciditeSeries->append(0, 0);
        tempSeries->append(0, 0);
        pointIndex = 1;
    }

    auto *lineChart = new QChart();
    lineChart->setBackgroundVisible(false);
    lineChart->setMargins(QMargins(10, 10, 10, 10));
    lineChart->setAnimationOptions(QChart::SeriesAnimations);
    lineChart->legend()->setAlignment(Qt::AlignBottom);
    lineChart->legend()->setLabelColor(QColor("#415033"));
    lineChart->addSeries(phSeries);
    lineChart->addSeries(aciditeSeries);
    lineChart->addSeries(tempSeries);

    auto *xAxis = new QValueAxis();
    xAxis->setTitleText("Index de lot");
    xAxis->setRange(0, std::max(1, pointIndex - 1));
    xAxis->setTickCount(std::min(6, std::max(2, pointIndex)));
    xAxis->setGridLineColor(QColor("#edf2e3"));
    xAxis->setLabelsColor(QColor("#556B2F"));
    xAxis->setLinePenColor(QColor("#74855c"));

    auto *yAxisLeft = new QValueAxis();
    yAxisLeft->setTitleText("pH / Acidité");
    yAxisLeft->setRange(0, maxLeftAxis);
    yAxisLeft->setGridLineColor(QColor("#edf2e3"));
    yAxisLeft->setLabelsColor(QColor("#556B2F"));
    yAxisLeft->setLinePenColor(QColor("#74855c"));

    auto *yAxisRight = new QValueAxis();
    yAxisRight->setTitleText("Température");
    yAxisRight->setRange(0, std::max(10.0, maxTemp + 5.0));
    yAxisRight->setLabelsColor(QColor("#4c83c3"));
    yAxisRight->setLinePenColor(QColor("#4c83c3"));

    lineChart->addAxis(xAxis, Qt::AlignBottom);
    lineChart->addAxis(yAxisLeft, Qt::AlignLeft);
    lineChart->addAxis(yAxisRight, Qt::AlignRight);

    phSeries->attachAxis(xAxis);
    phSeries->attachAxis(yAxisLeft);
    aciditeSeries->attachAxis(xAxis);
    aciditeSeries->attachAxis(yAxisLeft);
    tempSeries->attachAxis(xAxis);
    tempSeries->attachAxis(yAxisRight);

    m_lineChartView->setChart(lineChart);

    QString insight = QString("Conformité globale : %1 %. ").arg(QString::number(conformityRate, 'f', 1));
    if (conformityRate >= 80.0)
        insight += "La majorité des lots est conforme, ce qui est rassurant pour la qualité globale.";
    else if (conformityRate >= 50.0)
        insight += "La conformité est moyenne : il faut surveiller les lots en cours et réduire les rejets.";
    else
        insight += "Le niveau de conformité est faible : il faut analyser rapidement les causes de non-conformité.";

    if (totalLots > 0) {
        QSqlQuery queryTopLot;
        queryTopLot.prepare(
            "SELECT id_lot, NVL(quantite_produite,0) AS quantite_produite FROM ("
            "SELECT id_lot, quantite_produite FROM QUALITE ORDER BY quantite_produite DESC NULLS LAST, id_lot"
            ") WHERE ROWNUM = 1"
        );
        if (queryTopLot.exec() && queryTopLot.next()) {
            insight += QString(" Lot principal : lot %1 avec %2 L produits.")
                       .arg(queryTopLot.value("id_lot").toString(),
                            QString::number(queryTopLot.value("quantite_produite").toDouble(), 'f', 1));
        }
    }
    const double premiumIndex = qBound(0.0, 100.0 - (std::abs(avgPh - 7.0) * 10.0 + avgAcidite * 12.0 + std::max(0.0, avgTemp - 27.0) * 2.0), 100.0);
    if (premiumIndex >= 80.0)
        insight += " Indice premium élevé : la matière semble stable et bien maîtrisée.";
    else if (premiumIndex >= 55.0)
        insight += " Indice premium intermédiaire : surveiller la dérive chimique et thermique.";
    else
        insight += " Indice premium fragile : prévoir un contrôle renforcé sur les prochains lots.";

    insight += QString(" pH moyen : %1. Acidité moyenne : %2. Température moyenne : %3 °C.")
               .arg(QString::number(avgPh, 'f', 2), QString::number(avgAcidite, 'f', 2), QString::number(avgTemp, 'f', 1));

    if (m_huileInsightLabel)
        m_huileInsightLabel->setText(insight);
}

void MainWindow::on_btnStatHuile_clicked()
{
    if (ui->metiershuile) {
        ui->metiershuile->setCurrentIndex(2);
        chargerStatistiquesHuile();
    }
}

void MainWindow::setupAdvancedHuilePage()
{
    QWidget *advancedWidget = new QWidget();
    auto *pageLayout = new QVBoxLayout(advancedWidget);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea(advancedWidget);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *container = new QWidget();
    auto *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(26, 24, 26, 24);
    mainLayout->setSpacing(18);

    auto createFrame = [](const QString &bg, const QString &border) {
        auto *frame = new QFrame();
        frame->setStyleSheet(QString("QFrame { background:%1; border:1px solid %2; border-radius:18px; }").arg(bg, border));
        return frame;
    };

    auto *hero = createFrame("#fbfcf7", "#dbe6c8");
    auto *heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(22, 18, 22, 18);
    heroLayout->setSpacing(8);
    auto *heroTitle = new QLabel("Score qualité de l'huile - lecture simple", hero);
    heroTitle->setStyleSheet("font-size:24px; font-weight:800; color:#253418; border:none; background:transparent;");
    heroTitle->setWordWrap(true);
    auto *heroDesc = new QLabel("Le score part de 100. On enlève une pénalité si le pH sort de la zone idéale et une autre si l'acidité dépasse la limite fixée. Le tableau montre chaque étape du calcul pour que le score soit facile à comprendre.", hero);
    heroDesc->setStyleSheet("font-size:14px; color:#5f6f48; border:none; background:transparent;");
    heroDesc->setWordWrap(true);
    heroLayout->addWidget(heroTitle);
    heroLayout->addWidget(heroDesc);
    mainLayout->addWidget(hero);

    auto createKpiCard = [container](const QString &title, QLabel **valueLabel, const QString &accent, const QString &bg, const QString &subText) {
        auto *card = new QFrame(container);
        card->setMinimumSize(250, 130);
        card->setStyleSheet(QString(
            "QFrame { background:%1; border:1px solid #d7e2c5; border-radius:18px; }"
            "QLabel[role='title'] { color:#27361a; font-size:15px; font-weight:700; }"
            "QLabel[role='value'] { color:#1f2d15; font-size:32px; font-weight:800; }"
            "QLabel[role='badge'] { background:%2; color:white; border:none; border-radius:13px; font-size:11px; font-weight:700; padding:4px 10px; }"
            "QLabel[role='sub'] { color:#677255; font-size:12px; }"
        ).arg(bg, accent));
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(16, 14, 16, 14);
        layout->setSpacing(8);

        auto *top = new QHBoxLayout();
        auto *titleLabel = new QLabel(title, card);
        titleLabel->setProperty("role", "title");
        titleLabel->setWordWrap(true);
        auto *badge = new QLabel("IA", card);
        badge->setProperty("role", "badge");
        badge->setAlignment(Qt::AlignCenter);
        top->addWidget(titleLabel, 1);
        top->addWidget(badge, 0, Qt::AlignTop);
        layout->addLayout(top);

        *valueLabel = new QLabel("0", card);
        (*valueLabel)->setProperty("role", "value");
        (*valueLabel)->setWordWrap(true);
        layout->addWidget(*valueLabel);

        auto *sub = new QLabel(subText, card);
        sub->setProperty("role", "sub");
        sub->setWordWrap(true);
        layout->addWidget(sub);
        return card;
    };

    auto *kpiGrid = new QGridLayout();
    kpiGrid->setHorizontalSpacing(16);
    kpiGrid->setVerticalSpacing(16);
    kpiGrid->addWidget(createKpiCard("Score moyen", &m_huileScoreMoyenValue, "#556B2F", "#f4f8eb", "Moyenne des scores obtenus après les pénalités pH et acidité."), 0, 0);
    kpiGrid->addWidget(createKpiCard("Lots verts", &m_huilePremiumLotsValue, "#2f8f63", "#eef9f3", "Lots dont le score reste au-dessus du seuil vert."), 0, 1);
    kpiGrid->addWidget(createKpiCard("Lots orange", &m_huileSurveillanceLotsValue, "#d4af37", "#fff9ea", "Lots à surveiller avant validation finale."), 1, 0);
    kpiGrid->addWidget(createKpiCard("Lots rouges", &m_huileCritiqueLotsValue, "#b2413e", "#fff1f1", "Lots avec un score faible nécessitant une action rapide."), 1, 1);

    auto *kpiWidget = new QWidget(container);
    kpiWidget->setLayout(kpiGrid);
    mainLayout->addWidget(kpiWidget);

    auto *formulaFrame = createFrame("#ffffff", "#dbe6c8");
    auto *formulaLayout = new QVBoxLayout(formulaFrame);
    formulaLayout->setContentsMargins(18, 18, 18, 18);
    formulaLayout->setSpacing(12);
    auto *formulaTitle = new QLabel("Formule simple du score", formulaFrame);
    formulaTitle->setStyleSheet("font-size:18px; font-weight:800; color:#2a391f; border:none; background:transparent;");
    auto *formulaSub = new QLabel("Le score démarre à 100. On retire une pénalité pH si le lot sort de la zone idéale, puis une pénalité acidité si la limite est dépassée. Formule : Score = 100 - pénalité pH - pénalité acidité.", formulaFrame);
    formulaSub->setStyleSheet("font-size:13px; color:#6d7b56; border:none; background:transparent;");
    formulaSub->setWordWrap(true);
    formulaLayout->addWidget(formulaTitle);
    formulaLayout->addWidget(formulaSub);

    auto configureSpin = [](QDoubleSpinBox *spin, double min, double max, double step, double value, const QString &suffix = QString()) {
        spin->setRange(min, max);
        spin->setSingleStep(step);
        spin->setValue(value);
        spin->setDecimals(step < 1.0 ? 2 : 0);
        spin->setSuffix(suffix);
        spin->setMinimumHeight(46);
        spin->setMinimumWidth(220);
        spin->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
        spin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        spin->setStyleSheet("QDoubleSpinBox { background:#fbfcf8; border:1px solid #d7e2c5; border-radius:12px; padding:6px 12px; font-size:14px; color:#2a391f; }"
                            "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width:20px; }"
                            "QDoubleSpinBox::up-arrow, QDoubleSpinBox::down-arrow { width:10px; height:10px; }");
    };

    m_huilePhIdealMinSpin = new QDoubleSpinBox(formulaFrame);
    configureSpin(m_huilePhIdealMinSpin, 0.0, 14.0, 0.05, 4.80);
    m_huilePhIdealMaxSpin = new QDoubleSpinBox(formulaFrame);
    configureSpin(m_huilePhIdealMaxSpin, 0.0, 14.0, 0.05, 5.60);
    m_huileAcidIdealMaxSpin = new QDoubleSpinBox(formulaFrame);
    configureSpin(m_huileAcidIdealMaxSpin, 0.0, 10.0, 0.05, 0.80, " %");
    m_huilePhWeightSpin = new QDoubleSpinBox(formulaFrame);
    configureSpin(m_huilePhWeightSpin, 1.0, 30.0, 0.5, 10.0);
    m_huileAcidWeightSpin = new QDoubleSpinBox(formulaFrame);
    configureSpin(m_huileAcidWeightSpin, 1.0, 30.0, 0.5, 20.0);
    m_huileBalanceBonusSpin = new QDoubleSpinBox(formulaFrame);
    configureSpin(m_huileBalanceBonusSpin, 50.0, 95.0, 1.0, 70.0);
    m_huileSynergyPenaltySpin = new QDoubleSpinBox(formulaFrame);
    configureSpin(m_huileSynergyPenaltySpin, 20.0, 80.0, 1.0, 50.0);

    const QString labelStyle = QStringLiteral("QLabel { color:#334126; font-size:15px; font-weight:700; border:none; background:transparent; }");

    auto createFieldBlock = [&](const QString &label, QWidget *field, QWidget *parent) {
        auto *block = new QWidget(parent);
        auto *blockLayout = new QVBoxLayout(block);
        blockLayout->setContentsMargins(0, 0, 0, 0);
        blockLayout->setSpacing(8);

        auto *lbl = new QLabel(label, block);
        lbl->setStyleSheet(labelStyle);
        lbl->setWordWrap(true);
        lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        if (field) {
            field->setParent(block);
            field->setMinimumHeight(46);
            field->setMinimumWidth(220);
            field->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }

        blockLayout->addWidget(lbl);
        if (field)
            blockLayout->addWidget(field);
        return block;
    };

    auto *formulaColumns = new QHBoxLayout();
    formulaColumns->setSpacing(22);

    auto *formulaLeftCol = new QVBoxLayout();
    formulaLeftCol->setSpacing(14);
    formulaLeftCol->addWidget(createFieldBlock(QStringLiteral("pH idéal min"), m_huilePhIdealMinSpin, formulaFrame));
    formulaLeftCol->addWidget(createFieldBlock(QStringLiteral("Acidité max sans pénalité"), m_huileAcidIdealMaxSpin, formulaFrame));
    formulaLeftCol->addWidget(createFieldBlock(QStringLiteral("Coefficient acidité"), m_huileAcidWeightSpin, formulaFrame));
    formulaLeftCol->addWidget(createFieldBlock(QStringLiteral("Seuil rouge"), m_huileSynergyPenaltySpin, formulaFrame));
    formulaLeftCol->addStretch();

    auto *formulaRightCol = new QVBoxLayout();
    formulaRightCol->setSpacing(14);
    formulaRightCol->addWidget(createFieldBlock(QStringLiteral("pH idéal max"), m_huilePhIdealMaxSpin, formulaFrame));
    formulaRightCol->addWidget(createFieldBlock(QStringLiteral("Coefficient pH"), m_huilePhWeightSpin, formulaFrame));
    formulaRightCol->addWidget(createFieldBlock(QStringLiteral("Seuil vert"), m_huileBalanceBonusSpin, formulaFrame));
    formulaRightCol->addStretch();

    formulaColumns->addLayout(formulaLeftCol, 1);
    formulaColumns->addLayout(formulaRightCol, 1);
    formulaLayout->addLayout(formulaColumns);

    auto *formulaActions = new QHBoxLayout();
    formulaActions->addStretch();
    m_huileAdvancedRefreshBtn = new QPushButton("Calculer le score", formulaFrame);
    m_huileAdvancedRefreshBtn->setMinimumHeight(38);
    m_huileAdvancedRefreshBtn->setStyleSheet("QPushButton { background:#556B2F; color:white; border:none; border-radius:12px; padding:8px 16px; font-weight:700; } QPushButton:hover { background:#435624; }");
    m_huileAdvancedExportBtn = new QPushButton("Exporter le tableau", formulaFrame);
    m_huileAdvancedExportBtn->setMinimumHeight(38);
    m_huileAdvancedExportBtn->setStyleSheet("QPushButton { background:#ffffff; color:#556B2F; border:1px solid #c8d5ab; border-radius:12px; padding:8px 16px; font-weight:700; } QPushButton:hover { background:#f4f8eb; }");
    formulaActions->addWidget(m_huileAdvancedRefreshBtn);
    formulaActions->addWidget(m_huileAdvancedExportBtn);
    formulaLayout->addLayout(formulaActions);
    mainLayout->addWidget(formulaFrame);

    auto createChartFrame = [](const QString &title, const QString &subtitle, QChartView **viewPtr, int minHeight) {
        auto *frame = new QFrame();
        frame->setStyleSheet("QFrame { background:#ffffff; border:1px solid #dbe6c8; border-radius:18px; }");
        auto *layout = new QVBoxLayout(frame);
        layout->setContentsMargins(18, 18, 18, 18);
        layout->setSpacing(10);
        auto *titleLabel = new QLabel(title, frame);
        titleLabel->setStyleSheet("font-size:17px; font-weight:800; color:#2a391f; border:none; background:transparent;");
        titleLabel->setWordWrap(true);
        auto *subLabel = new QLabel(subtitle, frame);
        subLabel->setStyleSheet("font-size:13px; color:#6d7b56; border:none; background:transparent;");
        subLabel->setWordWrap(true);
        *viewPtr = new QChartView(frame);
        (*viewPtr)->setRenderHint(QPainter::Antialiasing);
        (*viewPtr)->setMinimumHeight(minHeight - 90);
        (*viewPtr)->setStyleSheet("background:transparent; border:none;");
        layout->addWidget(titleLabel);
        layout->addWidget(subLabel);
        layout->addWidget(*viewPtr, 1);
        frame->setMinimumHeight(minHeight);
        return frame;
    };

    auto *chartsRow = new QHBoxLayout();
    chartsRow->setSpacing(16);
    chartsRow->addWidget(createChartFrame("Répartition vert / orange / rouge", "Répartition simple des lots selon le score final.", &m_huileAdvancedClassChart, 320), 1);
    chartsRow->addWidget(createChartFrame("Top lots par score", "Les meilleurs scores permettent d'identifier les lots à forte valeur marchande.", &m_huileAdvancedTopChart, 320), 1);
    auto *chartsWidget = new QWidget(container);
    chartsWidget->setLayout(chartsRow);
    mainLayout->addWidget(chartsWidget);

    auto *tableFrame = createFrame("#ffffff", "#dbe6c8");
    auto *tableLayout = new QVBoxLayout(tableFrame);
    tableLayout->setContentsMargins(18, 18, 18, 18);
    tableLayout->setSpacing(10);
    auto *tableTitle = new QLabel("Analyse détaillée des lots", tableFrame);
    tableTitle->setStyleSheet("font-size:18px; font-weight:800; color:#2a391f; border:none; background:transparent;");
    auto *tableSub = new QLabel("Chaque ligne détaille le calcul du score : pénalité pH, pénalité acidité, score final et couleur.", tableFrame);
    tableSub->setStyleSheet("font-size:13px; color:#6d7b56; border:none; background:transparent;");
    tableSub->setWordWrap(true);
    m_huileAdvancedTable = new QTableWidget(tableFrame);
    m_huileAdvancedTable->setColumnCount(9);
    m_huileAdvancedTable->setHorizontalHeaderLabels({"Lot", "pH", "Acidité", "Pénalité pH", "Pénalité acidité", "Score", "Couleur", "Classe", "Calcul"});
    m_huileAdvancedTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_huileAdvancedTable->horizontalHeader()->setMinimumSectionSize(90);
    m_huileAdvancedTable->verticalHeader()->setVisible(false);
    m_huileAdvancedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_huileAdvancedTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_huileAdvancedTable->setAlternatingRowColors(false);
    m_huileAdvancedTable->setShowGrid(true);
    m_huileAdvancedTable->setMinimumHeight(320);
    m_huileAdvancedTable->setStyleSheet("QTableWidget { background:white; gridline-color:#d9e2c8; border:none; } QHeaderView::section { background:#f4f8eb; color:#2a391f; font-weight:700; border:none; border-bottom:1px solid #d9e2c8; padding:8px; }");
    tableLayout->addWidget(tableTitle);
    tableLayout->addWidget(tableSub);
    tableLayout->addWidget(m_huileAdvancedTable);

    m_huileAdvancedInsightLabel = new QLabel("Le détail du calcul s'affichera ici après chargement des lots.", tableFrame);
    m_huileAdvancedInsightLabel->setStyleSheet("font-size:14px; color:#4c5840; border:none; background:transparent;");
    m_huileAdvancedInsightLabel->setWordWrap(true);
    tableLayout->addWidget(m_huileAdvancedInsightLabel);
    mainLayout->addWidget(tableFrame);

    auto *forecastKpiGrid = new QGridLayout();
    forecastKpiGrid->setHorizontalSpacing(16);
    forecastKpiGrid->setVerticalSpacing(16);
    forecastKpiGrid->addWidget(createKpiCard("Score prévu moyen N+1", &m_huileForecastScoreMoyenValue, "#4f6d2f", "#f2f7e8", "Projection moyenne de la qualité de l'huile pour l'année prochaine selon la formule active."), 0, 0);
    forecastKpiGrid->addWidget(createKpiCard("Lots premium prévus", &m_huileForecastPremiumValue, "#2f8f63", "#eef9f3", "Lots qui devraient conserver un excellent profil qualité à horizon un an."), 0, 1);
    forecastKpiGrid->addWidget(createKpiCard("Lots sous vigilance", &m_huileForecastVigilanceValue, "#d4af37", "#fff9ea", "Lots orange si la dérive chimique se confirme l'année prochaine."), 1, 0);
    forecastKpiGrid->addWidget(createKpiCard("Lots rouges N+1", &m_huileForecastCritiqueValue, "#b2413e", "#fff1f1", "Lots susceptibles de devenir critiques si aucune action préventive n'est engagée."), 1, 1);
    auto *forecastKpiWidget = new QWidget(container);
    forecastKpiWidget->setLayout(forecastKpiGrid);
    mainLayout->addWidget(forecastKpiWidget);

    auto *forecastFormulaFrame = createFrame("#ffffff", "#dbe6c8");
    auto *forecastFormulaLayout = new QVBoxLayout(forecastFormulaFrame);
    forecastFormulaLayout->setContentsMargins(18, 18, 18, 18);
    forecastFormulaLayout->setSpacing(12);
    auto *forecastTitle = new QLabel("Prédiction créative de la qualité N+1", forecastFormulaFrame);
    forecastTitle->setStyleSheet("font-size:18px; font-weight:800; color:#2a391f; border:none; background:transparent;");
    auto *forecastSub = new QLabel("Le moteur projette la dérive annuelle de l'acidité et du pH, applique un stress d'oxydation aux lots déjà fragiles et un bonus de stabilité aux lots premium afin d'estimer la classe qualité de l'année prochaine.", forecastFormulaFrame);
    forecastSub->setStyleSheet("font-size:13px; color:#6d7b56; border:none; background:transparent;");
    forecastSub->setWordWrap(true);
    forecastFormulaLayout->addWidget(forecastTitle);
    forecastFormulaLayout->addWidget(forecastSub);

    m_huileForecastPhDriftSpin = new QDoubleSpinBox(forecastFormulaFrame);
    configureSpin(m_huileForecastPhDriftSpin, 0.00, 2.00, 0.05, 0.12);
    m_huileForecastAcidDriftSpin = new QDoubleSpinBox(forecastFormulaFrame);
    configureSpin(m_huileForecastAcidDriftSpin, 0.00, 3.00, 0.05, 0.18, " %");
    m_huileForecastOxidationSpin = new QDoubleSpinBox(forecastFormulaFrame);
    configureSpin(m_huileForecastOxidationSpin, 0.00, 25.00, 0.50, 6.00);
    m_huileForecastStabilityBonusSpin = new QDoubleSpinBox(forecastFormulaFrame);
    configureSpin(m_huileForecastStabilityBonusSpin, 0.00, 20.00, 0.50, 4.00);

    auto *forecastColumns = new QHBoxLayout();
    forecastColumns->setSpacing(22);

    auto *forecastLeftCol = new QVBoxLayout();
    forecastLeftCol->setSpacing(14);
    forecastLeftCol->addWidget(createFieldBlock(QStringLiteral("Dérive annuelle du pH"), m_huileForecastPhDriftSpin, forecastFormulaFrame));
    forecastLeftCol->addWidget(createFieldBlock(QStringLiteral("Stress d'oxydation"), m_huileForecastOxidationSpin, forecastFormulaFrame));
    forecastLeftCol->addStretch();

    auto *forecastRightCol = new QVBoxLayout();
    forecastRightCol->setSpacing(14);
    forecastRightCol->addWidget(createFieldBlock(QStringLiteral("Hausse annuelle d'acidité"), m_huileForecastAcidDriftSpin, forecastFormulaFrame));
    forecastRightCol->addWidget(createFieldBlock(QStringLiteral("Bonus de stabilité"), m_huileForecastStabilityBonusSpin, forecastFormulaFrame));
    forecastRightCol->addStretch();

    forecastColumns->addLayout(forecastLeftCol, 1);
    forecastColumns->addLayout(forecastRightCol, 1);
    forecastFormulaLayout->addLayout(forecastColumns);

    auto *forecastActions = new QHBoxLayout();
    forecastActions->addStretch();
    m_huileForecastRefreshBtn = new QPushButton("Simuler l'année prochaine", forecastFormulaFrame);
    m_huileForecastRefreshBtn->setMinimumHeight(38);
    m_huileForecastRefreshBtn->setStyleSheet("QPushButton { background:#6b4f3f; color:white; border:none; border-radius:12px; padding:8px 16px; font-weight:700; } QPushButton:hover { background:#543c2f; }");
    m_huileForecastExportBtn = new QPushButton("Exporter la prévision", forecastFormulaFrame);
    m_huileForecastExportBtn->setMinimumHeight(38);
    m_huileForecastExportBtn->setStyleSheet("QPushButton { background:#ffffff; color:#6b4f3f; border:1px solid #d7c9ba; border-radius:12px; padding:8px 16px; font-weight:700; } QPushButton:hover { background:#faf4ef; }");
    forecastActions->addWidget(m_huileForecastRefreshBtn);
    forecastActions->addWidget(m_huileForecastExportBtn);
    forecastFormulaLayout->addLayout(forecastActions);
    mainLayout->addWidget(forecastFormulaFrame);

    auto *forecastChartsRow = new QHBoxLayout();
    forecastChartsRow->setSpacing(16);
    forecastChartsRow->addWidget(createChartFrame("Carte des classes N+1", "Répartition projetée des lots premium, vigilants ou critiques pour l'année prochaine.", &m_huileForecastClassChart, 320), 1);
    forecastChartsRow->addWidget(createChartFrame("Évolution actuelle vs N+1", "Comparaison des scores actuels et prévus pour les lots les plus représentatifs.", &m_huileForecastTrendChart, 320), 1);
    auto *forecastChartsWidget = new QWidget(container);
    forecastChartsWidget->setLayout(forecastChartsRow);
    mainLayout->addWidget(forecastChartsWidget);

    auto *forecastTableFrame = createFrame("#ffffff", "#dbe6c8");
    auto *forecastTableLayout = new QVBoxLayout(forecastTableFrame);
    forecastTableLayout->setContentsMargins(18, 18, 18, 18);
    forecastTableLayout->setSpacing(10);
    auto *forecastTableTitle = new QLabel("Projection de qualité de l'huile pour l'année prochaine", forecastTableFrame);
    forecastTableTitle->setStyleSheet("font-size:18px; font-weight:800; color:#2a391f; border:none; background:transparent;");
    auto *forecastTableSub = new QLabel("Chaque lot reçoit une projection N+1 : pH projeté, acidité future, score prévisionnel, tendance et recommandation stratégique.", forecastTableFrame);
    forecastTableSub->setStyleSheet("font-size:13px; color:#6d7b56; border:none; background:transparent;");
    forecastTableSub->setWordWrap(true);
    m_huileForecastTable = new QTableWidget(forecastTableFrame);
    m_huileForecastTable->setColumnCount(9);
    m_huileForecastTable->setHorizontalHeaderLabels({"Lot", "pH actuel", "Acidité actuelle", "pH N+1", "Acidité N+1", "Score N+1", "Classe future", "Tendance", "Recommandation"});
    m_huileForecastTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_huileForecastTable->horizontalHeader()->setMinimumSectionSize(100);
    m_huileForecastTable->verticalHeader()->setVisible(false);
    m_huileForecastTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_huileForecastTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_huileForecastTable->setAlternatingRowColors(false);
    m_huileForecastTable->setShowGrid(true);
    m_huileForecastTable->setMinimumHeight(340);
    m_huileForecastTable->setStyleSheet("QTableWidget { background:white; gridline-color:#d9e2c8; border:none; } QHeaderView::section { background:#fbf6f2; color:#4c3428; font-weight:700; border:none; border-bottom:1px solid #e6dccf; padding:8px; }");
    forecastTableLayout->addWidget(forecastTableTitle);
    forecastTableLayout->addWidget(forecastTableSub);
    forecastTableLayout->addWidget(m_huileForecastTable);
    m_huileForecastInsightLabel = new QLabel("La projection N+1 apparaîtra ici après simulation.", forecastTableFrame);
    m_huileForecastInsightLabel->setStyleSheet("font-size:14px; color:#5d4a3f; border:none; background:transparent;");
    m_huileForecastInsightLabel->setWordWrap(true);
    forecastTableLayout->addWidget(m_huileForecastInsightLabel);
    mainLayout->addWidget(forecastTableFrame);

    mainLayout->addStretch(1);
    scroll->setWidget(container);
    pageLayout->addWidget(scroll);

    if (ui->metiershuile && ui->metiershuile->count() > 3) {
        QWidget *oldWidget = ui->metiershuile->widget(3);
        ui->metiershuile->removeWidget(oldWidget);
        ui->metiershuile->insertWidget(3, advancedWidget);
        delete oldWidget;
    }

    if (m_huileAdvancedRefreshBtn) {
        QObject::connect(m_huileAdvancedRefreshBtn, &QPushButton::clicked, this, [this]() {
            chargerAnalyseAvanceeHuile();
        });
    }
    if (m_huileAdvancedExportBtn) {
        QObject::connect(m_huileAdvancedExportBtn, &QPushButton::clicked, this, [this]() {
            if (m_huileAdvancedTable)
                exportTableToCsv(m_huileAdvancedTable, "analyse_avancee_huile.csv");
        });
    }
    if (m_huileForecastRefreshBtn) {
        QObject::connect(m_huileForecastRefreshBtn, &QPushButton::clicked, this, [this]() {
            chargerAnalyseAvanceeHuile();
        });
    }
    if (m_huileForecastExportBtn) {
        QObject::connect(m_huileForecastExportBtn, &QPushButton::clicked, this, [this]() {
            if (m_huileForecastTable)
                exportTableToCsv(m_huileForecastTable, "projection_qualite_huile_n_plus_1.csv");
        });
    }
}

struct HuileScoreBreakdown
{
    double phPenalty = 0.0;
    double acidPenalty = 0.0;
    double score = 0.0;
};

static HuileScoreBreakdown buildHuileScoreBreakdown(double ph, double acidite, double phMin, double phMax, double acidTarget, double phWeight, double acidWeight)
{
    if (phMin > phMax)
        std::swap(phMin, phMax);

    HuileScoreBreakdown data;
    // Version simple et lisible du score :
    // Score = 100 - penalite pH - penalite acidite
    if (ph < phMin)
        data.phPenalty = (phMin - ph) * phWeight;
    else if (ph > phMax)
        data.phPenalty = (ph - phMax) * phWeight;

    if (acidite > acidTarget)
        data.acidPenalty = (acidite - acidTarget) * acidWeight;

    data.phPenalty = std::clamp(data.phPenalty, 0.0, 100.0);
    data.acidPenalty = std::clamp(data.acidPenalty, 0.0, 100.0);
    data.score = std::clamp(100.0 - data.phPenalty - data.acidPenalty, 0.0, 100.0);
    return data;
}

static void normalizeHuileScoreThresholds(double &greenThreshold, double &redThreshold)
{
    greenThreshold = std::clamp(greenThreshold, 0.0, 100.0);
    redThreshold = std::clamp(redThreshold, 0.0, 100.0);
    if (redThreshold > greenThreshold)
        std::swap(redThreshold, greenThreshold);
}

double MainWindow::computeHuileQualityScore(double ph, double acidite) const
{
    if (!m_huilePhIdealMinSpin || !m_huilePhIdealMaxSpin || !m_huileAcidIdealMaxSpin || !m_huilePhWeightSpin || !m_huileAcidWeightSpin)
        return 0.0;

    const HuileScoreBreakdown data = buildHuileScoreBreakdown(
        ph,
        acidite,
        m_huilePhIdealMinSpin->value(),
        m_huilePhIdealMaxSpin->value(),
        m_huileAcidIdealMaxSpin->value(),
        m_huilePhWeightSpin->value(),
        m_huileAcidWeightSpin->value());

    return data.score;
}

QString MainWindow::huileScoreLabel(double score) const
{
    double greenThreshold = m_huileBalanceBonusSpin ? m_huileBalanceBonusSpin->value() : 70.0;
    double redThreshold = m_huileSynergyPenaltySpin ? m_huileSynergyPenaltySpin->value() : 50.0;
    normalizeHuileScoreThresholds(greenThreshold, redThreshold);

    if (score >= greenThreshold) return QStringLiteral("Vert");
    if (score >= redThreshold) return QStringLiteral("Orange");
    return QStringLiteral("Rouge");
}

QColor MainWindow::huileScoreColor(double score) const
{
    const QString label = huileScoreLabel(score);
    if (label == QStringLiteral("Vert")) return QColor("#E8F5E9");
    if (label == QStringLiteral("Orange")) return QColor("#FFF3E0");
    return QColor("#FFEBEE");
}

void MainWindow::applyHuileScoreColorsRow(int row, double score)
{
    if (!m_huileAdvancedTable) return;
    const QString label = huileScoreLabel(score);
    const QColor bg = huileScoreColor(score);
    QColor emphasis = QColor("#C8E6C9");
    if (label == QStringLiteral("Orange")) emphasis = QColor("#FFE0B2");
    else if (label == QStringLiteral("Rouge")) emphasis = QColor("#FFCDD2");

    for (int c = 0; c < m_huileAdvancedTable->columnCount(); ++c) {
        auto *item = m_huileAdvancedTable->item(row, c);
        if (!item) continue;
        item->setBackground(bg);
        item->setForeground(QBrush(QColor("#2F2F2F")));
        item->setTextAlignment(Qt::AlignCenter);
    }

    for (int c : {5, 6, 7}) {
        if (auto *item = m_huileAdvancedTable->item(row, c)) {
            item->setBackground(emphasis);
            item->setForeground(QBrush(QColor("#1F1F1F")));
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
        }
    }
}

QString MainWindow::huileForecastTrendLabel(double currentScore, double futureScore) const
{
    const double delta = futureScore - currentScore;
    if (delta >= 5.0) return "Amélioration";
    if (delta <= -12.0) return "Dégradation forte";
    if (delta <= -5.0) return "Dégradation modérée";
    return "Stable";
}

void MainWindow::applyHuileForecastColorsRow(int row, double score)
{
    if (!m_huileForecastTable) return;
    const QColor bg = huileScoreColor(score);
    QColor emphasis = QColor("#cddab7");
    if (score >= 85.0) emphasis = QColor("#8db58d");
    else if (score >= 70.0) emphasis = QColor("#b8c98f");
    else if (score >= 55.0) emphasis = QColor("#e4c66f");
    else emphasis = QColor("#e6a19c");

    for (int c = 0; c < m_huileForecastTable->columnCount(); ++c) {
        auto *item = m_huileForecastTable->item(row, c);
        if (!item) continue;
        item->setBackground(bg);
        item->setForeground(QBrush(QColor("#2c2c2c")));
        item->setTextAlignment(Qt::AlignCenter);
    }

    for (int c : {5, 6, 7}) {
        if (auto *item = m_huileForecastTable->item(row, c)) {
            item->setBackground(emphasis);
            item->setForeground(QBrush(QColor("#1f2d15")));
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
        }
    }
}

void MainWindow::chargerAnalyseAvanceeHuile()
{
    if (!m_huileAdvancedTable || !m_huileAdvancedClassChart || !m_huileAdvancedTopChart)
        return;

    if (m_huileScoreMoyenValue) m_huileScoreMoyenValue->setText("0 / 100");
    if (m_huilePremiumLotsValue) m_huilePremiumLotsValue->setText("0");
    if (m_huileSurveillanceLotsValue) m_huileSurveillanceLotsValue->setText("0");
    if (m_huileCritiqueLotsValue) m_huileCritiqueLotsValue->setText("0");
    if (m_huileAdvancedInsightLabel) m_huileAdvancedInsightLabel->setText("Aucune donnée chargée.");
    m_huileAdvancedTable->setRowCount(0);

    if (m_huileForecastScoreMoyenValue) m_huileForecastScoreMoyenValue->setText("0 / 100");
    if (m_huileForecastPremiumValue) m_huileForecastPremiumValue->setText("0");
    if (m_huileForecastVigilanceValue) m_huileForecastVigilanceValue->setText("0");
    if (m_huileForecastCritiqueValue) m_huileForecastCritiqueValue->setText("0");
    if (m_huileForecastInsightLabel) m_huileForecastInsightLabel->setText("Aucune projection calculée.");
    if (m_huileForecastTable) m_huileForecastTable->setRowCount(0);

    auto makeEmptyChart = []() {
        auto *chart = new QChart();
        chart->setBackgroundVisible(false);
        chart->legend()->hide();
        return chart;
    };
    m_huileAdvancedClassChart->setChart(makeEmptyChart());
    m_huileAdvancedTopChart->setChart(makeEmptyChart());
    if (m_huileForecastClassChart) m_huileForecastClassChart->setChart(makeEmptyChart());
    if (m_huileForecastTrendChart) m_huileForecastTrendChart->setChart(makeEmptyChart());

    if (!dbOpen()) {
        if (m_huileAdvancedInsightLabel)
            m_huileAdvancedInsightLabel->setText("La base Oracle n'est pas connectée. Ouvre la connexion puis recharge le scoring avancé.");
        if (m_huileForecastInsightLabel)
            m_huileForecastInsightLabel->setText("La base Oracle n'est pas connectée. Ouvre la connexion puis lance la projection N+1.");
        return;
    }

    const QString idCol = tableHasColumn("QUALITE", "id_lot") ? "id_lot" : (tableHasColumn("QUALITE", "id_qual") ? "id_qual" : QString());
    const QString phCol = tableHasColumn("QUALITE", "ph") ? "ph" : (tableHasColumn("QUALITE", "ph_prod") ? "ph_prod" : QString());
    const QString acidCol = tableHasColumn("QUALITE", "acidite") ? "acidite" : (tableHasColumn("QUALITE", "acid_prod") ? "acid_prod" : QString());
    const QString statusCol = tableHasColumn("QUALITE", "statut_qualite") ? "statut_qualite" : (tableHasColumn("QUALITE", "statut_qual") ? "statut_qual" : QString());

    if (idCol.isEmpty() || phCol.isEmpty() || acidCol.isEmpty()) {
        if (m_huileAdvancedInsightLabel)
            m_huileAdvancedInsightLabel->setText("Impossible de calculer le scoring : colonnes qualité introuvables dans la base.");
        if (m_huileForecastInsightLabel)
            m_huileForecastInsightLabel->setText("Impossible de projeter la qualité N+1 : colonnes qualité introuvables dans la base.");
        return;
    }

    const QString statusExpr = statusCol.isEmpty() ? QString("'EN_ANALYSE'") : statusCol;
    QSqlQuery query;
    const QString sql = QString("SELECT %1 AS lot_id, NVL(%2,0) AS ph_value, NVL(%3,0) AS acid_value, NVL(%4,'EN_ANALYSE') AS statut_value FROM QUALITE ORDER BY %1")
                            .arg(idCol, phCol, acidCol, statusExpr);
    if (!query.exec(sql)) {
        const QString err = QString("Erreur Oracle : %1").arg(query.lastError().text());
        if (m_huileAdvancedInsightLabel) m_huileAdvancedInsightLabel->setText(err);
        if (m_huileForecastInsightLabel) m_huileForecastInsightLabel->setText(err);
        return;
    }

    struct LotScore {
        QString id;
        double ph = 0.0;
        double acid = 0.0;
        QString status;
        double phPenalty = 0.0;
        double acidPenalty = 0.0;
        double score = 0.0;
        QString label;
        QString signal;
        QString recommendation;
        QString formulaText;
        double projectedPh = 0.0;
        double projectedAcid = 0.0;
        double futureScore = 0.0;
        QString futureLabel;
        QString futureTrend;
        QString futureRecommendation;
    };

    QVector<LotScore> rows;
    int greenCount = 0;
    int orangeCount = 0;
    int redCount = 0;
    double scoreSum = 0.0;

    const double phMin = m_huilePhIdealMinSpin ? m_huilePhIdealMinSpin->value() : 4.80;
    const double phMax = m_huilePhIdealMaxSpin ? m_huilePhIdealMaxSpin->value() : 5.60;
    const double phCenter = (phMin + phMax) / 2.0;
    const double phRadius = std::max(0.10, (phMax - phMin) / 2.0);
    const double acidTarget = m_huileAcidIdealMaxSpin ? m_huileAcidIdealMaxSpin->value() : 0.80;
    const double annualPhDrift = m_huileForecastPhDriftSpin ? m_huileForecastPhDriftSpin->value() : 0.12;
    const double annualAcidDrift = m_huileForecastAcidDriftSpin ? m_huileForecastAcidDriftSpin->value() : 0.18;
    const double oxidationStress = m_huileForecastOxidationSpin ? m_huileForecastOxidationSpin->value() : 6.0;
    const double stabilityBonus = m_huileForecastStabilityBonusSpin ? m_huileForecastStabilityBonusSpin->value() : 4.0;

    while (query.next()) {
        LotScore row;
        row.id = query.value("lot_id").toString();
        row.ph = query.value("ph_value").toDouble();
        row.acid = query.value("acid_value").toDouble();
        row.status = query.value("statut_value").toString();
        const HuileScoreBreakdown breakdown = buildHuileScoreBreakdown(
            row.ph,
            row.acid,
            phMin,
            phMax,
            acidTarget,
            m_huilePhWeightSpin ? m_huilePhWeightSpin->value() : 10.0,
            m_huileAcidWeightSpin ? m_huileAcidWeightSpin->value() : 20.0);

        row.phPenalty = breakdown.phPenalty;
        row.acidPenalty = breakdown.acidPenalty;
        row.score = breakdown.score;
        row.label = huileScoreLabel(row.score);
        row.formulaText = QStringLiteral("100 - %1 - %2 = %3")
                              .arg(QString::number(row.phPenalty, 'f', 1),
                                   QString::number(row.acidPenalty, 'f', 1),
                                   QString::number(row.score, 'f', 1));

        if (row.label == QStringLiteral("Vert")) {
            row.signal = QStringLiteral("Bon lot");
            row.recommendation = QStringLiteral("Le pH et l'acidité sont dans une zone rassurante.");
            ++greenCount;
        } else if (row.label == QStringLiteral("Orange")) {
            row.signal = QStringLiteral("À surveiller");
            row.recommendation = QStringLiteral("Le lot reste exploitable mais il faut renforcer le contrôle.");
            ++orangeCount;
        } else {
            row.signal = QStringLiteral("Critique");
            row.recommendation = QStringLiteral("Le lot dépasse les limites fixées et doit être revérifié rapidement.");
            ++redCount;
        }
        scoreSum += row.score;

        const double phDistance = std::max(0.0, std::abs(row.ph - phCenter) - phRadius);
        const double acidOverflow = std::max(0.0, row.acid - acidTarget);
        double oxidationImpact = oxidationStress * (acidOverflow * 0.06 + phDistance * 0.20);
        double stabilityRelief = (row.score >= 85.0 ? stabilityBonus : (row.score >= 70.0 ? stabilityBonus * 0.5 : 0.0));

        row.projectedPh = row.ph - annualPhDrift - phDistance * 0.30 - acidOverflow * 0.10 + stabilityRelief * 0.01;
        row.projectedAcid = row.acid + annualAcidDrift + oxidationImpact * 0.02 - stabilityRelief * 0.02;

        if (row.status.contains("REFUSE", Qt::CaseInsensitive) || row.status.contains("REJET", Qt::CaseInsensitive)) {
            row.projectedAcid += 0.15;
            row.projectedPh -= 0.08;
        } else if (row.status.contains("VALIDE", Qt::CaseInsensitive) || row.status.contains("ACCEP", Qt::CaseInsensitive)) {
            row.projectedAcid = std::max(0.0, row.projectedAcid - 0.05);
        }

        row.projectedPh = std::clamp(row.projectedPh, 0.0, 14.0);
        row.projectedAcid = std::clamp(row.projectedAcid, 0.0, 10.0);
        row.futureScore = computeHuileQualityScore(row.projectedPh, row.projectedAcid);
        row.futureLabel = huileScoreLabel(row.futureScore);
        row.futureTrend = huileForecastTrendLabel(row.score, row.futureScore);

        if (huileScoreLabel(row.futureScore) == QStringLiteral("Vert"))
            row.futureRecommendation = QStringLiteral("Le lot devrait rester stable l'année prochaine.");
        else if (huileScoreLabel(row.futureScore) == QStringLiteral("Orange"))
            row.futureRecommendation = QStringLiteral("Prévoir un contrôle supplémentaire avant la prochaine saison.");
        else
            row.futureRecommendation = QStringLiteral("Prévoir une action corrective rapide si la tendance se confirme.");

        rows.push_back(row);
    }

    const double avgScore = rows.isEmpty() ? 0.0 : scoreSum / rows.size();
    if (m_huileScoreMoyenValue) m_huileScoreMoyenValue->setText(QString::number(avgScore, 'f', 1) + " / 100");
    if (m_huilePremiumLotsValue) m_huilePremiumLotsValue->setText(QString::number(greenCount));
    if (m_huileSurveillanceLotsValue) m_huileSurveillanceLotsValue->setText(QString::number(orangeCount));
    if (m_huileCritiqueLotsValue) m_huileCritiqueLotsValue->setText(QString::number(redCount));

    auto makeItem = [](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    for (const LotScore &row : rows) {
        int r = m_huileAdvancedTable->rowCount();
        m_huileAdvancedTable->insertRow(r);
        m_huileAdvancedTable->setItem(r, 0, makeItem(row.id));
        m_huileAdvancedTable->setItem(r, 1, makeItem(QString::number(row.ph, 'f', 2)));
        m_huileAdvancedTable->setItem(r, 2, makeItem(QString::number(row.acid, 'f', 2)));
        m_huileAdvancedTable->setItem(r, 3, makeItem(QString::number(row.phPenalty, 'f', 1)));
        m_huileAdvancedTable->setItem(r, 4, makeItem(QString::number(row.acidPenalty, 'f', 1)));
        m_huileAdvancedTable->setItem(r, 5, makeItem(QString::number(row.score, 'f', 1)));
        m_huileAdvancedTable->setItem(r, 6, makeItem(row.label));

        QString classe;
        if (row.label == QStringLiteral("Vert"))
            classe = QStringLiteral("Premium");
        else if (row.label == QStringLiteral("Orange"))
            classe = QStringLiteral("A surveiller");
        else
            classe = QStringLiteral("Critique");
        m_huileAdvancedTable->setItem(r, 7, makeItem(classe));

        auto *calc = makeItem(row.formulaText);
        calc->setToolTip(QStringLiteral("Statut actuel : %1\nSignal : %2\n%3").arg(row.status, row.signal, row.recommendation));
        m_huileAdvancedTable->setItem(r, 8, calc);
        applyHuileScoreColorsRow(r, row.score);
    }

    auto *classSeries = new QPieSeries();
    classSeries->setHoleSize(0.45);
    const int greenDisplay = greenCount;
    if (greenDisplay + orangeCount + redCount == 0) {
        auto *slice = classSeries->append("Aucune donnée", 1);
        slice->setBrush(QColor("#dce6ca"));
        slice->setLabelVisible(true);
    } else {
        auto addSlice = [classSeries](const QString &label, int value, const QColor &color) {
            if (value <= 0) return;
            auto *slice = classSeries->append(label + " (" + QString::number(value) + ")", value);
            slice->setBrush(color);
            slice->setPen(QPen(Qt::white, 2));
            slice->setLabelVisible(true);
        };
        addSlice("Vert", greenDisplay, QColor("#66BB6A"));
        addSlice("Orange", orangeCount, QColor("#FFA726"));
        addSlice("Rouge", redCount, QColor("#EF5350"));
    }

    auto *classChart = new QChart();
    classChart->addSeries(classSeries);
    classChart->legend()->setAlignment(Qt::AlignBottom);
    classChart->legend()->setLabelColor(QColor("#415033"));
    classChart->setBackgroundVisible(false);
    classChart->setMargins(QMargins(6, 6, 6, 6));
    classChart->setAnimationOptions(QChart::SeriesAnimations);
    m_huileAdvancedClassChart->setChart(classChart);

    auto sortedCurrent = rows;
    std::sort(sortedCurrent.begin(), sortedCurrent.end(), [](const LotScore &a, const LotScore &b) { return a.score > b.score; });
    auto *scoreSet = new QBarSet("Score");
    scoreSet->setColor(QColor("#8aa14a"));
    QStringList cats;
    for (int i = 0; i < std::min<int>(6, sortedCurrent.size()); ++i) {
        cats << ("Lot " + sortedCurrent[i].id);
        *scoreSet << sortedCurrent[i].score;
    }
    if (cats.isEmpty()) {
        cats << "Aucun lot";
        *scoreSet << 0.0;
    }
    auto *scoreSeries = new QHorizontalBarSeries();
    scoreSeries->append(scoreSet);
    auto *topChart = new QChart();
    topChart->addSeries(scoreSeries);
    topChart->setBackgroundVisible(false);
    topChart->legend()->hide();
    topChart->setMargins(QMargins(10, 10, 10, 10));
    topChart->setAnimationOptions(QChart::SeriesAnimations);
    auto *axisY = new QBarCategoryAxis();
    axisY->append(cats);
    auto *axisX = new QValueAxis();
    axisX->setRange(0, 100);
    axisX->setTitleText("Score");
    axisX->setLabelFormat("%.0f");
    axisX->setGridLineColor(QColor("#e6ecd8"));
    axisX->setLinePenColor(QColor("#74855c"));
    axisX->setLabelsColor(QColor("#556B2F"));
    topChart->addAxis(axisY, Qt::AlignLeft);
    topChart->addAxis(axisX, Qt::AlignBottom);
    scoreSeries->attachAxis(axisY);
    scoreSeries->attachAxis(axisX);
    m_huileAdvancedTopChart->setChart(topChart);

    QString insight;
    if (sortedCurrent.isEmpty()) {
        insight = "Aucun lot qualité n'est disponible pour calculer le score premium.";
    } else {
        const LotScore best = sortedCurrent.first();
        const LotScore worst = sortedCurrent.last();
        insight = QStringLiteral("Le meilleur lot actuel est %1 avec un score de %2/100. ")
                      .arg(best.id, QString::number(best.score, 'f', 1));
        insight += QStringLiteral("Le lot le plus faible est %1 avec %2/100. ")
                       .arg(worst.id, QString::number(worst.score, 'f', 1));
        insight += QStringLiteral("Règle active : score = 100 - pénalité pH - pénalité acidité. ");
        insight += QStringLiteral("Zone pH idéale : [%1 - %2], acidité max sans pénalité : %3. ")
                       .arg(QString::number(m_huilePhIdealMinSpin->value(), 'f', 2),
                            QString::number(m_huilePhIdealMaxSpin->value(), 'f', 2),
                            QString::number(m_huileAcidIdealMaxSpin->value(), 'f', 2));
        insight += QStringLiteral("Résultat : %1 vert(s), %2 orange(s), %3 rouge(s).")
                       .arg(greenCount)
                       .arg(orangeCount)
                       .arg(redCount);
    }
    if (m_huileAdvancedInsightLabel)
        m_huileAdvancedInsightLabel->setText(insight);

    // Prévision qualité N+1
    int futureGreenCount = 0;
    int futureOrangeCount = 0;
    int futureRedCount = 0;
    double futureScoreSum = 0.0;
    for (const LotScore &row : rows) {
        futureScoreSum += row.futureScore;
        const QString futureLabel = huileScoreLabel(row.futureScore);
        if (futureLabel == QStringLiteral("Vert")) ++futureGreenCount;
        else if (futureLabel == QStringLiteral("Orange")) ++futureOrangeCount;
        else ++futureRedCount;
    }
    const double futureAvgScore = rows.isEmpty() ? 0.0 : futureScoreSum / rows.size();
    if (m_huileForecastScoreMoyenValue) m_huileForecastScoreMoyenValue->setText(QString::number(futureAvgScore, 'f', 1) + " / 100");
    if (m_huileForecastPremiumValue) m_huileForecastPremiumValue->setText(QString::number(futureGreenCount));
    if (m_huileForecastVigilanceValue) m_huileForecastVigilanceValue->setText(QString::number(futureOrangeCount));
    if (m_huileForecastCritiqueValue) m_huileForecastCritiqueValue->setText(QString::number(futureRedCount));

    if (m_huileForecastTable) {
        for (const LotScore &row : rows) {
            const int r = m_huileForecastTable->rowCount();
            m_huileForecastTable->insertRow(r);
            m_huileForecastTable->setItem(r, 0, makeItem(row.id));
            m_huileForecastTable->setItem(r, 1, makeItem(QString::number(row.ph, 'f', 2)));
            m_huileForecastTable->setItem(r, 2, makeItem(QString::number(row.acid, 'f', 2)));
            m_huileForecastTable->setItem(r, 3, makeItem(QString::number(row.projectedPh, 'f', 2)));
            m_huileForecastTable->setItem(r, 4, makeItem(QString::number(row.projectedAcid, 'f', 2)));
            m_huileForecastTable->setItem(r, 5, makeItem(QString::number(row.futureScore, 'f', 1)));
            m_huileForecastTable->setItem(r, 6, makeItem(row.futureLabel));
            m_huileForecastTable->setItem(r, 7, makeItem(row.futureTrend));
            auto *rec = makeItem(row.futureRecommendation);
            rec->setToolTip(QStringLiteral("Statut actuel : %1\nTendance : %2\n%3").arg(row.status, row.futureTrend, row.futureRecommendation));
            m_huileForecastTable->setItem(r, 8, rec);
            applyHuileForecastColorsRow(r, row.futureScore);
        }
    }

    if (m_huileForecastClassChart) {
        auto *futureSeries = new QPieSeries();
        futureSeries->setHoleSize(0.45);
        auto addFutureSlice = [futureSeries](const QString &label, int value, const QColor &color) {
            if (value <= 0) return;
            auto *slice = futureSeries->append(label + " (" + QString::number(value) + ")", value);
            slice->setBrush(color);
            slice->setPen(QPen(Qt::white, 2));
            slice->setLabelVisible(true);
        };
        if (futureGreenCount + futureOrangeCount + futureRedCount == 0) {
            auto *slice = futureSeries->append("Aucune donnée", 1);
            slice->setBrush(QColor("#e5e9dc"));
            slice->setLabelVisible(true);
        } else {
            addFutureSlice("Vert", futureGreenCount, QColor("#66BB6A"));
            addFutureSlice("Orange", futureOrangeCount, QColor("#FFA726"));
            addFutureSlice("Rouge", futureRedCount, QColor("#EF5350"));
        }
        auto *futureChart = new QChart();
        futureChart->addSeries(futureSeries);
        futureChart->legend()->setAlignment(Qt::AlignBottom);
        futureChart->legend()->setLabelColor(QColor("#594637"));
        futureChart->setBackgroundVisible(false);
        futureChart->setMargins(QMargins(6, 6, 6, 6));
        futureChart->setAnimationOptions(QChart::SeriesAnimations);
        m_huileForecastClassChart->setChart(futureChart);
    }

    if (m_huileForecastTrendChart) {
        auto sortedFuture = rows;
        std::sort(sortedFuture.begin(), sortedFuture.end(), [](const LotScore &a, const LotScore &b) { return a.futureScore < b.futureScore; });
        auto *currentSet = new QBarSet("Actuel");
        auto *futureSet = new QBarSet("N+1");
        currentSet->setColor(QColor("#9eb36b"));
        futureSet->setColor(QColor("#7c5943"));
        QStringList lotCats;
        const int limit = std::min<int>(6, sortedFuture.size());
        for (int i = 0; i < limit; ++i) {
            lotCats << ("Lot " + sortedFuture[i].id);
            *currentSet << sortedFuture[i].score;
            *futureSet << sortedFuture[i].futureScore;
        }
        if (lotCats.isEmpty()) {
            lotCats << "Aucun lot";
            *currentSet << 0.0;
            *futureSet << 0.0;
        }
        auto *series = new QBarSeries();
        series->append(currentSet);
        series->append(futureSet);
        auto *chart = new QChart();
        chart->addSeries(series);
        chart->setBackgroundVisible(false);
        chart->setAnimationOptions(QChart::SeriesAnimations);
        chart->legend()->setAlignment(Qt::AlignBottom);
        chart->legend()->setLabelColor(QColor("#594637"));
        auto *axisXFuture = new QBarCategoryAxis();
        axisXFuture->append(lotCats);
        auto *axisYFuture = new QValueAxis();
        axisYFuture->setRange(0, 100);
        axisYFuture->setTitleText("Score qualité");
        axisYFuture->setLabelFormat("%.0f");
        axisYFuture->setGridLineColor(QColor("#ece5dc"));
        axisYFuture->setLinePenColor(QColor("#7c5943"));
        axisYFuture->setLabelsColor(QColor("#6b4f3f"));
        chart->addAxis(axisXFuture, Qt::AlignBottom);
        chart->addAxis(axisYFuture, Qt::AlignLeft);
        series->attachAxis(axisXFuture);
        series->attachAxis(axisYFuture);
        m_huileForecastTrendChart->setChart(chart);
    }

    QString futureInsight;
    if (rows.isEmpty()) {
        futureInsight = "Aucun lot n'est disponible pour projeter la qualité de l'année prochaine.";
    } else {
        auto bestFuture = std::max_element(rows.begin(), rows.end(), [](const LotScore &a, const LotScore &b) { return a.futureScore < b.futureScore; });
        auto worstFuture = std::min_element(rows.begin(), rows.end(), [](const LotScore &a, const LotScore &b) { return a.futureScore < b.futureScore; });
        futureInsight = QString("Projection N+1 : le lot %1 conserverait le meilleur potentiel avec %2/100, tandis que le lot %3 tomberait à %4/100 si la dérive chimique actuelle se maintient. ")
                            .arg(bestFuture->id,
                                 QString::number(bestFuture->futureScore, 'f', 1),
                                 worstFuture->id,
                                 QString::number(worstFuture->futureScore, 'f', 1));
        futureInsight += QString("Paramètres actifs : dérive pH %1, hausse d'acidité %2, stress d'oxydation %3, bonus stabilité %4.")
                            .arg(QString::number(annualPhDrift, 'f', 2),
                                 QString::number(annualAcidDrift, 'f', 2),
                                 QString::number(oxidationStress, 'f', 2),
                                 QString::number(stabilityBonus, 'f', 2));
    }
    if (m_huileForecastInsightLabel)
        m_huileForecastInsightLabel->setText(futureInsight);
}

// ════════════════════════════════════════════════════════════════════════════
//  HUILE - TRI ET RECHARGEMENT
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::on_trihuileButton_clicked()
{
    qDebug() << "Bouton tri cliqué";

    QSqlQuery query;
    query.prepare("SELECT id_lot, responsable_controle, date_production, "
                  "quantite_produite, ph, acidite, amerture, statut_qualite, "
                  "id_stock, code_couleur "
                  "FROM QUALITE ORDER BY date_production DESC");

    ui->tableWidget_4->setRowCount(0);

    if (!query.exec()) {
        QMessageBox::warning(this, "Erreur", "Erreur lors du tri: " + query.lastError().text());
        return;
    }

    int row = 0;
    while (query.next()) {
        ui->tableWidget_4->insertRow(row);
        ui->tableWidget_4->setItem(row, 0, new QTableWidgetItem(query.value("id_lot").toString()));
        ui->tableWidget_4->setItem(row, 1, new QTableWidgetItem(query.value("responsable_controle").toString()));
        ui->tableWidget_4->setItem(row, 2, new QTableWidgetItem(query.value("date_production").toString()));
        ui->tableWidget_4->setItem(row, 3, new QTableWidgetItem(query.value("quantite_produite").toString()));
        ui->tableWidget_4->setItem(row, 4, new QTableWidgetItem(query.value("ph").toString()));
        ui->tableWidget_4->setItem(row, 5, new QTableWidgetItem(query.value("acidite").toString()));
        ui->tableWidget_4->setItem(row, 6, new QTableWidgetItem(query.value("amerture").toString()));
        ui->tableWidget_4->setItem(row, 7, new QTableWidgetItem(query.value("statut_qualite").toString()));
        ui->tableWidget_4->setItem(row, 8, new QTableWidgetItem(query.value("id_stock").toString()));
        ui->tableWidget_4->setItem(row, 9, new QTableWidgetItem(query.value("code_couleur").toString()));
        addHuileActionButtonsToRow(row);
        row++;
    }

    QMessageBox::information(this, "Tri", QString("%1 lignes triées par date décroissante").arg(row));
}

void MainWindow::on_reloadhuilebutton_clicked()
{
    qDebug() << "Bouton rechargement cliqué";

    QSqlQuery query;
    query.prepare("SELECT id_lot, responsable_controle, date_production, "
                  "quantite_produite, ph, acidite, amerture, statut_qualite, "
                  "id_stock, code_couleur "
                  "FROM QUALITE ORDER BY id_lot");

    ui->tableWidget_4->setRowCount(0);

    if (!query.exec()) {
        QMessageBox::warning(this, "Erreur", "Erreur lors du rechargement: " + query.lastError().text());
        return;
    }

    int row = 0;
    while (query.next()) {
        ui->tableWidget_4->insertRow(row);
        ui->tableWidget_4->setItem(row, 0, new QTableWidgetItem(query.value("id_lot").toString()));
        ui->tableWidget_4->setItem(row, 1, new QTableWidgetItem(query.value("responsable_controle").toString()));
        ui->tableWidget_4->setItem(row, 2, new QTableWidgetItem(query.value("date_production").toString()));
        ui->tableWidget_4->setItem(row, 3, new QTableWidgetItem(query.value("quantite_produite").toString()));
        ui->tableWidget_4->setItem(row, 4, new QTableWidgetItem(query.value("ph").toString()));
        ui->tableWidget_4->setItem(row, 5, new QTableWidgetItem(query.value("acidite").toString()));
        ui->tableWidget_4->setItem(row, 6, new QTableWidgetItem(query.value("amerture").toString()));
        ui->tableWidget_4->setItem(row, 7, new QTableWidgetItem(query.value("statut_qualite").toString()));
        ui->tableWidget_4->setItem(row, 8, new QTableWidgetItem(query.value("id_stock").toString()));
        ui->tableWidget_4->setItem(row, 9, new QTableWidgetItem(query.value("code_couleur").toString()));
        addHuileActionButtonsToRow(row);
        row++;
    }

    QMessageBox::information(this, "Actualisation", QString("%1 lignes chargées").arg(row));
}

// ════════════════════════════════════════════════════════════════════════════
//  EVENT FILTER
// ════════════════════════════════════════════════════════════════════════════

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui->codecouleurLineEdit && event->type() == QEvent::MouseButtonPress) {
        QColor initial = QColor(ui->codecouleurLineEdit->text());
        if (!initial.isValid()) initial = Qt::white;

        QColor color = QColorDialog::getColor(initial, this, "Choisissez une couleur");
        if (color.isValid()) {
            QString hex = color.name();
            ui->codecouleurLineEdit->setText(hex);
            ui->codecouleurLineEdit->setStyleSheet(
                QString("background-color: %1; color: %2; border: 2px solid #ccc; border-radius: 4px;")
                    .arg(hex)
                    .arg(color.lightness() > 128 ? "#000000" : "#ffffff"));
        }
        return true;
    }
    return QMainWindow::eventFilter(obj, event);
}
void MainWindow::on_btnAdvHuile_clicked()
{
    if (ui->metiershuile) {
        ui->metiershuile->setCurrentIndex(3);
        chargerAnalyseAvanceeHuile();
    }
}


// ════════════════════════════════════════════════════════════════════════════
//  MACHINES — INTÉGRATION
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::setupModule5()
{
    ensureMachineTopBarVisible();
    ensureSerieComboInMachineForm();
    ensureMachineExtraFields();
    ensureMachineSearchModes();
    ensureSerieUiInAdvanced();
    ensureMachineTableColumns();
    setupMachineHeaderSorting();
    setupValidatorsModule5();

    if (auto *lbl = findChild<QLabel*>("statusmachineLabel")) {
        lbl->setText("Etat machine");
    }
    if (auto *lbl = findChild<QLabel*>("fonctionmachinelabel")) {
        lbl->setText("Type de machine");
    }
    if (auto *lbl = findChild<QLabel*>("datefonctmachinelabel")) {
        lbl->setText("Date d'installation");
    }

    if (ui->metierspersonnel_2 && ui->consulterpersonnel_3)
        ui->metierspersonnel_2->setCurrentWidget(ui->consulterpersonnel_3);

    if (btnRefreshAdvanced) {
        connect(btnRefreshAdvanced, &QPushButton::clicked, this, [this]() {
            refreshAdvancedAnalytics();
        });
    }
    if (btnApplyRiskFormula) {
        connect(btnApplyRiskFormula, &QPushButton::clicked, this, [this]() {
            refreshAdvancedAnalytics();
        });
    }
    if (btnExportAdvanced) {
        connect(btnExportAdvanced, &QPushButton::clicked, this, [this]() {
            exportTableToCsv(advMachineTable, "analyse_machines_avancee.csv");
        });
    }
    if (btnCorrectiveAdvanced) {
        connect(btnCorrectiveAdvanced, &QPushButton::clicked, this, [this]() {
            showMachineCorrectiveDialog();
        });
    }
    if (btnRefreshMachinePorts) {
        connect(btnRefreshMachinePorts, &QPushButton::clicked, this, &MainWindow::refreshMachineSerialPorts);
    }
    if (btnConnectMachineSensor) {
        connect(btnConnectMachineSensor, &QPushButton::clicked, this, &MainWindow::connectMachineSensor);
    }
    if (btnDisconnectMachineSensor) {
        connect(btnDisconnectMachineSensor, &QPushButton::clicked, this, &MainWindow::disconnectMachineSensor);
    }

    refreshMachineSerialPorts();

    if (dbOpen()) {
        fillSeriesCombo();
        loadMachines();
        updateMachineCharts();
        refreshAdvancedAnalytics();
        loadMachineSensorTargets();
    }
}


QString MainWindow::inferSensorState(double temperature) const
{
    if (temperature >= 40.0) return "DANGER";
    if (temperature >= 30.0) return "ALERTE";
    return "NORMAL";
}

void MainWindow::updateMachineSensorUiState(const QString& statusText, const QString& color)
{
    if (!machineSensorStatusLabel) return;

    const QString safeColor = color.isEmpty() ? QString("#556B2F") : color;
    machineSensorStatusLabel->setText(statusText);
    machineSensorStatusLabel->setStyleSheet(QString(
        "font-weight:600; color:%1; background:white; border:1px solid %1; border-radius:8px; padding:8px;"
    ).arg(safeColor));
}

void MainWindow::refreshMachineSerialPorts()
{
    if (!machineSensorPortCombo) return;

    const QString currentPort = machineSensorPortCombo->currentData().toString();
    machineSensorPortCombo->clear();

    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        const QString label = info.portName() + (info.description().isEmpty() ? QString() : QString(" - ") + info.description());
        machineSensorPortCombo->addItem(label, info.portName());
    }

    if (machineSensorPortCombo->count() == 0) {
        machineSensorPortCombo->addItem("Aucun port détecté", QString());
        updateMachineSensorUiState("Aucun port série détecté. Branche l'Arduino puis clique sur Actualiser les ports.", "#8a6d1f");
        return;
    }

    int idx = machineSensorPortCombo->findData(currentPort);
    machineSensorPortCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}

void MainWindow::loadMachineSensorTargets()
{
    if (!machineSensorTargetCombo || !dbOpen()) return;

    const QVariant previousId = machineSensorTargetCombo->currentData();
    machineSensorTargetCombo->clear();

    QSqlQuery q;
    if (!q.exec("SELECT id_machine, nom_machine, NVL(type_machine,'-') FROM MACHINE ORDER BY id_machine")) {
        updateMachineSensorUiState("Impossible de charger la liste des machines pour le capteur.", "#a94442");
        return;
    }

    while (q.next()) {
        const int id = q.value(0).toInt();
        const QString name = q.value(1).toString().trimmed();
        const QString type = q.value(2).toString().trimmed();
        machineSensorTargetCombo->addItem(QString("#%1 - %2 (%3)").arg(id).arg(name.isEmpty() ? "Machine" : name, type.isEmpty() ? "-" : type), id);
    }

    int idx = machineSensorTargetCombo->findData(previousId);
    if (idx < 0 && linkedMachineId > 0)
        idx = machineSensorTargetCombo->findData(linkedMachineId);
    machineSensorTargetCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}

void MainWindow::connectMachineSensor()
{
    if (!machineSerial || !machineSensorTargetCombo || !machineSensorPortCombo)
        return;

    const int machineId = machineSensorTargetCombo->currentData().toInt();
    const QString portName = machineSensorPortCombo->currentData().toString().trimmed();

    if (machineId <= 0) {
        updateMachineSensorUiState("Choisis d'abord une machine cible dans le module Machine.", "#a94442");
        return;
    }
    if (portName.isEmpty()) {
        updateMachineSensorUiState("Choisis un port série valide avant de connecter le capteur.", "#a94442");
        return;
    }

    if (machineSerial->isOpen())
        machineSerial->close();

    machineSerialBuffer.clear();
    machineSerial->setPortName(portName);
    machineSerial->setBaudRate(QSerialPort::Baud9600);
    machineSerial->setDataBits(QSerialPort::Data8);
    machineSerial->setParity(QSerialPort::NoParity);
    machineSerial->setStopBits(QSerialPort::OneStop);
    machineSerial->setFlowControl(QSerialPort::NoFlowControl);

    if (!machineSerial->open(QIODevice::ReadOnly)) {
        updateMachineSensorUiState(QString("Échec de connexion au port %1.").arg(portName), "#a94442");
        return;
    }

    linkedMachineId = machineId;
    updateMachineSensorUiState(QString("Capteur connecté sur %1 pour %2. Les températures reçues mettront à jour directement la table MACHINE.").arg(portName, machineSensorTargetCombo->currentText()), "#2f6f3e");
    if (machineSensorLastValueLabel)
        machineSensorLastValueLabel->setText("Dernière lecture : en attente de données Arduino...");
}

void MainWindow::disconnectMachineSensor()
{
    if (machineSerial && machineSerial->isOpen())
        machineSerial->close();
    machineSerialBuffer.clear();
    linkedMachineId = -1;
    updateMachineSensorUiState("Capteur déconnecté. Aucune mise à jour automatique en cours.", "#8a6d1f");
    if (machineSensorLastValueLabel)
        machineSensorLastValueLabel->setText("Dernière lecture : --");
}

void MainWindow::applyTemperatureToMachine(int machineId, double temperature, const QString& state)
{
    if (!dbOpen() || machineId <= 0)
        return;

    const QString dbState = (state == "DANGER") ? "MAINTENANCE" : "ACTIVE";

    QSqlQuery q;
    q.prepare("UPDATE MACHINE SET temperature_actuelle = :temp, etat_machine = :etat WHERE id_machine = :id");
    q.bindValue(":temp", temperature);
    q.bindValue(":etat", dbState);
    q.bindValue(":id", machineId);

    if (!q.exec()) {
        updateMachineSensorUiState(QString("Température reçue mais la mise à jour Oracle a échoué pour la machine #%1.").arg(machineId), "#a94442");
        return;
    }

    if (machineSensorLastValueLabel) {
        machineSensorLastValueLabel->setText(QString("Dernière lecture : %1 °C | état capteur : %2 | machine #%3 mise à jour à %4")
                                             .arg(QString::number(temperature, 'f', 1), state, QString::number(machineId), QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss")));
    }

    const QString color = (state == "DANGER") ? "#a94442" : (state == "ALERTE" ? "#b26a00" : "#2f6f3e");
    updateMachineSensorUiState(QString("Machine #%1 synchronisée : %2 °C (%3)").arg(machineId).arg(QString::number(temperature, 'f', 1), state), color);

    loadMachines(lastMachineWhereSql, lastMachineBinds);
    updateMachineCharts();
    refreshAdvancedAnalytics();
    loadMachineSensorTargets();
}

void MainWindow::readMachineSerialData()
{
    if (!machineSerial)
        return;

    machineSerialBuffer.append(machineSerial->readAll());

    while (true) {
        const int newlineIndex = machineSerialBuffer.indexOf('\n');
        if (newlineIndex < 0)
            break;

        QByteArray rawLine = machineSerialBuffer.left(newlineIndex);
        machineSerialBuffer.remove(0, newlineIndex + 1);
        const QString line = QString::fromUtf8(rawLine).trimmed();
        if (line.isEmpty())
            continue;

        double temperature = std::numeric_limits<double>::quiet_NaN();
        QString state;

        if (line.startsWith("TEMP:", Qt::CaseInsensitive)) {
            const QStringList parts = line.split(';', Qt::SkipEmptyParts);
            if (!parts.isEmpty()) {
                const QString tempValue = parts[0].section(':', 1, 1).trimmed();
                if (tempValue.compare("ERR", Qt::CaseInsensitive) == 0)
                    continue;
                temperature = tempValue.toDouble();
            }
            for (const QString &part : parts) {
                if (part.startsWith("STATE:", Qt::CaseInsensitive)) {
                    state = part.section(':', 1, 1).trimmed().toUpper();
                    break;
                }
            }
        } else if (line.startsWith("Température", Qt::CaseInsensitive) || line.startsWith("Temperature", Qt::CaseInsensitive)) {
            QRegularExpression re("([-+]?[0-9]*\\.?[0-9]+)");
            const QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch())
                temperature = match.captured(1).toDouble();
        } else {
            continue;
        }

        if (std::isnan(temperature))
            continue;
        if (state.isEmpty())
            state = inferSensorState(temperature);

        const int targetMachineId = (machineSensorTargetCombo && machineSensorTargetCombo->currentData().toInt() > 0)
                                    ? machineSensorTargetCombo->currentData().toInt()
                                    : linkedMachineId;
        if (targetMachineId <= 0) {
            updateMachineSensorUiState("Température reçue mais aucune machine cible n'est sélectionnée.", "#a94442");
            continue;
        }

        linkedMachineId = targetMachineId;
        applyTemperatureToMachine(targetMachineId, temperature, state);
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

void MainWindow::ensureMachineExtraFields()
{
    if (!ui->ajoutpersonnel_3) return;

    auto *formHost = ui->ajoutpersonnel_3->findChild<QWidget*>("formLayoutWidget_6");
    auto *form = ui->ajoutpersonnel_3->findChild<QFormLayout*>("formLayout_6");
    if (!formHost || !form) return;

    form->setContentsMargins(8, 8, 8, 8);
    form->setHorizontalSpacing(18);
    form->setVerticalSpacing(16);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setFormAlignment(Qt::AlignTop | Qt::AlignLeft);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    formHost->setMinimumWidth(380);
    formHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    if (auto *lbl = ui->ajoutpersonnel_3->findChild<QLabel*>("nommachinelabel"))
        lbl->setText("Nom de la machine");
    if (auto *lbl = ui->ajoutpersonnel_3->findChild<QLabel*>("fonctionmachinelabel"))
        lbl->setText("Type de machine");
    if (auto *lbl = ui->ajoutpersonnel_3->findChild<QLabel*>("datefonctmachinelabel"))
        lbl->setText("Date d'installation");
    if (auto *lbl = ui->ajoutpersonnel_3->findChild<QLabel*>("statusmachineLabel"))
        lbl->setText("Etat de la machine");

    auto styleField = [](QWidget *w) {
        if (!w) return;
        w->setMinimumHeight(40);
        w->setMinimumWidth(230);
        w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    };

    for (auto *lbl : formHost->findChildren<QLabel*>())
        lbl->setMinimumWidth(165);

    for (auto *le : formHost->findChildren<QLineEdit*>())
        styleField(le);
    for (auto *cb : formHost->findChildren<QComboBox*>())
        styleField(cb);
    for (auto *de : formHost->findChildren<QDateEdit*>())
        styleField(de);

    if (!findMachineHoursEdit()) {
        auto *hours = new QLineEdit(ui->ajoutpersonnel_3);
        hours->setObjectName("heuresmachine");
        hours->setPlaceholderText("Heures de fonctionnement");
        styleField(hours);
        form->insertRow(4, new QLabel("Heures de fonctionnement", ui->ajoutpersonnel_3), hours);
    }

    if (!findMachineTempEdit()) {
        auto *temp = new QLineEdit(ui->ajoutpersonnel_3);
        temp->setObjectName("temperaturemachine");
        temp->setPlaceholderText("Temperature actuelle (C)");
        styleField(temp);
        form->insertRow(5, new QLabel("Temperature actuelle", ui->ajoutpersonnel_3), temp);
    }

    if (cbSerieMachine) {
        styleField(cbSerieMachine);
        if (cbSerieMachine->count() == 0)
            cbSerieMachine->addItem("Aucune serie");
    }

    if (auto *name = findChild<QLineEdit*>("nommachine"))
        name->setPlaceholderText("Nom de la machine");

    if (auto *btn = ui->ajoutpersonnel_3->findChild<QPushButton*>("ajoutermachine")) {
        btn->setText(editMachineId == -1 ? "Ajouter" : "Enregistrer");
        btn->setMinimumHeight(42);
        btn->setMinimumWidth(150);
        btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    if (auto *cbEtat = findMachineEtatCombo()) {
        QString current = cbEtat->currentText().trimmed();
        cbEtat->clear();
        cbEtat->addItems({"Actif", "Maintenance", "Panne"});
        if (current.compare("Non actif", Qt::CaseInsensitive) == 0)
            current = "Panne";
        int idx = cbEtat->findText(current);
        cbEtat->setCurrentIndex(idx >= 0 ? idx : 0);
        styleField(cbEtat);
    }
}

void MainWindow::ensureMachineSearchModes()
{
    if (ui->datmachinne) {
        const QString current = ui->datmachinne->currentText();
        ui->datmachinne->clear();
        ui->datmachinne->addItems({"ID", "Nom", "Type", "Etat", "Date"});
        int idx = ui->datmachinne->findText(current);
        ui->datmachinne->setCurrentIndex(idx >= 0 ? idx : 1);
    }

    if (ui->trie) {
        const QString current = ui->trie->currentText().trimmed().toLower();
        ui->trie->clear();
        ui->trie->addItems({"id", "nom", "datefonctionnement", "etat", "type", "heures", "temperature"});
        int idx = ui->trie->findText(current, Qt::MatchFixedString);
        ui->trie->setCurrentIndex(idx >= 0 ? idx : 0);
    }
}

void MainWindow::setupValidatorsModule5()
{
    if (auto *cap = findChild<QLineEdit*>("capacitelineprod")) {
        auto *v = new QDoubleValidator(0, 1e12, 2, cap);
        v->setNotation(QDoubleValidator::StandardNotation);
        cap->setValidator(v);
        cap->setPlaceholderText("Ex: 1500");
    }

    if (auto *hours = findMachineHoursEdit()) {
        auto *v = new QDoubleValidator(0, 1e12, 2, hours);
        v->setNotation(QDoubleValidator::StandardNotation);
        hours->setValidator(v);
    }

    if (auto *temp = findMachineTempEdit()) {
        auto *v = new QDoubleValidator(-20, 200, 2, temp);
        v->setNotation(QDoubleValidator::StandardNotation);
        temp->setValidator(v);
    }

    auto trimOnEditFinished = [](QLineEdit* le){
        if (!le) return;
        QObject::connect(le, &QLineEdit::editingFinished, le, [le](){
            le->setText(le->text().trimmed());
        });
    };

    trimOnEditFinished(findChild<QLineEdit*>("nommachine"));
    trimOnEditFinished(findChild<QLineEdit*>("nomserielinemachine_2"));
    trimOnEditFinished(findMachineHoursEdit());
    trimOnEditFinished(findMachineTempEdit());

    if (auto *d = findChild<QDateEdit*>("datefonctioserienmachine_2")) {
        d->setCalendarPopup(true);
        if (!d->date().isValid()) d->setDate(QDate::currentDate());
    }
    if (auto *d = findChild<QDateEdit*>("datefonctionmachine")) {
        d->setCalendarPopup(true);
        if (!d->date().isValid()) d->setDate(QDate::currentDate());
    }
}

void MainWindow::ensureSerieComboInMachineForm()
{
    if (!ui->ajoutpersonnel_3) return;

    auto *page = ui->ajoutpersonnel_3;
    auto *formHost = page->findChild<QWidget*>("formLayoutWidget_6");
    auto *form = page->findChild<QFormLayout*>("formLayout_6");
    if (!formHost || !form) return;

    cbSerieMachine = page->findChild<QComboBox*>("cbSerieMachine");
    if (!cbSerieMachine) {
        cbSerieMachine = new QComboBox(page);
        cbSerieMachine->setObjectName("cbSerieMachine");
        int insertRow = qMax(4, form->rowCount() - 1);
        form->insertRow(insertRow, new QLabel("Serie machine", page), cbSerieMachine);
    }

    auto *box = page->findChild<QGroupBox*>("serieMachineBox");
    if (!box) {
        box = new QGroupBox("Ajouter une serie", page);
        box->setObjectName("serieMachineBox");

        auto *lay = new QFormLayout(box);
        lay->setContentsMargins(22, 24, 22, 20);
        lay->setHorizontalSpacing(18);
        lay->setVerticalSpacing(18);
        lay->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        lay->setFormAlignment(Qt::AlignTop | Qt::AlignLeft);
        lay->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        lay->setRowWrapPolicy(QFormLayout::DontWrapRows);

        auto makeField = [](QWidget *w) {
            if (!w) return;
            w->setMinimumHeight(44);
            w->setMinimumWidth(220);
            w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        };

        auto makeLabel = [box](const QString &text) {
            auto *lbl = new QLabel(text, box);
            lbl->setMinimumWidth(120);
            lbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            lbl->setWordWrap(true);
            lbl->setStyleSheet("font-size:14px; color:#22311a;");
            return lbl;
        };

        auto *nom = new QLineEdit(box);
        nom->setObjectName("nomserielinemachine_2");
        nom->setPlaceholderText("Nom de la serie");
        makeField(nom);
        lay->addRow(makeLabel("Nom serie"), nom);

        auto *cap = new QLineEdit(box);
        cap->setObjectName("capacitelineprod");
        cap->setPlaceholderText("Ex: 1500");
        makeField(cap);
        lay->addRow(makeLabel("Capacite"), cap);

        auto *date = new QDateEdit(box);
        date->setObjectName("datefonctioserienmachine_2");
        date->setCalendarPopup(true);
        date->setDate(QDate::currentDate());
        makeField(date);
        lay->addRow(makeLabel("Date service"), date);

        auto *etat = new QComboBox(box);
        etat->setObjectName("statulineseriesmachine_2");
        etat->addItems({"Actif", "Non actif"});
        makeField(etat);
        lay->addRow(makeLabel("Etat"), etat);

        auto *btn = new QPushButton("Ajouter serie", box);
        btn->setObjectName("ajouterlineseriemachine_2");
        btn->setMinimumHeight(46);
        btn->setMinimumWidth(210);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        lay->addRow(makeLabel(""), btn);
        connect(btn, &QPushButton::clicked, this, &MainWindow::on_ajouterlineseriemachine_2_clicked);
    }

    auto *leftBox = page->findChild<QGroupBox*>("machineAddFormBox");
    if (!leftBox) {
        leftBox = new QGroupBox("Ajouter une machine", page);
        leftBox->setObjectName("machineAddFormBox");
        auto *leftLay = new QVBoxLayout(leftBox);
        leftLay->setContentsMargins(16, 22, 16, 18);
        leftLay->setSpacing(10);
        leftLay->addWidget(formHost);
    } else if (auto *leftLay = qobject_cast<QVBoxLayout*>(leftBox->layout())) {
        if (leftLay->indexOf(formHost) == -1)
            leftLay->addWidget(formHost);
    }

    auto *separator = page->findChild<QFrame*>("machineAddSeparator");
    if (!separator) {
        separator = new QFrame(page);
        separator->setObjectName("machineAddSeparator");
        separator->setFrameShape(QFrame::VLine);
        separator->setFrameShadow(QFrame::Sunken);
        separator->setLineWidth(1);
        separator->setMidLineWidth(0);
        separator->setMinimumWidth(18);
        separator->setMaximumWidth(18);
        separator->setStyleSheet("QFrame#machineAddSeparator { color: #D7DEC5; }");
    }

    box->setMinimumWidth(430);
    box->setMaximumWidth(470);
    box->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    box->setStyleSheet("QGroupBox#serieMachineBox { margin-top: 18px; padding-top: 8px; }");

    leftBox->setMinimumWidth(510);
    leftBox->setMaximumWidth(560);
    leftBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    leftBox->setStyleSheet("QGroupBox#machineAddFormBox { margin-top: 18px; padding-top: 8px; }");

    formHost->setMinimumWidth(450);
    formHost->setMaximumWidth(520);
    formHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QHBoxLayout *mainLayout = qobject_cast<QHBoxLayout*>(page->layout());
    if (!mainLayout) {
        mainLayout = new QHBoxLayout(page);
        mainLayout->setObjectName("machineAddPageLayout");
    }

    while (mainLayout->count() > 0) {
        QLayoutItem *item = mainLayout->takeAt(0);
        delete item;
    }

    mainLayout->setContentsMargins(24, 18, 44, 20);
    mainLayout->setSpacing(36);
    mainLayout->addWidget(leftBox, 0, Qt::AlignTop);
    mainLayout->addWidget(separator, 0, Qt::AlignTop);
    mainLayout->addWidget(box, 0, Qt::AlignTop);
    mainLayout->addStretch(1);
}


void MainWindow::ensureSerieUiInAdvanced()
{
    if (!ui->metieravancee_4) return;

    auto *page = ui->metieravancee_4;
    auto *scroll = page->findChild<QScrollArea*>("machineAdvancedScroll", Qt::FindDirectChildrenOnly);
    if (!scroll) {
        if (auto *oldLayout = page->layout()) {
            QLayoutItem *item;
            while ((item = oldLayout->takeAt(0)) != nullptr) {
                if (item->widget()) item->widget()->deleteLater();
                delete item;
            }
            delete oldLayout;
        }
        const auto children = page->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (auto *child : children) {
            if (child) child->deleteLater();
        }

        auto *pageLayout = new QVBoxLayout(page);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setSpacing(0);

        scroll = new QScrollArea(page);
        scroll->setObjectName("machineAdvancedScroll");
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        pageLayout->addWidget(scroll);

        auto *container = new QWidget(scroll);
        container->setObjectName("machineAdvancedContainer");
        scroll->setWidget(container);

        auto *root = new QVBoxLayout(container);
        root->setContentsMargins(20, 18, 20, 24);
        root->setSpacing(16);

        auto *title = new QLabel("Métiers avancés : maintenance prédictive et analyse de performance", container);
        title->setWordWrap(true);
        title->setStyleSheet("font-size:16px; font-weight:700; color:#22311a;");
        root->addWidget(title);

        auto *sensorBox = new QGroupBox("Connexion capteur Arduino / DHT22", container);
        sensorBox->setObjectName("machineSensorBox");
        auto *sensorRoot = new QVBoxLayout(sensorBox);
        sensorRoot->setContentsMargins(14, 18, 14, 14);
        sensorRoot->setSpacing(12);

        auto *sensorHelp = new QLabel("Choisis une machine, sélectionne le port COM de l'Arduino puis clique sur Connecter. Chaque température reçue met à jour directement la colonne temperature_actuelle de la table MACHINE.", sensorBox);
        sensorHelp->setWordWrap(true);
        sensorHelp->setStyleSheet("color:#5c624d;");
        sensorRoot->addWidget(sensorHelp);

        auto *sensorGrid = new QGridLayout();
        sensorGrid->setHorizontalSpacing(14);
        sensorGrid->setVerticalSpacing(10);

        auto makeFieldTitle = [sensorBox](const QString &txt) {
            auto *lbl = new QLabel(txt, sensorBox);
            lbl->setStyleSheet("font-weight:600; color:#33411f;");
            return lbl;
        };

        machineSensorTargetCombo = new QComboBox(sensorBox);
        machineSensorTargetCombo->setObjectName("machineSensorTargetCombo");
        machineSensorTargetCombo->setMinimumHeight(36);
        machineSensorTargetCombo->setMinimumWidth(220);
        machineSensorPortCombo = new QComboBox(sensorBox);
        machineSensorPortCombo->setObjectName("machineSensorPortCombo");
        machineSensorPortCombo->setMinimumHeight(36);
        machineSensorPortCombo->setMinimumWidth(180);

        sensorGrid->addWidget(makeFieldTitle("Machine cible"), 0, 0);
        sensorGrid->addWidget(machineSensorTargetCombo, 0, 1);
        sensorGrid->addWidget(makeFieldTitle("Port série"), 0, 2);
        sensorGrid->addWidget(machineSensorPortCombo, 0, 3);

        btnRefreshMachinePorts = new QPushButton("Actualiser les ports", sensorBox);
        btnRefreshMachinePorts->setObjectName("btnRefreshMachinePorts");
        btnRefreshMachinePorts->setMinimumHeight(36);
        btnConnectMachineSensor = new QPushButton("Connecter le capteur", sensorBox);
        btnConnectMachineSensor->setObjectName("btnConnectMachineSensor");
        btnConnectMachineSensor->setMinimumHeight(36);
        btnDisconnectMachineSensor = new QPushButton("Déconnecter", sensorBox);
        btnDisconnectMachineSensor->setObjectName("btnDisconnectMachineSensor");
        btnDisconnectMachineSensor->setMinimumHeight(36);

        sensorGrid->addWidget(btnRefreshMachinePorts, 1, 1);
        sensorGrid->addWidget(btnConnectMachineSensor, 1, 2);
        sensorGrid->addWidget(btnDisconnectMachineSensor, 1, 3);

        sensorRoot->addLayout(sensorGrid);

        machineSensorStatusLabel = new QLabel("Capteur non connecté", sensorBox);
        machineSensorStatusLabel->setObjectName("machineSensorStatusLabel");
        machineSensorStatusLabel->setWordWrap(true);
        machineSensorStatusLabel->setStyleSheet("font-weight:600; color:#8a6d1f; background:#fff7e3; border:1px solid #eed79a; border-radius:8px; padding:8px;");
        sensorRoot->addWidget(machineSensorStatusLabel);

        machineSensorLastValueLabel = new QLabel("Dernière lecture : --", sensorBox);
        machineSensorLastValueLabel->setObjectName("machineSensorLastValueLabel");
        machineSensorLastValueLabel->setWordWrap(true);
        machineSensorLastValueLabel->setStyleSheet("color:#4f5a3c; background:#f7f9f1; border:1px solid #d9dfc8; border-radius:8px; padding:8px;");
        sensorRoot->addWidget(machineSensorLastValueLabel);

        root->addWidget(sensorBox);

        auto makeValue = [container](const QString &objectName) {
            auto *lbl = new QLabel("0", container);
            lbl->setObjectName(objectName);
            lbl->setAlignment(Qt::AlignCenter);
            lbl->setMinimumHeight(54);
            lbl->setStyleSheet("font-size:20px; font-weight:700; background:#f5f7ef; border:1px solid #d9dfc8; border-radius:10px; padding:8px;");
            return lbl;
        };

        auto *kpiBox = new QGroupBox("Résumé intelligent", container);
        auto *kpiGrid = new QGridLayout(kpiBox);
        kpiGrid->setContentsMargins(14, 18, 14, 14);
        kpiGrid->setHorizontalSpacing(14);
        kpiGrid->setVerticalSpacing(8);
        for (int i = 0; i < 4; ++i)
            kpiGrid->setColumnStretch(i, 1);

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

        auto *formulaBox = new QGroupBox("Formule modifiable du risque", container);
        auto *formulaRoot = new QVBoxLayout(formulaBox);
        formulaRoot->setContentsMargins(14, 18, 14, 14);
        formulaRoot->setSpacing(12);

        auto *formulaHelp = new QLabel("Modifie les paramètres puis clique sur “Appliquer la formule” pour recalculer le risque et recolorer automatiquement chaque machine.", formulaBox);
        formulaHelp->setWordWrap(true);
        formulaHelp->setStyleSheet("color:#5c624d;");
        formulaRoot->addWidget(formulaHelp);

        auto makeDoubleSpin = [formulaBox](const QString &objectName, double min, double max, double value, double step, const QString &suffix = QString(), int decimals = 1) {
            auto *spin = new QDoubleSpinBox(formulaBox);
            spin->setObjectName(objectName);
            spin->setRange(min, max);
            spin->setDecimals(decimals);
            spin->setSingleStep(step);
            spin->setValue(value);
            spin->setMinimumWidth(150);
            spin->setMinimumHeight(34);
            spin->setAlignment(Qt::AlignCenter);
            if (!suffix.isEmpty()) spin->setSuffix(suffix);
            spin->setStyleSheet("QDoubleSpinBox { background:white; border:1px solid #cfd8bb; border-radius:8px; padding:4px 8px; } QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width:18px; }");
            return spin;
        };

        auto makeIntSpin = [formulaBox](const QString &objectName, int min, int max, int value, const QString &suffix = QString()) {
            auto *spin = new QSpinBox(formulaBox);
            spin->setObjectName(objectName);
            spin->setRange(min, max);
            spin->setValue(value);
            spin->setMinimumWidth(150);
            spin->setMinimumHeight(34);
            spin->setAlignment(Qt::AlignCenter);
            if (!suffix.isEmpty()) spin->setSuffix(suffix);
            spin->setStyleSheet("QSpinBox { background:white; border:1px solid #cfd8bb; border-radius:8px; padding:4px 8px; } QSpinBox::up-button, QSpinBox::down-button { width:18px; }");
            return spin;
        };

        advTempReferenceSpin = makeDoubleSpin("advTempReferenceSpin", 0.0, 150.0, 45.0, 1.0, " °C", 1);
        advTempCoeffSpin = makeDoubleSpin("advTempCoeffSpin", 0.0, 10.0, 1.2, 0.1, "", 1);
        advHoursCoeffSpin = makeDoubleSpin("advHoursCoeffSpin", 0.0, 20.0, 4.0, 0.5, "", 1);
        advAgeCoeffSpin = makeDoubleSpin("advAgeCoeffSpin", 0.0, 20.0, 2.5, 0.5, "", 1);
        advMaintenancePenaltySpin = makeIntSpin("advMaintenancePenaltySpin", 0, 100, 18, " pts");
        advPannePenaltySpin = makeIntSpin("advPannePenaltySpin", 0, 100, 35, " pts");
        advOrangeMinSpin = makeIntSpin("advOrangeMinSpin", 0, 100, 40, " /100");
        advRedMinSpin = makeIntSpin("advRedMinSpin", 0, 100, 70, " /100");

        auto *formsRow = new QHBoxLayout();
        formsRow->setSpacing(24);
        auto *leftForm = new QFormLayout();
        auto *rightForm = new QFormLayout();
        leftForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        rightForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        leftForm->setFormAlignment(Qt::AlignTop);
        rightForm->setFormAlignment(Qt::AlignTop);
        leftForm->setHorizontalSpacing(12);
        rightForm->setHorizontalSpacing(12);
        leftForm->setVerticalSpacing(10);
        rightForm->setVerticalSpacing(10);
        leftForm->addRow("Température de référence", advTempReferenceSpin);
        leftForm->addRow("Coefficient heures (par 1000 h)", advHoursCoeffSpin);
        leftForm->addRow("Pénalité maintenance", advMaintenancePenaltySpin);
        leftForm->addRow("Seuil orange", advOrangeMinSpin);
        rightForm->addRow("Coefficient température", advTempCoeffSpin);
        rightForm->addRow("Coefficient ancienneté (par an)", advAgeCoeffSpin);
        rightForm->addRow("Pénalité panne", advPannePenaltySpin);
        rightForm->addRow("Seuil rouge", advRedMinSpin);
        formsRow->addLayout(leftForm, 1);
        formsRow->addLayout(rightForm, 1);
        formulaRoot->addLayout(formsRow);

        advFormulaSummaryLabel = new QLabel(formulaBox);
        advFormulaSummaryLabel->setObjectName("advFormulaSummaryLabel");
        advFormulaSummaryLabel->setWordWrap(true);
        advFormulaSummaryLabel->setStyleSheet("color:#4f5a3c; background:#f7f9f1; border:1px solid #d9dfc8; border-radius:8px; padding:8px;");
        formulaRoot->addWidget(advFormulaSummaryLabel);

        auto *formulaBtnRow = new QHBoxLayout();
        formulaBtnRow->addStretch();
        btnApplyRiskFormula = new QPushButton("Appliquer la formule", formulaBox);
        btnApplyRiskFormula->setObjectName("btnApplyRiskFormula");
        btnApplyRiskFormula->setMinimumHeight(38);
        btnApplyRiskFormula->setStyleSheet("QPushButton { padding:8px 16px; border:1px solid #8aa04b; border-radius:10px; background:white; } QPushButton:hover { background:#f6f9ed; }");
        formulaBtnRow->addWidget(btnApplyRiskFormula);
        formulaRoot->addLayout(formulaBtnRow);
        root->addWidget(formulaBox);

        auto *analysisBox = new QGroupBox("Analyse prédictive des machines", container);
        auto *analysisLayout = new QVBoxLayout(analysisBox);
        analysisLayout->setContentsMargins(14, 18, 14, 14);
        analysisLayout->setSpacing(12);

        auto *btnRow = new QHBoxLayout();
        btnRow->addStretch();
        btnRefreshAdvanced = new QPushButton("Actualiser l'analyse", analysisBox);
        btnRefreshAdvanced->setObjectName("btnRefreshAdvanced");
        btnRefreshAdvanced->setMinimumHeight(38);
        btnRefreshAdvanced->setStyleSheet("QPushButton { padding:8px 16px; border:1px solid #8aa04b; border-radius:10px; background:white; } QPushButton:hover { background:#f6f9ed; }");
        btnExportAdvanced = new QPushButton("Exporter l'analyse", analysisBox);
        btnExportAdvanced->setObjectName("btnExportAdvanced");
        btnExportAdvanced->setMinimumHeight(38);
        btnExportAdvanced->setStyleSheet("QPushButton { padding:8px 16px; border:1px solid #d6dbe8; border-radius:10px; background:white; } QPushButton:hover { background:#f8f9fc; }");
        btnCorrectiveAdvanced = new QPushButton("Correctif ciblé", analysisBox);
        btnCorrectiveAdvanced->setObjectName("btnCorrectiveAdvanced");
        btnCorrectiveAdvanced->setMinimumHeight(38);
        btnCorrectiveAdvanced->setStyleSheet("QPushButton { padding:8px 16px; border:1px solid #c88c2b; color:#6b4200; border:1px solid #e2b05d; border-radius:10px; background:#fff7ea; } QPushButton:hover { background:#fff1d6; }");
        btnRow->addWidget(btnRefreshAdvanced);
        btnRow->addWidget(btnExportAdvanced);
        btnRow->addWidget(btnCorrectiveAdvanced);
        analysisLayout->addLayout(btnRow);

        advMachineTable = new QTableWidget(analysisBox);
        advMachineTable->setObjectName("advMachineTable");
        advMachineTable->setColumnCount(8);
        advMachineTable->setHorizontalHeaderLabels({"ID", "Machine", "Série", "Heures", "Température", "Âge (jours)", "Risque", "Performance"});
        advMachineTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        advMachineTable->setSelectionMode(QAbstractItemView::NoSelection);
        advMachineTable->setAlternatingRowColors(false);
        advMachineTable->verticalHeader()->setVisible(false);
        advMachineTable->setWordWrap(true);
        advMachineTable->setTextElideMode(Qt::ElideRight);
        advMachineTable->setMinimumHeight(320);
        advMachineTable->setMaximumHeight(420);
        advMachineTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        advMachineTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        advMachineTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        advMachineTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        advMachineTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        advMachineTable->setStyleSheet(
            "QTableWidget { border:1px solid #dfe7cf; border-radius:10px; gridline-color:#d6dfc4; background:white; color:#22311a; selection-background-color:#eef4df; selection-color:#22311a; }"
            "QHeaderView::section { background:#eff4e5; color:#2f3b1f; font-weight:700; border:none; border-right:1px solid #d6dfc4; border-bottom:1px solid #d6dfc4; padding:8px 10px; }"
            "QTableWidget::item { padding:8px 10px; color:#22311a; }"
        );
        auto *hh = advMachineTable->horizontalHeader();
        hh->setStretchLastSection(false);
        hh->setDefaultAlignment(Qt::AlignCenter);
        hh->setMinimumSectionSize(90);
        hh->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        hh->setSectionResizeMode(1, QHeaderView::Stretch);
        hh->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        hh->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        hh->setSectionResizeMode(4, QHeaderView::ResizeToContents);
        hh->setSectionResizeMode(5, QHeaderView::ResizeToContents);
        hh->setSectionResizeMode(6, QHeaderView::ResizeToContents);
        hh->setSectionResizeMode(7, QHeaderView::ResizeToContents);
        advMachineTable->setColumnWidth(1, 220);
        advMachineTable->setColumnWidth(2, 120);
        advMachineTable->setColumnWidth(6, 110);
        advMachineTable->setColumnWidth(7, 120);
        analysisLayout->addWidget(advMachineTable);

        auto *legend = new QLabel("Le tableau se colore automatiquement selon le risque : vert = faible, orange = moyen, rouge = élevé. Le bouton Correctif ciblé ouvre la liste des machines en panne avec la durée estimée et les équipements nécessaires.", analysisBox);
        legend->setWordWrap(true);
        legend->setStyleSheet("color:#5c624d;");
        analysisLayout->addWidget(legend);

        root->addWidget(analysisBox);
        root->addStretch();
    }

    advTotalValue = page->findChild<QLabel*>("advTotalValue");
    advActiveValue = page->findChild<QLabel*>("advActiveValue");
    advCriticalValue = page->findChild<QLabel*>("advCriticalValue");
    advAvgPerfValue = page->findChild<QLabel*>("advAvgPerfValue");
    advMachineTable = page->findChild<QTableWidget*>("advMachineTable");
    btnRefreshAdvanced = page->findChild<QPushButton*>("btnRefreshAdvanced");
    btnExportAdvanced = page->findChild<QPushButton*>("btnExportAdvanced");
    btnCorrectiveAdvanced = page->findChild<QPushButton*>("btnCorrectiveAdvanced");
    btnApplyRiskFormula = page->findChild<QPushButton*>("btnApplyRiskFormula");
    advTempReferenceSpin = page->findChild<QDoubleSpinBox*>("advTempReferenceSpin");
    advTempCoeffSpin = page->findChild<QDoubleSpinBox*>("advTempCoeffSpin");
    advHoursCoeffSpin = page->findChild<QDoubleSpinBox*>("advHoursCoeffSpin");
    advAgeCoeffSpin = page->findChild<QDoubleSpinBox*>("advAgeCoeffSpin");
    advMaintenancePenaltySpin = page->findChild<QSpinBox*>("advMaintenancePenaltySpin");
    advPannePenaltySpin = page->findChild<QSpinBox*>("advPannePenaltySpin");
    advOrangeMinSpin = page->findChild<QSpinBox*>("advOrangeMinSpin");
    advRedMinSpin = page->findChild<QSpinBox*>("advRedMinSpin");
    advFormulaSummaryLabel = page->findChild<QLabel*>("advFormulaSummaryLabel");
    machineSensorTargetCombo = page->findChild<QComboBox*>("machineSensorTargetCombo");
    machineSensorPortCombo = page->findChild<QComboBox*>("machineSensorPortCombo");
    btnRefreshMachinePorts = page->findChild<QPushButton*>("btnRefreshMachinePorts");
    btnConnectMachineSensor = page->findChild<QPushButton*>("btnConnectMachineSensor");
    btnDisconnectMachineSensor = page->findChild<QPushButton*>("btnDisconnectMachineSensor");
    machineSensorStatusLabel = page->findChild<QLabel*>("machineSensorStatusLabel");
    machineSensorLastValueLabel = page->findChild<QLabel*>("machineSensorLastValueLabel");
}

void MainWindow::ensureMachineTableColumns()
{
    if (!ui->tablemachine) return;

    ui->tablemachine->setColumnCount(9);
    ui->tablemachine->setHorizontalHeaderLabels({"Id","Nom","Type","Date","Etat","Heures","Température","Série","Actions"});
    ui->tablemachine->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tablemachine->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tablemachine->setSelectionMode(QAbstractItemView::SingleSelection);

    for (int c = 0; c < 8; ++c)
        ui->tablemachine->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);

    ui->tablemachine->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    ui->tablemachine->setColumnWidth(8, 120);
}

void MainWindow::setupMachineHeaderSorting()
{
    if (!ui->tablemachine || !ui->tablemachine->horizontalHeader())
        return;

    QHeaderView *header = ui->tablemachine->horizontalHeader();
    header->setSectionsClickable(true);
    header->setSortIndicatorShown(false);

    disconnect(header, &QHeaderView::sectionClicked,
               this, &MainWindow::showMachineSortMenu);

    connect(header, &QHeaderView::sectionClicked,
            this, &MainWindow::showMachineSortMenu);
}

QString MainWindow::machineColumnTitle(int logicalIndex) const
{
    switch (logicalIndex) {
    case 0: return "Id";
    case 1: return "Nom";
    case 2: return "Type";
    case 3: return "Date";
    case 4: return "Etat";
    case 5: return "Heures";
    case 6: return "Température";
    case 7: return "Série";
    default: return "";
    }
}

QString MainWindow::machineColumnToSql(int logicalIndex) const
{
    switch (logicalIndex) {
    case 0: return "m.id_machine";
    case 1: return "m.nom_machine";
    case 2: return "m.type_machine";
    case 3: return "m.date_installation";
    case 4: return "m.etat_machine";
    case 5: return "m.heures_fonctionnement";
    case 6: return "m.temperature_actuelle";
    case 7: return machineSupportsSeries() ? "s.nom_serie" : "m.nom_machine";
    default: return "";
    }
}

void MainWindow::showMachineSortMenu(int logicalIndex)
{
    if (!ui->tablemachine || !ui->tablemachine->horizontalHeader())
        return;

    if (logicalIndex == 8)
        return;

    const QString sqlColumn = machineColumnToSql(logicalIndex);
    const QString title = machineColumnTitle(logicalIndex);
    if (sqlColumn.isEmpty())
        return;

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: white; border: 1px solid #cfcfcf; padding: 6px; }"
        "QMenu::item { padding: 8px 22px; }"
        "QMenu::item:selected { background-color: #dfe8cf; color: black; }"
    );

    QAction *ascAction = menu.addAction("Trier " + title + " : croissant");
    QAction *descAction = menu.addAction("Trier " + title + " : décroissant");

    QHeaderView *header = ui->tablemachine->horizontalHeader();
    QPoint pos = header->mapToGlobal(QPoint(header->sectionPosition(logicalIndex), header->height()));

    QAction *chosen = menu.exec(pos);
    if (!chosen)
        return;

    orderByMachine = sqlColumn;
    machineSortColumn = logicalIndex;
    machineSortOrder = (chosen == ascAction) ? Qt::AscendingOrder : Qt::DescendingOrder;

    loadMachines(lastMachineWhereSql, lastMachineBinds);
}

bool MainWindow::dbOpen() const
{
    auto db = QSqlDatabase::database();
    return db.isValid() && db.isOpen();
}

bool MainWindow::tableHasColumn(const QString& table, const QString& column) const
{
    if (!dbOpen()) return false;

    QSqlQuery q;
    if (!q.exec("SELECT * FROM " + table + " WHERE 1=0"))
        return false;

    const QSqlRecord rec = q.record();
    return rec.indexOf(column) >= 0
        || rec.indexOf(column.toUpper()) >= 0
        || rec.indexOf(column.toLower()) >= 0;
}

bool MainWindow::machineSupportsSeries() const
{
    if (!dbOpen()) return false;
    QSqlQuery q;
    if (!q.exec("SELECT * FROM SERIE_MACHINE WHERE 1=0"))
        return false;
    return tableHasColumn("MACHINE", "ID_SERIE");
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
    if (t.contains("pann")) return "PANNE";
    if (t.contains("non")) return "PANNE";
    return "ACTIVE";
}

QLineEdit* MainWindow::findMachineNameEdit() const
{
    if (auto *w = findChild<QLineEdit*>("nommachine")) return w;
    return ui->ajoutpersonnel_3 ? ui->ajoutpersonnel_3->findChild<QLineEdit*>("nommachine") : nullptr;
}

QLineEdit* MainWindow::findMachineHoursEdit() const
{
    if (auto *w = findChild<QLineEdit*>("heuresmachine")) return w;
    return ui->ajoutpersonnel_3 ? ui->ajoutpersonnel_3->findChild<QLineEdit*>("heuresmachine") : nullptr;
}

QLineEdit* MainWindow::findMachineTempEdit() const
{
    if (auto *w = findChild<QLineEdit*>("temperaturemachine")) return w;
    return ui->ajoutpersonnel_3 ? ui->ajoutpersonnel_3->findChild<QLineEdit*>("temperaturemachine") : nullptr;
}

QComboBox* MainWindow::findMachineTypeCombo() const
{
    if (auto *w = findChild<QComboBox*>("fonctionmachine")) return w;
    return ui->ajoutpersonnel_3 ? ui->ajoutpersonnel_3->findChild<QComboBox*>("fonctionmachine") : nullptr;
}

QComboBox* MainWindow::findMachineEtatCombo() const
{
    if (auto *w = findChild<QComboBox*>("statusmachine")) return w;
    return nullptr;
}

QDateEdit* MainWindow::findMachineDateEdit() const
{
    if (auto *w = findChild<QDateEdit*>("datefonctionmachine")) return w;
    return ui->ajoutpersonnel_3 ? ui->ajoutpersonnel_3->findChild<QDateEdit*>("datefonctionmachine") : nullptr;
}

void MainWindow::fillSeriesCombo()
{
    if (!cbSerieMachine) return;
    cbSerieMachine->clear();

    if (!dbOpen()) {
        cbSerieMachine->setEnabled(false);
        cbSerieMachine->addItem("-- DB fermée --", -1);
        return;
    }

    if (!machineSupportsSeries()) {
        cbSerieMachine->setEnabled(false);
        cbSerieMachine->addItem("-- schéma sans série --", -1);
        return;
    }

    cbSerieMachine->setEnabled(true);

    QSqlQuery q;
    if(!q.exec("SELECT id_serie, nom_serie FROM SERIE_MACHINE ORDER BY nom_serie")) {
        qDebug() << "SERIE_MACHINE indisponible:" << q.lastError().text();
        cbSerieMachine->setEnabled(false);
        cbSerieMachine->addItem("-- aucune série --", -1);
        return;
    }

    while(q.next()) {
        cbSerieMachine->addItem(q.value(1).toString(), q.value(0).toInt());
    }

    if(cbSerieMachine->count() == 0)
        cbSerieMachine->addItem("-- aucune série --", -1);
}

void MainWindow::loadMachines(const QString& whereSql, const QVariantList& binds)
{
    if (!dbOpen() || !ui->tablemachine) return;

    lastMachineWhereSql = whereSql;
    lastMachineBinds = binds;

    const bool withSeries = machineSupportsSeries();
    QString sql;

    if (withSeries) {
        sql =
            "SELECT m.id_machine, m.nom_machine, m.type_machine, m.date_installation, m.etat_machine, "
            "NVL(m.heures_fonctionnement,0), NVL(m.temperature_actuelle,0), NVL(s.nom_serie, '-') "
            "FROM MACHINE m LEFT JOIN SERIE_MACHINE s ON m.id_serie = s.id_serie ";
    } else {
        sql =
            "SELECT m.id_machine, m.nom_machine, m.type_machine, m.date_installation, m.etat_machine, "
            "NVL(m.heures_fonctionnement,0), NVL(m.temperature_actuelle,0), '-' AS nom_serie "
            "FROM MACHINE m ";
    }

    if (!whereSql.trimmed().isEmpty())
        sql += whereSql + " ";

    sql += "ORDER BY " + orderByMachine;
    sql += (machineSortOrder == Qt::AscendingOrder) ? " ASC" : " DESC";

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
        ui->tablemachine->setItem(row,5,new QTableWidgetItem(QString::number(q.value(5).toDouble(), 'f', 0)));
        ui->tablemachine->setItem(row,6,new QTableWidgetItem(QString::number(q.value(6).toDouble(), 'f', 1) + " °C"));
        ui->tablemachine->setItem(row,7,new QTableWidgetItem(q.value(7).toString()));
        addActionsToMachineRow(row, q.value(0).toInt());
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
    btnEdit->setToolTip("Modifier");
    btnEdit->setIcon(QIcon(":/img/edit.svg"));
    btnEdit->setIconSize(QSize(16,16));
    btnEdit->setFixedSize(32,24);
    btnEdit->setFocusPolicy(Qt::NoFocus);

    auto *btnDel = new QPushButton(container);
    btnDel->setToolTip("Supprimer");
    btnDel->setIcon(QIcon(":/img/delete.svg"));
    btnDel->setIconSize(QSize(16,16));
    btnDel->setFixedSize(32,24);
    btnDel->setFocusPolicy(Qt::NoFocus);

    h->addWidget(btnEdit);
    h->addWidget(btnDel);
    ui->tablemachine->setCellWidget(row, 8, container);
    ui->tablemachine->setRowHeight(row, 34);

    connect(btnEdit, &QPushButton::clicked, this, [this, idMachine](){ handleEditMachine(idMachine); });
    connect(btnDel, &QPushButton::clicked, this, [this, idMachine](){ handleDeleteMachine(idMachine); });
}

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
    auto *leHours = findMachineHoursEdit();
    auto *leTemp = findMachineTempEdit();
    auto *cbType = findMachineTypeCombo();
    auto *cbEtatUi = findMachineEtatCombo();
    auto *de = findMachineDateEdit();

    QString nom = leNom ? leNom->text().trimmed() : "";
    QString type = cbType ? cbType->currentText().trimmed() : "";
    QDate dateInstall = de ? de->date() : QDate::currentDate();
    QString etatDb = mapMachineEtatToDb(cbEtatUi ? cbEtatUi->currentText() : "Actif");
    const bool withSeries = machineSupportsSeries();
    int serieId = (withSeries && cbSerieMachine) ? cbSerieMachine->currentData().toInt() : -1;

    bool okHours = false;
    double hours = leHours ? leHours->text().trimmed().toDouble(&okHours) : 0.0;
    bool okTemp = false;
    double temp = leTemp ? leTemp->text().trimmed().toDouble(&okTemp) : 0.0;

    if (nom.isEmpty()) {
        QMessageBox::warning(this, "Machine", "Nom machine obligatoire.");
        if (leNom) leNom->setFocus();
        busyMachine = false;
        return;
    }
    if (type.isEmpty()) {
        QMessageBox::warning(this, "Machine", "Type obligatoire.");
        if (cbType) cbType->setFocus();
        busyMachine = false;
        return;
    }
    if (leHours && !leHours->text().trimmed().isEmpty() && (!okHours || hours < 0)) {
        QMessageBox::warning(this, "Machine", "Les heures de fonctionnement doivent être un nombre positif.");
        leHours->setFocus();
        busyMachine = false;
        return;
    }
    if (leTemp && !leTemp->text().trimmed().isEmpty() && (!okTemp || temp < -20 || temp > 200)) {
        QMessageBox::warning(this, "Machine", "La température actuelle doit être entre -20 et 200 °C.");
        leTemp->setFocus();
        busyMachine = false;
        return;
    }
    if (withSeries && serieId <= 0) {
        QMessageBox::warning(this, "Machine", "Choisissez d'abord une série machine.");
        if (cbSerieMachine) cbSerieMachine->setFocus();
        busyMachine = false;
        return;
    }

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    QSqlQuery q;
    bool ok = false;

    if (editMachineId != -1) {
        if (withSeries) {
            q.prepare("UPDATE MACHINE SET nom_machine=?, type_machine=?, etat_machine=?, date_installation=?, heures_fonctionnement=?, temperature_actuelle=?, id_serie=? WHERE id_machine=?");
            q.addBindValue(nom);
            q.addBindValue(type);
            q.addBindValue(etatDb);
            q.addBindValue(dateInstall);
            q.addBindValue(hours);
            q.addBindValue(temp);
            q.addBindValue(serieId);
            q.addBindValue(editMachineId);
        } else {
            q.prepare("UPDATE MACHINE SET nom_machine=?, type_machine=?, etat_machine=?, date_installation=?, heures_fonctionnement=?, temperature_actuelle=? WHERE id_machine=?");
            q.addBindValue(nom);
            q.addBindValue(type);
            q.addBindValue(etatDb);
            q.addBindValue(dateInstall);
            q.addBindValue(hours);
            q.addBindValue(temp);
            q.addBindValue(editMachineId);
        }
        ok = q.exec();
    } else {
        if (withSeries) {
            q.prepare("INSERT INTO MACHINE (nom_machine, type_machine, etat_machine, date_installation, heures_fonctionnement, temperature_actuelle, id_serie) VALUES (?, ?, ?, ?, ?, ?, ?)");
            q.addBindValue(nom);
            q.addBindValue(type);
            q.addBindValue(etatDb);
            q.addBindValue(dateInstall);
            q.addBindValue(hours);
            q.addBindValue(temp);
            q.addBindValue(serieId);
        } else {
            q.prepare("INSERT INTO MACHINE (nom_machine, type_machine, etat_machine, date_installation, heures_fonctionnement, temperature_actuelle) VALUES (?, ?, ?, ?, ?, ?)");
            q.addBindValue(nom);
            q.addBindValue(type);
            q.addBindValue(etatDb);
            q.addBindValue(dateInstall);
            q.addBindValue(hours);
            q.addBindValue(temp);
        }
        ok = q.exec();
        if(!ok && q.lastError().text().toUpper().contains("ID_MACHINE")) {
            int id = nextId("MACHINE","id_machine");
            q.clear();
            if (withSeries) {
                q.prepare("INSERT INTO MACHINE (id_machine, nom_machine, type_machine, etat_machine, date_installation, heures_fonctionnement, temperature_actuelle, id_serie) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
                q.addBindValue(id);
                q.addBindValue(nom);
                q.addBindValue(type);
                q.addBindValue(etatDb);
                q.addBindValue(dateInstall);
                q.addBindValue(hours);
                q.addBindValue(temp);
                q.addBindValue(serieId);
            } else {
                q.prepare("INSERT INTO MACHINE (id_machine, nom_machine, type_machine, etat_machine, date_installation, heures_fonctionnement, temperature_actuelle) VALUES (?, ?, ?, ?, ?, ?, ?)");
                q.addBindValue(id);
                q.addBindValue(nom);
                q.addBindValue(type);
                q.addBindValue(etatDb);
                q.addBindValue(dateInstall);
                q.addBindValue(hours);
                q.addBindValue(temp);
            }
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
    QMessageBox::information(this, "OK", (editMachineId==-1) ? "Machine ajoutée" : "Machine modifiée");

    editMachineId = -1;
    if (ui->ajoutermachine) ui->ajoutermachine->setText("Ajouter");
    if (leNom) leNom->clear();
    if (leHours) leHours->clear();
    if (leTemp) leTemp->clear();
    if (cbEtatUi) cbEtatUi->setCurrentIndex(0);
    if (de) de->setDate(QDate::currentDate());

    loadMachines();
    updateMachineCharts();
    refreshAdvancedAnalytics();
    busyMachine = false;
}

void MainWindow::handleEditMachine(int idMachine)
{
    if (!dbOpen()) return;

    const bool withSeries = machineSupportsSeries();
    QSqlQuery q;
    if (withSeries) {
        q.prepare("SELECT nom_machine, type_machine, etat_machine, date_installation, heures_fonctionnement, temperature_actuelle, id_serie FROM MACHINE WHERE id_machine=?");
    } else {
        q.prepare("SELECT nom_machine, type_machine, etat_machine, date_installation, heures_fonctionnement, temperature_actuelle FROM MACHINE WHERE id_machine=?");
    }
    q.addBindValue(idMachine);

    if(!q.exec() || !q.next()) {
        QMessageBox::critical(this, "SQL", q.lastError().text());
        return;
    }

    if (ui->metierspersonnel_2 && ui->ajoutpersonnel_3)
        ui->metierspersonnel_2->setCurrentWidget(ui->ajoutpersonnel_3);

    auto *leNom = findMachineNameEdit();
    auto *leHours = findMachineHoursEdit();
    auto *leTemp = findMachineTempEdit();
    auto *cbType = findMachineTypeCombo();
    auto *cbEtatUi = findMachineEtatCombo();
    auto *de = findMachineDateEdit();

    if (leNom) leNom->setText(q.value(0).toString());
    if (cbType) {
        int idxT = cbType->findText(q.value(1).toString());
        if (idxT < 0) {
            cbType->addItem(q.value(1).toString());
            idxT = cbType->findText(q.value(1).toString());
        }
        cbType->setCurrentIndex(idxT >= 0 ? idxT : 0);
    }

    QString etDb = q.value(2).toString().toUpper();
    QString etUi = (etDb == "ACTIVE") ? "Actif" : (etDb == "MAINTENANCE" ? "Maintenance" : "Panne");
    if (cbEtatUi) {
        int idxE = cbEtatUi->findText(etUi);
        if (idxE < 0) idxE = 0;
        cbEtatUi->setCurrentIndex(idxE);
    }

    QDate d = q.value(3).toDate();
    if (de && d.isValid()) de->setDate(d);
    if (leHours) leHours->setText(QString::number(q.value(4).toDouble(), 'f', 0));
    if (leTemp) leTemp->setText(QString::number(q.value(5).toDouble(), 'f', 1));

    if (withSeries && cbSerieMachine) {
        fillSeriesCombo();
        int serieId = q.value(6).toInt();
        int idxS = cbSerieMachine->findData(serieId);
        cbSerieMachine->setCurrentIndex(idxS >= 0 ? idxS : 0);
    }

    editMachineId = idMachine;
    if (ui->ajoutermachine) ui->ajoutermachine->setText("Enregistrer");
}

void MainWindow::handleDeleteMachine(int idMachine)
{
    auto reply = QMessageBox::question(this, "Suppression", QString("Supprimer la machine ID=%1 ?").arg(idMachine));
    if (reply != QMessageBox::Yes) return;

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    QSqlQuery q;
    q.prepare("DELETE FROM MACHINE WHERE id_machine=?");
    q.addBindValue(idMachine);

    if(!q.exec()) {
        db.rollback();
        QMessageBox::critical(this, "SQL", q.lastError().text());
        return;
    }

    db.commit();
    QMessageBox::information(this, "OK", "Machine supprimée");
    loadMachines();
    updateMachineCharts();
    refreshAdvancedAnalytics();
}

void MainWindow::addSerieFromAddPage()
{
    if (busySerie) return;
    busySerie = true;

    if (!dbOpen()) {
        QMessageBox::warning(this, "DB", "Connexion fermée.");
        busySerie = false;
        return;
    }

    if (!machineSupportsSeries()) {
        QMessageBox::information(this, "Série", "Votre schéma actuel ne gère pas SERIE_MACHINE. Cette étape est optionnelle avec votre SQL local.");
        busySerie = false;
        return;
    }

    auto *leNom = findChild<QLineEdit*>("nomserielinemachine_2");
    auto *leCap = findChild<QLineEdit*>("capacitelineprod");
    auto *deDate = findChild<QDateEdit*>("datefonctioserienmachine_2");
    auto *cbEtat = findChild<QComboBox*>("statulineseriesmachine_2");

    QString nom = leNom ? leNom->text().trimmed() : "";
    bool okCap = false;
    double cap = leCap ? leCap->text().trimmed().toDouble(&okCap) : 0;
    QDate d = deDate ? deDate->date() : QDate::currentDate();
    QString etat = cbEtat ? cbEtat->currentText().trimmed() : "Actif";

    if (nom.isEmpty()) {
        QMessageBox::warning(this, "Série", "Nom série obligatoire.");
        if (leNom) leNom->setFocus();
        busySerie = false;
        return;
    }
    if (!okCap || cap <= 0) {
        QMessageBox::warning(this, "Série", "La capacité doit être un nombre > 0.");
        if (leCap) leCap->setFocus();
        busySerie = false;
        return;
    }

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    QSqlQuery q;
    bool ok = false;

    q.prepare("INSERT INTO SERIE_MACHINE (nom_serie, capacite_production, date_mise_service, etat_serie, responsable, description) VALUES (?, ?, ?, ?, ?, ?)");
    q.addBindValue(nom);
    q.addBindValue(cap);
    q.addBindValue(d);
    q.addBindValue(etat.toUpper());
    q.addBindValue(QString("Admin"));
    q.addBindValue(QVariant());
    ok = q.exec();

    if(!ok && q.lastError().text().toUpper().contains("ID_SERIE")) {
        int id = nextId("SERIE_MACHINE","id_serie");
        q.clear();
        q.prepare("INSERT INTO SERIE_MACHINE (id_serie, nom_serie, capacite_production, date_mise_service, etat_serie, responsable, description) VALUES (?, ?, ?, ?, ?, ?, ?)");
        q.addBindValue(id);
        q.addBindValue(nom);
        q.addBindValue(cap);
        q.addBindValue(d);
        q.addBindValue(etat.toUpper());
        q.addBindValue(QString("Admin"));
        q.addBindValue(QVariant());
        ok = q.exec();
    }

    if(!ok) {
        db.rollback();
        QMessageBox::critical(this, "SQL", q.lastError().text());
        busySerie = false;
        return;
    }

    db.commit();
    QMessageBox::information(this, "OK", "Série ajoutée");
    fillSeriesCombo();
    if (leNom) leNom->clear();
    if (leCap) leCap->clear();
    if (deDate) deDate->setDate(QDate::currentDate());
    if (cbEtat) cbEtat->setCurrentIndex(0);
    busySerie = false;
}

int MainWindow::computeMachineRiskScore(const QString& etat, double temp, double hours, int ageDays) const
{
    const double tempReference = advTempReferenceSpin ? advTempReferenceSpin->value() : 45.0;
    const double tempCoeff = advTempCoeffSpin ? advTempCoeffSpin->value() : 1.2;
    const double hoursCoeff = advHoursCoeffSpin ? advHoursCoeffSpin->value() : 4.0;
    const double ageCoeff = advAgeCoeffSpin ? advAgeCoeffSpin->value() : 2.5;
    const int maintenancePenalty = advMaintenancePenaltySpin ? advMaintenancePenaltySpin->value() : 18;
    const int pannePenalty = advPannePenaltySpin ? advPannePenaltySpin->value() : 35;

    double riskValue = 0.0;
    const QString etatDb = etat.trimmed().toUpper();

    if (etatDb == "PANNE") {
        riskValue += pannePenalty;
    } else if (etatDb == "MAINTENANCE") {
        riskValue += maintenancePenalty;
    } else if (!etatDb.isEmpty() && etatDb != "ACTIVE") {
        riskValue += maintenancePenalty;
    }

    riskValue += tempCoeff * std::max(0.0, temp - tempReference);
    riskValue += hoursCoeff * std::max(0.0, hours / 1000.0);
    riskValue += ageCoeff * std::max(0.0, ageDays / 365.0);

    return std::clamp(qRound(riskValue), 0, 100);
}

int MainWindow::computeMachinePerformanceScore(int risk) const
{
    return std::clamp(100 - risk, 0, 100);
}

QString MainWindow::riskBandLabel(int risk) const
{
    int orangeMin = advOrangeMinSpin ? advOrangeMinSpin->value() : 40;
    int redMin = advRedMinSpin ? advRedMinSpin->value() : 70;
    if (redMin <= orangeMin)
        redMin = orangeMin + 1;

    if (risk >= redMin)
        return "Élevé";
    if (risk >= orangeMin)
        return "Moyen";
    return "Faible";
}

void MainWindow::applyRiskColorsToAdvancedRow(int row, int risk)
{
    if (!advMachineTable) return;

    int orangeMin = advOrangeMinSpin ? advOrangeMinSpin->value() : 40;
    int redMin = advRedMinSpin ? advRedMinSpin->value() : 70;
    if (redMin <= orangeMin)
        redMin = orangeMin + 1;

    QColor rowColor;
    QColor accentColor;
    const QColor textColor("#1f2a16");

    if (risk >= redMin) {
        rowColor = QColor("#fde8e8");
        accentColor = QColor("#ef9a9a");
    } else if (risk >= orangeMin) {
        rowColor = QColor("#fff4df");
        accentColor = QColor("#f7c873");
    } else {
        rowColor = QColor("#e8f6ea");
        accentColor = QColor("#9fd3a8");
    }

    for (int col = 0; col < advMachineTable->columnCount(); ++col) {
        auto *item = advMachineTable->item(row, col);
        if (!item) continue;
        item->setData(Qt::BackgroundRole, rowColor);
        item->setData(Qt::ForegroundRole, textColor);
        item->setTextAlignment(Qt::AlignCenter);
        QFont f = item->font();
        f.setBold(false);
        item->setFont(f);
    }

    const QList<int> strongColumns{1, 6, 7};
    for (int col : strongColumns) {
        if (auto *item = advMachineTable->item(row, col)) {
            item->setData(Qt::BackgroundRole, accentColor);
            item->setData(Qt::ForegroundRole, textColor);
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
        }
    }
}

int MainWindow::correctiveDaysEstimate(const QString& type, double temp, double hours, int ageDays) const
{
    const QString t = type.trimmed().toLower();
    int days = 1;

    if (t.contains("centrif") || t.contains("press") || t.contains("presse"))
        days += 1;
    if (t.contains("filtr") && (temp >= 55.0 || hours >= 4000.0))
        days += 1;
    if (temp >= 55.0)
        days += 1;
    if (temp >= 75.0)
        days += 1;
    if (hours >= 5000.0)
        days += 1;
    if (hours >= 10000.0)
        days += 1;
    if (ageDays >= 3650)
        days += 1;

    return std::clamp(days, 1, 7);
}


QString MainWindow::correctiveEquipmentSummary(const QString& type, double temp, double hours, int ageDays) const
{
    QStringList equipments;
    const QString t = type.trimmed().toLower();

    if (t.contains("filtr")) {
        equipments << "cartouche de filtration premium" << "joints d'etancheite" << "manometre" << "kit de purge";
    } else if (t.contains("press") || t.contains("presse")) {
        equipments << "courroie" << "roulements" << "huile hydraulique" << "kit de calage";
    } else if (t.contains("centrif")) {
        equipments << "kit d'equilibrage" << "capteur de vibration" << "palier" << "outil d'alignement";
    } else if (t.contains("lav")) {
        equipments << "pompe eau" << "electrovanne" << "buses" << "kit de nettoyage";
    } else if (t.contains("broy") || t.contains("moulin")) {
        equipments << "lames ou marteaux" << "tamis" << "graisse technique" << "extracteur";
    } else {
        equipments << "kit de diagnostic" << "jeu de joints" << "lubrifiant technique";
    }

    if (temp >= 60.0)
        equipments << "sonde thermique";
    if (temp >= 80.0)
        equipments << "ventilation d'appoint";
    if (hours >= 8000.0)
        equipments << "kit de maintenance lourde";
    if (ageDays >= 3650)
        equipments << "pieces d'usure";

    equipments.removeDuplicates();
    return equipments.join(", ");
}


QString MainWindow::correctiveActionSummary(const QString& type, double temp, double hours, int ageDays) const
{
    QStringList actions;
    const QString t = type.trimmed().toLower();

    if (t.contains("filtr")) {
        actions << "purger le circuit"
                << "remplacer la cartouche"
                << "recaler la pression de filtration";
    } else if (t.contains("press") || t.contains("presse")) {
        actions << "controler le circuit hydraulique"
                << "retendre la transmission"
                << "verifier les roulements";
    } else if (t.contains("centrif")) {
        actions << "reequilibrer le rotor"
                << "controler la vibration"
                << "realigner le palier";
    } else if (t.contains("lav")) {
        actions << "nettoyer le bloc lavage"
                << "tester le debit"
                << "verifier l'electrovanne";
    } else if (t.contains("broy") || t.contains("moulin")) {
        actions << "inspecter le broyeur"
                << "remplacer les pieces d'usure"
                << "recaler le tamis";
    } else {
        actions << "ouvrir le diagnostic complet"
                << "controler les organes critiques"
                << "valider l'essai de redemarrage";
    }

    if (temp >= 60.0)
        actions << "stabiliser la temperature";
    if (hours >= 8000.0)
        actions << "lancer une revision lourde";
    if (ageDays >= 3650)
        actions << "renouveler les composants fatigues";

    actions.removeDuplicates();
    return actions.join(" + ");
}


void MainWindow::showMachineCorrectiveDialog()
{
    if (!dbOpen()) {
        QMessageBox::warning(this, "Correctif machine", "La base de donnees n'est pas ouverte.");
        return;
    }

    QSqlQuery q;
    QString sql =
        "SELECT m.id_machine, m.nom_machine, m.type_machine, NVL(s.nom_serie, '-'), NVL(m.heures_fonctionnement,0), NVL(m.temperature_actuelle,0), m.date_installation, NVL(m.etat_machine,'ACTIVE') "
        "FROM MACHINE m LEFT JOIN SERIE_MACHINE s ON m.id_serie = s.id_serie "
        "WHERE UPPER(NVL(m.etat_machine,'ACTIVE')) = 'PANNE' ORDER BY m.id_machine";

    bool ok = q.exec(sql);
    if (!ok) {
        q.clear();
        ok = q.exec(
            "SELECT id_machine, nom_machine, type_machine, '-' AS nom_serie, NVL(heures_fonctionnement,0), NVL(temperature_actuelle,0), date_installation, NVL(etat_machine,'ACTIVE') "
            "FROM MACHINE WHERE UPPER(NVL(etat_machine,'ACTIVE')) = 'PANNE' ORDER BY id_machine");
    }

    if (!ok) {
        QMessageBox::warning(this,
                             "Correctif machine",
                             QStringLiteral("Impossible de charger les machines en panne :\n%1")
                                 .arg(q.lastError().text()));
        return;
    }

    struct CorrectiveRowData {
        int id = 0;
        QString machine;
        QString type;
        QString serie;
        int days = 0;
        QString equipments;
        QString action;
        QString priority;
        QString pack;
        QString stage;
        QString mission;
        QString restart;
        QColor rowColor;
        QColor accentColor;
        QString tooltip;
    };

    auto normalizeBulletText = [](QString value) {
        value = value.trimmed();
        value.replace(" + ", "\n- ");
        if (!value.isEmpty() && !value.startsWith("- "))
            value.prepend("- ");
        return value;
    };

    auto buildMission = [](const QString &type, double temp, double hours) {
        const QString t = type.trimmed().toLower();
        if (t.contains("filtr")) {
            if (temp >= 70.0)
                return QStringLiteral("Mission filtration propre - refroidissement et remise sous pression");
            return QStringLiteral("Mission filtration propre - purge, cartouche et etancheite");
        }
        if (t.contains("press") || t.contains("presse"))
            return QStringLiteral("Mission presse relance - hydraulique, transmission et effort");
        if (t.contains("centrif"))
            return QStringLiteral("Mission centrifuge stable - equilibrage et vibration");
        if (t.contains("lav"))
            return QStringLiteral("Mission lavage net - debit, buses et electrovanne");
        if (t.contains("broy") || t.contains("moulin")) {
            if (hours >= 8000.0)
                return QStringLiteral("Mission broyage endurance - revision lourde et usure");
            return QStringLiteral("Mission broyage propre - tamis, lames et graissage");
        }
        return QStringLiteral("Mission redemarrage atelier - diagnostic et validation finale");
    };

    auto buildRestart = [](const QString &type, double temp) {
        const QString t = type.trimmed().toLower();
        if (t.contains("filtr"))
            return QStringLiteral("Redemarrage apres test de debit et pression stabilisee");
        if (t.contains("press") || t.contains("presse"))
            return QStringLiteral("Redemarrage apres test hydraulique et charge a vide");
        if (t.contains("centrif"))
            return QStringLiteral("Redemarrage apres vibration sous seuil et rotor equilibre");
        if (t.contains("lav"))
            return QStringLiteral("Redemarrage apres cycle eau claire et verification du debit");
        if (t.contains("broy") || t.contains("moulin")) {
            if (temp >= 70.0)
                return QStringLiteral("Redemarrage apres refroidissement et essai progressif");
            return QStringLiteral("Redemarrage apres essai a vide et controle des jeux");
        }
        return QStringLiteral("Redemarrage apres validation atelier et test fonctionnel");
    };

    QList<CorrectiveRowData> rows;
    int totalDays = 0;
    int urgentCount = 0;
    int priorityCount = 0;
    int plannedCount = 0;
    int inProgressCount = 0;
    int readyToRestartCount = 0;

    while (q.next()) {
        const int id = q.value(0).toInt();
        QString machine = q.value(1).toString().trimmed();
        const QString type = q.value(2).toString().trimmed();
        QString serie = q.value(3).toString().trimmed();
        const double hours = q.value(4).toDouble();
        const double temp = q.value(5).toDouble();
        const QDate installDate = q.value(6).toDate();
        const int ageDays = installDate.isValid() ? installDate.daysTo(QDate::currentDate()) : 0;

        if (serie.isEmpty()) serie = "-";
        if (machine.isEmpty()) machine = serie != "-" ? serie : QString("Machine %1").arg(id);

        const int days = correctiveDaysEstimate(type, temp, hours, ageDays);
        QString equipments = correctiveEquipmentSummary(type, temp, hours, ageDays).trimmed();
        QString action = correctiveActionSummary(type, temp, hours, ageDays).trimmed();
        if (equipments.isEmpty()) equipments = "Inspection standard";
        if (action.isEmpty()) action = "Controle technique";

        CorrectiveRowData row;
        row.id = id;
        row.machine = machine;
        row.type = type.isEmpty() ? "-" : type;
        row.serie = serie;
        row.days = days;
        row.equipments = normalizeBulletText(equipments);
        row.action = normalizeBulletText(action);
        row.mission = buildMission(type, temp, hours);
        row.restart = buildRestart(type, temp);

        if (days >= 5 || temp >= 90.0 || hours >= 7000.0) {
            row.priority = "URGENT";
            row.pack = "Pack rouge";
            row.stage = "Correction en cours • cellule atelier mobilisee";
            row.rowColor = QColor("#fdeaea");
            row.accentColor = QColor("#c43c35");
            ++urgentCount;
            ++inProgressCount;
        } else if (days >= 3 || temp >= 70.0 || hours >= 4000.0) {
            row.priority = "PRIORITAIRE";
            row.pack = "Pack orange";
            row.stage = "Correction en cours • pieces et equipe deja engagees";
            row.rowColor = QColor("#fff2df");
            row.accentColor = QColor("#d9822b");
            ++priorityCount;
            ++inProgressCount;
        } else {
            row.priority = "PLANIFIE";
            row.pack = "Pack vert";
            row.stage = "Pret atelier • redemarrage a valider";
            row.rowColor = QColor("#eef8ee");
            row.accentColor = QColor("#3f8f54");
            ++plannedCount;
            ++readyToRestartCount;
        }

        row.tooltip = QStringLiteral("Machine en panne\nMission : %1\nStatut : %2\nPriorite : %3\nDuree estimee : %4 jours\nEquipements : %5\nAction : %6\nRedemarrage : %7")
                          .arg(row.mission,
                               row.stage,
                               row.priority,
                               QString::number(days),
                               equipments,
                               action,
                               row.restart);

        totalDays += days;
        rows.append(row);
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Correctif des machines en panne");
    dialog.resize(1460, 820);
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    auto *title = new QLabel("War room corrective - machines en panne", &dialog);
    title->setStyleSheet("font-size:24px; font-weight:800; color:#2f3b1f;");
    layout->addWidget(title);

    auto *subtitle = new QLabel("Vue priorisee des machines en panne : delai de remise en service, kit d'intervention, scenario correctif et relance atelier. Chaque ligne raconte l'etat de la correction en cours pour aider l'equipe a agir vite et bien.", &dialog);
    subtitle->setWordWrap(true);
    subtitle->setStyleSheet("color:#5f674d; background:#f7f9f2; border:1px solid #dde5c9; border-radius:12px; padding:12px;");
    layout->addWidget(subtitle);

    auto createMetricCard = [&](const QString &value, const QString &label, const QString &note, const QString &accent, const QString &bg) {
        auto *card = new QWidget(&dialog);
        card->setStyleSheet(QString("background:%1; border:1px solid %2; border-radius:14px;").arg(bg, accent));
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(14, 12, 14, 12);
        cardLayout->setSpacing(4);

        auto *valueLabel = new QLabel(value, card);
        valueLabel->setStyleSheet(QString("font-size:24px; font-weight:800; color:%1;").arg(accent));
        auto *labelWidget = new QLabel(label, card);
        labelWidget->setStyleSheet("font-size:13px; font-weight:700; color:#2c2c2c;");
        labelWidget->setWordWrap(true);
        auto *noteWidget = new QLabel(note, card);
        noteWidget->setWordWrap(true);
        noteWidget->setStyleSheet("font-size:11px; color:#5c5c5c;");

        cardLayout->addWidget(valueLabel);
        cardLayout->addWidget(labelWidget);
        cardLayout->addWidget(noteWidget);
        return card;
    };

    auto *cardsRow = new QHBoxLayout();
    cardsRow->setSpacing(10);
    cardsRow->addWidget(createMetricCard(QString::number(rows.size()), "Machines en panne", "Nombre total de cas a traiter maintenant.", "#8d2c2c", "#fff4f4"));
    cardsRow->addWidget(createMetricCard(QString::number(totalDays) + " j", "Charge corrective", "Somme des jours estimes pour les correctifs.", "#7a3b00", "#fff7ed"));
    cardsRow->addWidget(createMetricCard(QString::number(inProgressCount), "Corrections en cours", "Machines deja engagees dans un scenario correctif.", "#c43c35", "#fdeaea"));
    cardsRow->addWidget(createMetricCard(QString::number(readyToRestartCount), "Pretes au redemarrage", "Machines avec kit pret et verification finale a lancer.", "#556B2F", "#f4f8eb"));
    layout->addLayout(cardsRow);

    auto *table = new QTableWidget(&dialog);
    table->setColumnCount(11);
    table->setHorizontalHeaderLabels({"ID", "Machine", "Type", "Serie", "Priorite", "Statut atelier", "Jours", "Mission", "Equipements", "Correctif cible", "Relance"});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setVisible(false);
    table->setWordWrap(true);
    table->setTextElideMode(Qt::ElideNone);
    table->setAlternatingRowColors(false);
    table->setShowGrid(true);
    table->setStyleSheet(
        "QTableWidget { border:1px solid #d8dee8; border-radius:14px; background:white; gridline-color:#edf1f6; color:#212121; }"
        "QHeaderView::section { background:#eff4ea; color:#2f3b1f; font-weight:800; border:none; border-right:1px solid #dde5c9; border-bottom:1px solid #dde5c9; padding:10px 12px; }"
        "QTableWidget::item { padding:10px 12px; color:#212121; }"
        "QTableWidget::item:selected { background:#dbe8cb; color:#1f2b12; }"
    );
    auto *header = table->horizontalHeader();
    header->setDefaultAlignment(Qt::AlignCenter);
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(7, QHeaderView::Stretch);
    header->setSectionResizeMode(8, QHeaderView::Stretch);
    header->setSectionResizeMode(9, QHeaderView::Stretch);
    header->setSectionResizeMode(10, QHeaderView::Stretch);
    table->setMinimumHeight(450);
    layout->addWidget(table, 1);

    for (const auto &rowData : rows) {
        const int row = table->rowCount();
        table->insertRow(row);

        auto makeItem = [&](const QString &value, int alignment, bool highlight = false, const QColor &customBg = QColor()) {
            auto *item = new QTableWidgetItem(value);
            item->setTextAlignment(alignment);
            item->setToolTip(rowData.tooltip);
            item->setForeground(QBrush(QColor("#1f1f1f")));
            item->setBackground(QBrush(customBg.isValid() ? customBg : rowData.rowColor));
            if (highlight) {
                item->setBackground(QBrush(rowData.accentColor.lighter(170)));
                QFont f = item->font();
                f.setBold(true);
                item->setFont(f);
            }
            return item;
        };

        auto *idItem = makeItem(QString::number(rowData.id), Qt::AlignCenter, false);
        idItem->setData(Qt::EditRole, rowData.id);
        auto *machineItem = makeItem(rowData.machine, Qt::AlignCenter, true);
        auto *typeItem = makeItem(rowData.type, Qt::AlignCenter, false);
        auto *serieItem = makeItem(rowData.serie, Qt::AlignCenter, false);
        auto *priorityItem = makeItem(rowData.priority, Qt::AlignCenter, true);
        auto *stageItem = makeItem(rowData.stage, Qt::AlignCenter, true, rowData.accentColor.lighter(185));
        auto *daysItem = makeItem(QString::number(rowData.days) + " j", Qt::AlignCenter, true);
        daysItem->setData(Qt::EditRole, rowData.days);
        auto *missionItem = makeItem(rowData.mission, Qt::AlignLeft | Qt::AlignVCenter, true, rowData.accentColor.lighter(192));
        auto *equipItem = makeItem(rowData.equipments, Qt::AlignLeft | Qt::AlignVCenter, false);
        auto *actionItem = makeItem(rowData.action, Qt::AlignLeft | Qt::AlignVCenter, false);
        auto *restartItem = makeItem(rowData.restart, Qt::AlignLeft | Qt::AlignVCenter, false, rowData.accentColor.lighter(195));

        table->setItem(row, 0, idItem);
        table->setItem(row, 1, machineItem);
        table->setItem(row, 2, typeItem);
        table->setItem(row, 3, serieItem);
        table->setItem(row, 4, priorityItem);
        table->setItem(row, 5, stageItem);
        table->setItem(row, 6, daysItem);
        table->setItem(row, 7, missionItem);
        table->setItem(row, 8, equipItem);
        table->setItem(row, 9, actionItem);
        table->setItem(row, 10, restartItem);
        table->setRowHeight(row, 94);
    }

    if (!rows.isEmpty())
        table->sortItems(6, Qt::DescendingOrder);

    auto *legend = new QLabel("Legende visuelle : rouge = intervention urgente, orange = correction en cours prioritaire, vert = redemarrage presque pret. Les colonnes Priorite, Statut atelier, Jours et Mission sont accentuees pour raconter clairement le scenario de correction.", &dialog);
    legend->setWordWrap(true);
    legend->setStyleSheet("color:#525d42; background:#f9fbf6; border:1px dashed #cfdab8; border-radius:10px; padding:10px;");
    layout->addWidget(legend);

    auto *footer = new QLabel(&dialog);
    footer->setWordWrap(true);
    footer->setStyleSheet("color:#5c3520; background:#fff8ef; border:1px solid #f0d7b4; border-radius:12px; padding:12px;");
    if (rows.isEmpty()) {
        footer->setText("Aucune machine en panne pour le moment. Aucun correctif urgent n'est necessaire.");
    } else {
        footer->setText(QString("%1 machine(s) en panne detectee(s). %2 correction(s) sont deja en cours, %3 machine(s) sont proches du redemarrage. Focus recommande : traiter d'abord les %4 urgence(s), puis securiser les relances atelier.")
                            .arg(rows.size())
                            .arg(inProgressCount)
                            .arg(readyToRestartCount)
                            .arg(urgentCount));
    }
    layout->addWidget(footer);

    auto *closeRow = new QHBoxLayout();
    closeRow->addStretch();
    auto *closeBtn = new QPushButton("Fermer", &dialog);
    closeBtn->setMinimumHeight(38);
    closeBtn->setStyleSheet("QPushButton { padding:8px 18px; border:1px solid #cfdab8; border-radius:10px; background:#f5f8ef; color:#2f3b1f; font-weight:700; } QPushButton:hover { background:#e9f0dd; }");
    QObject::connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    closeRow->addWidget(closeBtn);
    layout->addLayout(closeRow);

    dialog.exec();
}


void MainWindow::refreshAdvancedAnalytics()
{
    if (!advMachineTable) return;

    advMachineTable->setRowCount(0);

    int orangeMin = advOrangeMinSpin ? advOrangeMinSpin->value() : 40;
    int redMin = advRedMinSpin ? advRedMinSpin->value() : 70;
    if (redMin <= orangeMin) {
        redMin = orangeMin + 1;
        if (advRedMinSpin)
            advRedMinSpin->setValue(redMin);
    }

    if (advFormulaSummaryLabel) {
        const double tempReference = advTempReferenceSpin ? advTempReferenceSpin->value() : 45.0;
        const double tempCoeff = advTempCoeffSpin ? advTempCoeffSpin->value() : 1.2;
        const double hoursCoeff = advHoursCoeffSpin ? advHoursCoeffSpin->value() : 4.0;
        const double ageCoeff = advAgeCoeffSpin ? advAgeCoeffSpin->value() : 2.5;
        const int maintenancePenalty = advMaintenancePenaltySpin ? advMaintenancePenaltySpin->value() : 18;
        const int pannePenalty = advPannePenaltySpin ? advPannePenaltySpin->value() : 35;

        advFormulaSummaryLabel->setText(
            QString("Formule active : risque = pénalité état + %1 × max(0, température - %2) + %3 × (heures / 1000) + %4 × (âge en jours / 365). "
                    "Pénalité maintenance = %5 pts, pénalité panne = %6 pts. Couleurs : vert < %7, orange de %7 à %8, rouge à partir de %8.")
                .arg(QString::number(tempCoeff, 'f', 1))
                .arg(QString::number(tempReference, 'f', 1))
                .arg(QString::number(hoursCoeff, 'f', 1))
                .arg(QString::number(ageCoeff, 'f', 1))
                .arg(maintenancePenalty)
                .arg(pannePenalty)
                .arg(orangeMin)
                .arg(redMin));
    }

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
                             "Analyse avancée",
                             QString("Impossible de charger l'analyse avancée :\n%1")
                                 .arg(q.lastError().text()));
        return;
    }

    int total = 0;
    int active = 0;
    int critical = 0;
    int perfSum = 0;

    while (q.next()) {
        const int id = q.value(0).toInt();
        QString rawMachine = q.value(1).toString().simplified();
        QString serie = q.value(2).toString().simplified();
        QString machine = rawMachine;
        if (serie.isEmpty() || serie.compare("null", Qt::CaseInsensitive) == 0)
            serie = "-";
        if (machine.isEmpty() || machine == "-" || machine.compare("null", Qt::CaseInsensitive) == 0)
            machine = (serie != "-") ? serie : QString("Machine %1").arg(id);
        if (machine.isEmpty())
            machine = QString("Machine %1").arg(id);
        const double hours = q.value(3).toDouble();
        const double temp = q.value(4).toDouble();
        const QDate installDate = q.value(5).toDate();
        const QString etat = q.value(6).toString().trimmed().toUpper();
        const int ageDays = installDate.isValid() ? installDate.daysTo(QDate::currentDate()) : 0;

        const int risk = computeMachineRiskScore(etat, temp, hours, ageDays);
        const int perf = computeMachinePerformanceScore(risk);
        const QString band = riskBandLabel(risk);

        QString predictedAction;
        QString recommendation;
        if (risk >= redMin) {
            predictedAction = "Maintenance urgente";
            recommendation = "Machine critique : intervention immédiate, contrôle thermique et vérification des pièces sensibles.";
        } else if (risk >= orangeMin) {
            predictedAction = "Contrôle planifié";
            recommendation = "Risque moyen : planifier une maintenance préventive et surveiller la température et les heures.";
        } else {
            predictedAction = "Surveillance normale";
            recommendation = "Risque faible : conserver le rythme actuel de suivi.";
        }

        auto makeItem = [](const QString &value, const QString &tooltip = QString()) {
            auto *item = new QTableWidgetItem(value);
            item->setTextAlignment(Qt::AlignCenter);
            if (!tooltip.isEmpty())
                item->setToolTip(tooltip);
            return item;
        };

        const QString tooltip = QStringLiteral("Etat : %1\nAction predite : %2\n%3")
                                    .arg(etat.isEmpty() ? "ACTIVE" : etat,
                                         predictedAction,
                                         recommendation);

        const int row = advMachineTable->rowCount();
        advMachineTable->insertRow(row);
        advMachineTable->setItem(row, 0, makeItem(QString::number(id), tooltip));
        advMachineTable->setItem(row, 1, makeItem(machine, tooltip));
        advMachineTable->setItem(row, 2, makeItem(serie, tooltip));
        advMachineTable->setItem(row, 3, makeItem(QString::number(hours, 'f', 0), tooltip));
        advMachineTable->setItem(row, 4, makeItem(QString::number(temp, 'f', 1) + " °C", tooltip));
        advMachineTable->setItem(row, 5, makeItem(QString::number(ageDays), tooltip));
        const QString riskText = QString::number(risk) + " %";
        const QString perfText = QString::number(perf) + " %";
        advMachineTable->setItem(row, 6, makeItem(riskText, QStringLiteral("Niveau de risque : %1\n%2").arg(band, tooltip)));
        advMachineTable->setItem(row, 7, makeItem(perfText, tooltip));
        advMachineTable->setRowHeight(row, 42);
        applyRiskColorsToAdvancedRow(row, risk);

        total++;
        if (etat == "ACTIVE") active++;
        if (risk >= redMin) critical++;
        perfSum += perf;
    }

    advMachineTable->resizeColumnsToContents();
    advMachineTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    advMachineTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    advMachineTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    advMachineTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    advMachineTable->setColumnWidth(0, std::max(advMachineTable->columnWidth(0), 70));
    advMachineTable->setColumnWidth(1, std::max(advMachineTable->columnWidth(1), 220));
    advMachineTable->setColumnWidth(2, std::max(advMachineTable->columnWidth(2), 110));
    advMachineTable->setColumnWidth(6, std::max(advMachineTable->columnWidth(6), 110));
    advMachineTable->setColumnWidth(7, std::max(advMachineTable->columnWidth(7), 120));

    if (advTotalValue) advTotalValue->setText(QString::number(total));
    if (advActiveValue) advActiveValue->setText(QString::number(active));
    if (advCriticalValue) advCriticalValue->setText(QString::number(critical));
    if (advAvgPerfValue) advAvgPerfValue->setText(total > 0 ? QString::number(perfSum / total) + " %" : "0 %");
}

void MainWindow::exportTableToCsv(QTableWidget* t, const QString& defaultName) const
{
    if (!t) return;

    QString path = QFileDialog::getSaveFileName(nullptr, "Export CSV", defaultName, "CSV (*.csv)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "Export", "Impossible d'ouvrir le fichier.");
        return;
    }

    QTextStream out(&f);
    int lastCol = t->columnCount();
    if (lastCol > 0) {
        auto *lastHeader = t->horizontalHeaderItem(lastCol - 1);
        if (lastHeader && lastHeader->text().trimmed().compare("Actions", Qt::CaseInsensitive) == 0)
            lastCol -= 1;
    }

    for(int c = 0; c < lastCol; ++c) {
        auto *h = t->horizontalHeaderItem(c);
        out << '"' << (h ? h->text().replace('"', "''") : QString()) << '"';
        if (c < lastCol - 1) out << ';';
    }
    out << "\n";

    for(int r = 0; r < t->rowCount(); ++r) {
        for(int c = 0; c < lastCol; ++c) {
            auto *it = t->item(r,c);
            out << '"' << (it ? it->text().replace('"', "''") : QString()) << '"';
            if (c < lastCol - 1) out << ';';
        }
        out << "\n";
    }

    f.close();
    QMessageBox::information(nullptr, "Export", "Export terminé.");
}

void MainWindow::exportMachineTableToPdf(QTableWidget* t, const QString& title, const QString& defaultName) const
{
    if (!t) return;

    QString path = QFileDialog::getSaveFileName(nullptr, "Export PDF", defaultName, "PDF (*.pdf)");
    if (path.isEmpty()) return;

    QString html =
        "<html><head><meta charset='utf-8'>"
        "<style>"
        "body{font-family:Arial,sans-serif;color:#2f3b1f;}"
        "h1{color:#556B2F;margin-bottom:6px;}"
        ".meta{color:#666;margin-bottom:16px;}"
        "table{width:100%;border-collapse:collapse;}"
        "th,td{border:1px solid #d9dfc8;padding:8px;text-align:left;}"
        "th{background:#f5f7ef;color:#556B2F;}"
        "tr:nth-child(even){background:#fafcf6;}"
        "</style></head><body>";

    html += "<h1>" + title.toHtmlEscaped() + "</h1>";
    html += "<div class='meta'>Généré le : " + QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm:ss") + "</div>";
    html += "<table><thead><tr>";

    int lastCol = t->columnCount();
    if (lastCol > 0) {
        auto *lastHeader = t->horizontalHeaderItem(lastCol - 1);
        if (lastHeader && lastHeader->text().trimmed().compare("Actions", Qt::CaseInsensitive) == 0)
            lastCol -= 1;
    }

    for (int c = 0; c < lastCol; ++c) {
        auto *h = t->horizontalHeaderItem(c);
        html += "<th>" + (h ? h->text().toHtmlEscaped() : QString()) + "</th>";
    }
    html += "</tr></thead><tbody>";

    for (int r = 0; r < t->rowCount(); ++r) {
        html += "<tr>";
        for (int c = 0; c < lastCol; ++c) {
            auto *it = t->item(r, c);
            html += "<td>" + (it ? it->text().toHtmlEscaped() : QString()) + "</td>";
        }
        html += "</tr>";
    }

    html += "</tbody></table></body></html>";

    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);

    QMessageBox::information(nullptr, "Export", "PDF exporté avec succès.");
}


void MainWindow::updateMachineCharts()
{
    if (!ui->statPersonnel_3) return;

    auto *page = ui->statPersonnel_3;
    auto *scroll = page->findChild<QScrollArea*>("machineStatsScroll", Qt::FindDirectChildrenOnly);
    if (!scroll) {
        if (auto *oldLayout = page->layout()) {
            QLayoutItem *item;
            while ((item = oldLayout->takeAt(0)) != nullptr) {
                if (item->widget()) item->widget()->deleteLater();
                delete item;
            }
            delete oldLayout;
        }
        const auto children = page->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (auto *child : children) {
            if (child) child->deleteLater();
        }

        auto *pageLayout = new QVBoxLayout(page);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setSpacing(0);

        scroll = new QScrollArea(page);
        scroll->setObjectName("machineStatsScroll");
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        pageLayout->addWidget(scroll);

        auto *container = new QWidget(scroll);
        container->setObjectName("machineStatsContainer");
        scroll->setWidget(container);

        auto *mainLayout = new QVBoxLayout(container);
        mainLayout->setContentsMargins(20, 18, 20, 24);
        mainLayout->setSpacing(16);

        auto *hero = new QFrame(container);
        hero->setStyleSheet("QFrame { background:#fbfcf8; border:1px solid #dfe7cf; border-radius:20px; } QLabel[role='title'] { font-size:18px; font-weight:800; color:#22311a; } QLabel[role='sub'] { color:#667255; }");
        auto *heroLayout = new QVBoxLayout(hero);
        heroLayout->setContentsMargins(18, 16, 18, 16);
        heroLayout->setSpacing(8);
        auto *heroTitle = new QLabel("Tableau de bord des machines", hero);
        heroTitle->setProperty("role", "title");
        heroTitle->setWordWrap(true);
        auto *heroSub = new QLabel("Suivi visuel de l'état, de la disponibilité, de la température et de l'utilisation des machines de l'huilerie.", hero);
        heroSub->setProperty("role", "sub");
        heroSub->setWordWrap(true);
        heroLayout->addWidget(heroTitle);
        heroLayout->addWidget(heroSub);
        mainLayout->addWidget(hero);

        auto createStatCard = [container](const QString &title, QLabel **valueLabel, const QString &accent, const QString &bg, const QString &subText) -> QFrame* {
            auto *card = new QFrame(container);
            card->setMinimumHeight(130);
            card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            card->setStyleSheet(QString(
                "QFrame { background:%1; border:1px solid #dfe7cf; border-radius:18px; }"
                "QLabel[role='title'] { color:#344126; font-size:13px; font-weight:700; }"
                "QLabel[role='value'] { color:#1c2815; font-size:28px; font-weight:800; }"
                "QLabel[role='pill'] { background:%2; color:white; border-radius:12px; padding:5px 10px; font-size:11px; font-weight:700; }"
                "QLabel[role='sub'] { color:#677255; font-size:11px; }"
            ).arg(bg, accent));
            auto *layout = new QVBoxLayout(card);
            layout->setContentsMargins(16, 14, 16, 14);
            layout->setSpacing(8);
            auto *top = new QHBoxLayout();
            auto *titleLabel = new QLabel(title, card);
            titleLabel->setProperty("role", "title");
            titleLabel->setWordWrap(true);
            auto *pill = new QLabel("KPI", card);
            pill->setProperty("role", "pill");
            pill->setAlignment(Qt::AlignCenter);
            top->addWidget(titleLabel, 1);
            top->addWidget(pill, 0);
            auto *value = new QLabel("0", card);
            value->setProperty("role", "value");
            auto *sub = new QLabel(subText, card);
            sub->setProperty("role", "sub");
            sub->setWordWrap(true);
            layout->addLayout(top);
            layout->addWidget(value);
            layout->addWidget(sub);
            if (valueLabel) *valueLabel = value;
            return card;
        };

        auto *cardsGrid = new QGridLayout();
        cardsGrid->setHorizontalSpacing(16);
        cardsGrid->setVerticalSpacing(16);
        cardsGrid->addWidget(createStatCard("Total des machines", &statMachineTotalValue, "#556B2F", "#f4f8eb", "Nombre total d'équipements suivis dans ce module."), 0, 0);
        cardsGrid->addWidget(createStatCard("Machines actives", &statMachineActiveValue, "#2f8f63", "#eef9f3", "Machines actuellement opérationnelles."), 0, 1);
        cardsGrid->addWidget(createStatCard("Machines en maintenance", &statMachineMaintenanceValue, "#c48a1f", "#fff8ea", "Machines prises en charge en maintenance préventive ou corrective."), 1, 0);
        cardsGrid->addWidget(createStatCard("Machines en panne", &statMachinePanneValue, "#a23d3d", "#fff1f1", "Machines indisponibles nécessitant une intervention."), 1, 1);
        auto *cardsWidget = new QWidget(container);
        cardsWidget->setLayout(cardsGrid);
        mainLayout->addWidget(cardsWidget);

        auto createSectionFrame = [container](const QString &title, const QString &subtitle, QChartView **chartView, int minHeight) -> QFrame* {
            auto *frame = new QFrame(container);
            frame->setStyleSheet("QFrame { background:white; border:1px solid #e7ecdc; border-radius:22px; } QLabel[role='sectionTitle'] { color:#2f3b1f; font-size:17px; font-weight:800; } QLabel[role='sectionSub'] { color:#78806e; font-size:11px; }");
            auto *layout = new QVBoxLayout(frame);
            layout->setContentsMargins(18, 18, 18, 18);
            layout->setSpacing(10);
            auto *titleLabel = new QLabel(title, frame);
            titleLabel->setProperty("role", "sectionTitle");
            titleLabel->setWordWrap(true);
            auto *subtitleLabel = new QLabel(subtitle, frame);
            subtitleLabel->setProperty("role", "sectionSub");
            subtitleLabel->setWordWrap(true);
            auto *view = new QChartView(frame);
            view->setMinimumHeight(minHeight);
            view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            view->setRenderHint(QPainter::Antialiasing);
            view->setStyleSheet("background: transparent;");
            layout->addWidget(titleLabel);
            layout->addWidget(subtitleLabel);
            layout->addWidget(view, 1);
            if (chartView) *chartView = view;
            return frame;
        };

        mainLayout->addWidget(createSectionFrame("Répartition par état", "Diagramme en anneau pour visualiser rapidement les machines actives, en maintenance et en panne.", &chartStatusView, 260));
        mainLayout->addWidget(createSectionFrame("Répartition par type", "Histogramme comparatif du nombre de machines pour chaque type déclaré dans la base.", &chartTypeView, 260));

        auto *bottomFrame = new QFrame(container);
        bottomFrame->setStyleSheet("QFrame { background:white; border:1px solid #e7ecdc; border-radius:22px; } QLabel[role='sectionTitle'] { color:#2f3b1f; font-size:17px; font-weight:800; } QLabel[role='sectionSub'] { color:#78806e; font-size:11px; } QLabel[role='insight'] { background:#f7f9f2; border:1px solid #e5ead7; border-radius:14px; padding:12px; color:#425136; font-size:12px; }");
        auto *bottomLayout = new QVBoxLayout(bottomFrame);
        bottomLayout->setContentsMargins(18, 18, 18, 18);
        bottomLayout->setSpacing(10);
        auto *bottomTitle = new QLabel("Machines les plus sollicitées", bottomFrame);
        bottomTitle->setProperty("role", "sectionTitle");
        bottomTitle->setWordWrap(true);
        auto *bottomSub = new QLabel("Top 5 selon les heures de fonctionnement, utile pour anticiper la maintenance préventive et prioriser les contrôles.", bottomFrame);
        bottomSub->setProperty("role", "sectionSub");
        bottomSub->setWordWrap(true);
        chartHoursView = new QChartView(bottomFrame);
        chartHoursView->setMinimumHeight(280);
        chartHoursView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        chartHoursView->setRenderHint(QPainter::Antialiasing);
        chartHoursView->setStyleSheet("background: transparent;");
        statMachineInsightLabel = new QLabel(bottomFrame);
        statMachineInsightLabel->setProperty("role", "insight");
        statMachineInsightLabel->setWordWrap(true);
        bottomLayout->addWidget(bottomTitle);
        bottomLayout->addWidget(bottomSub);
        bottomLayout->addWidget(chartHoursView, 1);
        bottomLayout->addWidget(statMachineInsightLabel);
        mainLayout->addWidget(bottomFrame);
        mainLayout->addStretch();
    }


    if (!dbOpen()) {
        if (statMachineTotalValue) statMachineTotalValue->setText("0");
        if (statMachineActiveValue) statMachineActiveValue->setText("0");
        if (statMachineMaintenanceValue) statMachineMaintenanceValue->setText("0");
        if (statMachinePanneValue) statMachinePanneValue->setText("0");
        if (statMachineInsightLabel) statMachineInsightLabel->setText("Aucune connexion à la base de données. Ouvre Oracle puis recharge les statistiques.");
        return;
    }

    int total = 0;
    int active = 0;
    int maintenance = 0;
    int panne = 0;
    double avgTemp = 0.0;
    QString hottestMachine = "-";
    double hottestTemp = 0.0;

    QSqlQuery summaryQuery;
    if (summaryQuery.exec("SELECT NVL(etat_machine, 'INCONNU') AS etat, COUNT(*) AS nb FROM MACHINE GROUP BY NVL(etat_machine, 'INCONNU')")) {
        while (summaryQuery.next()) {
            const QString etat = summaryQuery.value(0).toString().trimmed().toUpper();
            const int nb = summaryQuery.value(1).toInt();
            total += nb;
            if (etat == "ACTIVE") active = nb;
            else if (etat == "MAINTENANCE") maintenance = nb;
            else if (etat == "PANNE") panne = nb;
        }
    }

    QSqlQuery tempQuery;
    if (tempQuery.exec("SELECT NVL(AVG(temperature_actuelle),0), NVL(MAX(temperature_actuelle),0) FROM MACHINE")) {
        if (tempQuery.next()) {
            avgTemp = tempQuery.value(0).toDouble();
            hottestTemp = tempQuery.value(1).toDouble();
        }
    }

    QSqlQuery hottestQuery;
    if (hottestQuery.exec("SELECT nom_machine, NVL(temperature_actuelle,0) FROM MACHINE ORDER BY NVL(temperature_actuelle,0) DESC")) {
        if (hottestQuery.next()) {
            hottestMachine = hottestQuery.value(0).toString();
            if (hottestMachine.trimmed().isEmpty()) hottestMachine = "Machine sans nom";
        }
    }

    if (statMachineTotalValue) statMachineTotalValue->setText(QString::number(total));
    if (statMachineActiveValue) statMachineActiveValue->setText(QString::number(active));
    if (statMachineMaintenanceValue) statMachineMaintenanceValue->setText(QString::number(maintenance));
    if (statMachinePanneValue) statMachinePanneValue->setText(QString::number(panne));

    auto *stateSeries = new QPieSeries();
    stateSeries->setHoleSize(0.46);
    struct StateItem { QString name; int count; QColor color; };
    const QList<StateItem> stateItems = {
        {"ACTIVE", active, QColor("#2f9cdb")},
        {"MAINTENANCE", maintenance, QColor("#d4a437")},
        {"PANNE", panne, QColor("#16364c")}
    };

    int maxState = 0;
    for (const auto &item : stateItems) maxState = std::max(maxState, item.count);
    bool hasStateData = false;
    for (const auto &item : stateItems) {
        if (item.count <= 0) continue;
        hasStateData = true;
        auto *slice = stateSeries->append(item.name + " (" + QString::number(item.count) + ")", item.count);
        slice->setBrush(item.color);
        slice->setBorderColor(Qt::white);
        slice->setLabelVisible(true);
        if (item.count == maxState) {
            slice->setExploded(true);
            slice->setExplodeDistanceFactor(0.08);
        }
    }
    if (!hasStateData) {
        auto *slice = stateSeries->append("Aucune donnée", 1);
        slice->setBrush(QColor("#d8decb"));
        slice->setLabelVisible(true);
    }

    auto *stateChart = new QChart();
    stateChart->addSeries(stateSeries);
    stateChart->setBackgroundVisible(false);
    stateChart->setMargins(QMargins(6, 6, 6, 6));
    stateChart->legend()->setAlignment(Qt::AlignBottom);
    stateChart->legend()->setLabelColor(QColor("#415033"));
    stateChart->setAnimationOptions(QChart::SeriesAnimations);
    if (chartStatusView) chartStatusView->setChart(stateChart);

    auto *typeSet = new QBarSet("Machines");
    typeSet->setColor(QColor("#8aa04b"));
    QStringList typeCategories;
    int typeMax = 1;
    QSqlQuery typeQuery;
    if (typeQuery.exec("SELECT NVL(type_machine, 'Non défini') AS type_machine, COUNT(*) AS nb FROM MACHINE GROUP BY NVL(type_machine, 'Non défini') ORDER BY nb DESC")) {
        while (typeQuery.next()) {
            typeCategories << typeQuery.value(0).toString();
            const int count = typeQuery.value(1).toInt();
            *typeSet << count;
            typeMax = std::max(typeMax, count);
        }
    }
    if (typeCategories.isEmpty()) {
        typeCategories << "Aucune donnée";
        *typeSet << 0;
    }

    auto *typeSeries = new QBarSeries();
    typeSeries->append(typeSet);
    typeSeries->setLabelsVisible(true);

    auto *typeChart = new QChart();
    typeChart->addSeries(typeSeries);
    typeChart->setBackgroundVisible(false);
    typeChart->legend()->hide();
    typeChart->setMargins(QMargins(6, 6, 6, 6));
    typeChart->setAnimationOptions(QChart::SeriesAnimations);

    auto *typeAxisX = new QBarCategoryAxis();
    typeAxisX->append(typeCategories);
    typeChart->addAxis(typeAxisX, Qt::AlignBottom);
    typeSeries->attachAxis(typeAxisX);
    auto *typeAxisY = new QValueAxis();
    typeAxisY->setRange(0, typeMax + 1);
    typeAxisY->setLabelFormat("%d");
    typeAxisY->setTickCount(std::max(2, typeMax + 1));
    typeChart->addAxis(typeAxisY, Qt::AlignLeft);
    typeSeries->attachAxis(typeAxisY);
    if (chartTypeView) chartTypeView->setChart(typeChart);

    auto *hoursSet = new QBarSet("Heures de fonctionnement");
    hoursSet->setColor(QColor("#c28a2c"));
    QStringList hourCategories;
    qreal maxHours = 1.0;
    QString mostUsedMachine = "-";
    qreal mostUsedHours = 0.0;

    QSqlQuery hoursQuery;
    if (hoursQuery.exec("SELECT nom_machine, h FROM (SELECT NVL(nom_machine, 'Machine') AS nom_machine, NVL(heures_fonctionnement,0) AS h FROM MACHINE ORDER BY NVL(heures_fonctionnement,0) DESC) WHERE ROWNUM <= 5")) {
        bool firstRow = true;
        while (hoursQuery.next()) {
            const QString machineName = hoursQuery.value(0).toString();
            const qreal hours = hoursQuery.value(1).toDouble();
            hourCategories << machineName;
            *hoursSet << hours;
            maxHours = std::max(maxHours, hours);
            if (firstRow) {
                mostUsedMachine = machineName;
                mostUsedHours = hours;
                firstRow = false;
            }
        }
    }
    if (hourCategories.isEmpty()) {
        hourCategories << "Aucune donnée";
        *hoursSet << 0;
    }

    auto *hoursSeries = new QHorizontalBarSeries();
    hoursSeries->append(hoursSet);
    hoursSeries->setLabelsVisible(true);
    hoursSeries->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);

    auto *hoursChart = new QChart();
    hoursChart->addSeries(hoursSeries);
    hoursChart->setBackgroundVisible(false);
    hoursChart->legend()->hide();
    hoursChart->setMargins(QMargins(6, 6, 6, 6));
    hoursChart->setAnimationOptions(QChart::SeriesAnimations);

    auto *hoursAxisX = new QValueAxis();
    hoursAxisX->setRange(0, maxHours + std::max<qreal>(10.0, maxHours * 0.15));
    hoursAxisX->setLabelFormat("%.0f h");
    hoursChart->addAxis(hoursAxisX, Qt::AlignBottom);
    hoursSeries->attachAxis(hoursAxisX);
    auto *hoursAxisY = new QBarCategoryAxis();
    hoursAxisY->append(hourCategories);
    hoursChart->addAxis(hoursAxisY, Qt::AlignLeft);
    hoursSeries->attachAxis(hoursAxisY);
    if (chartHoursView) chartHoursView->setChart(hoursChart);

    QString healthMessage;
    if (panne > 0) {
        healthMessage = "Attention : des machines sont en panne, il faut prioriser une intervention corrective.";
    } else if (maintenance > 0) {
        healthMessage = "Bon signal : aucune panne bloquante, mais une maintenance préventive est déjà en cours.";
    } else {
        healthMessage = "Excellent : le parc machine est entièrement opérationnel pour le moment.";
    }

    if (statMachineInsightLabel) {
        statMachineInsightLabel->setText(
            QString("Insight intelligent : la machine la plus sollicitée est <b>%1</b> avec <b>%2 h</b>. "
                    "La température moyenne du parc est de <b>%3 °C</b> et la plus chaude est <b>%4</b> avec <b>%5 °C</b>. %6")
                .arg(mostUsedMachine.isEmpty() ? "-" : mostUsedMachine)
                .arg(QString::number(mostUsedHours, 'f', 0))
                .arg(QString::number(avgTemp, 'f', 1))
                .arg(hottestMachine)
                .arg(QString::number(hottestTemp, 'f', 1))
                .arg(healthMessage));
    }
}


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
    refreshMachineSerialPorts();
    loadMachineSensorTargets();
    refreshAdvancedAnalytics();
}

void MainWindow::on_ajoutermachine_clicked()
{
    saveMachineFromForm();
}

void MainWindow::on_rechrchemahine_clicked()
{
    QString txt = ui->recherchemachine ? ui->recherchemachine->text().trimmed() : "";
    if (txt.isEmpty()) {
        loadMachines();
        return;
    }

    QString mode = ui->datmachinne ? ui->datmachinne->currentText().trimmed().toUpper() : "NOM";
    if (mode == "DATE") {
        loadMachines("WHERE TO_CHAR(m.date_installation,'YYYY-MM-DD') = ?", {txt});
    } else if (mode == "ID") {
        loadMachines("WHERE TO_CHAR(m.id_machine) LIKE ?", { "%" + txt + "%" });
    } else if (mode == "TYPE") {
        loadMachines("WHERE UPPER(m.type_machine) LIKE UPPER(?)", { "%" + txt + "%" });
    } else if (mode == "ETAT") {
        loadMachines("WHERE UPPER(m.etat_machine) LIKE UPPER(?)", { "%" + txt + "%" });
    } else {
        loadMachines("WHERE UPPER(m.nom_machine) LIKE UPPER(?)", { "%" + txt + "%" });
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
    if (v == "id") { orderByMachine = "m.id_machine"; machineSortColumn = 0; }
    else if (v == "nom") { orderByMachine = "m.nom_machine"; machineSortColumn = 1; }
    else if (v == "datefonctionnement") { orderByMachine = "m.date_installation"; machineSortColumn = 3; }
    else if (v == "etat") { orderByMachine = "m.etat_machine"; machineSortColumn = 4; }
    else if (v == "type") { orderByMachine = "m.type_machine"; machineSortColumn = 2; }
    else if (v == "heures") { orderByMachine = "m.heures_fonctionnement"; machineSortColumn = 5; }
    else if (v == "temperature") { orderByMachine = "m.temperature_actuelle"; machineSortColumn = 6; }
    else { orderByMachine = "m.id_machine"; machineSortColumn = 0; }

    machineSortOrder = Qt::AscendingOrder;
    loadMachines(lastMachineWhereSql, lastMachineBinds);
}

void MainWindow::on_exportmachine_clicked()
{
    exportMachineTableToPdf(ui->tablemachine, "Liste des machines", "machines.pdf");
}

void MainWindow::on_exportermachinne_clicked()
{
    exportTableToCsv(ui->tablemachine, "machines.csv");
}

void MainWindow::on_ajouterlineseriemachine_2_clicked()
{
    addSerieFromAddPage();
}
