#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartGlobal>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
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
#include <QString>
#include <QDialog>
#include <QLabel>
#include <QToolButton>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
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
#include <QTextStream>
#include <QFileDialog>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QPieSlice>
// Qt 6: Charts classes are accessible without a QtCharts namespace when linked

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

        // Wrap the modules in a content area that applies offsets (drop + right shift)
        auto* contentArea = new QWidget(ui->mainprogram);
        auto* contentLayout = new QVBoxLayout(contentArea);
        qreal dpiX = QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen()->logicalDotsPerInchX() : 96.0;
        qreal dpiY = QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen()->logicalDotsPerInchY() : 96.0;
        int leftMarginPx = static_cast<int>((20.0 / 25.4) * dpiX); // 20 mm ~ 2 cm
        int topMarginPx  = static_cast<int>((10.0 / 25.4) * dpiY); // 10 mm ~ 1 cm
        contentLayout->setContentsMargins(leftMarginPx, topMarginPx, 0, 0);
        contentLayout->setSpacing(0);
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
    // Initialize the personnel table with action buttons
    setupPersonnelTable();
    // Add sidebar toggle button (hamburger) and interaction hooks
    setupInteractiveHooks();
    connect(ui->btnStatHuile, &QToolButton::clicked, this, &MainWindow::on_btnStatHuile_clicked);
    // Palette de couleur — QLineEdit ne supporte pas clicked(), connexion manuelle
    connect(ui->codecouleurLineEdit, &QLineEdit::selectionChanged, this, [this]() {
        // ne rien faire ici
    });
    ui->codecouleurLineEdit->installEventFilter(this);
}

MainWindow::~MainWindow()
{
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
}

void MainWindow::on_btnAdvEmp_clicked()
{
    if (ui->modules->currentIndex() != 0)
        crossFadeToIndex(ui->modules, 0);
    crossFadeToIndex(ui->metierspersonnel, 3);
}

// ════════════════════════════════════════════════════════════════════════════
//  SIDEBAR MODULE NAVIGATION
// ════════════════════════════════════════════════════════════════════════════

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
    setActiveModuleButton(5);
}

void MainWindow::on_btnmod3_clicked()
{
    ui->MainStacked->setCurrentIndex(1);
    ui->modules->setCurrentIndex(1);
    setActiveModuleButton(1);
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
    // Génère automatiquement le prochain ID lot
    QSqlQuery seqQuery;
    seqQuery.prepare("SELECT seq_id_lot.NEXTVAL FROM DUAL");
    if (seqQuery.exec() && seqQuery.next()) {
        ui->idlotLineEdit->setText(seqQuery.value(0).toString());
        ui->idlotLineEdit->setReadOnly(true); // l'utilisateur ne peut pas modifier
        ui->idlotLineEdit->setStyleSheet("background-color: #f0f0f0; color: #888;");
    }

    // ... reste du code existant
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

void MainWindow::on_ajouterHuileBtn_clicked()
{
    // --- Récupération des valeurs ---
    QString idlot       = ui->idlotLineEdit->text().trimmed();
    QString nomResp     = ui->nomresponsableLineEdit->text().trimmed();
    QString dateStr     = ui->dateprodDateEdit->date().toString("yyyy-MM-dd");
    QString statut      = ui->statutComboBox->currentText();
    QString quantite    = ui->quantiteLineEdit->text().trimmed();
    QString maxqt       = ui->maxqtLineEdit->text().trimmed();
    QString ph          = ui->phLineEdit->text().trimmed();
    QString idStock     = ui->idstockLineEdit->text().trimmed();  // ← NOUVEAU
    QString acidite     = ui->aciditeComboBox->currentText();
    QString codeCouleur = ui->codecouleurLineEdit->text().trimmed();
    QString amerture    = ui->amertureLineEdit->text().trimmed();

    // --- CONTROLE 1 : Champs vides ---
    if (idlot.isEmpty() || nomResp.isEmpty() || quantite.isEmpty() ||
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
                  "(id_lot, date_production, quantite_produite, "
                  "ph, acidite, amerture, code_couleur, statut_qualite, "
                  "responsable_controle, max_quantite, id_stock) "
                  "VALUES "
                  "(?, TO_DATE(?, 'YYYY-MM-DD'), ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    query.addBindValue(idlot.toInt());
    query.addBindValue(dateStr);
    query.addBindValue(quantite.toDouble());
    query.addBindValue(ph.toDouble());
    query.addBindValue(acidite);
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
    if (current == next) return;

    auto* outEffect = new QGraphicsOpacityEffect(current);
    current->setGraphicsEffect(outEffect);
    auto* outAnim = new QPropertyAnimation(outEffect, "opacity", current);
    outAnim->setDuration(180);
    outAnim->setStartValue(1.0);
    outAnim->setEndValue(0.0);
    outAnim->setEasingCurve(QEasingCurve::OutCubic);

    QObject::connect(outAnim, &QPropertyAnimation::finished, this, [this, stack, newIndex, current]() {
        current->setGraphicsEffect(nullptr);
        stack->setCurrentIndex(newIndex);
        QWidget* incoming = stack->currentWidget();
        auto* inEffect = new QGraphicsOpacityEffect(incoming);
        incoming->setGraphicsEffect(inEffect);
        auto* inAnim = new QPropertyAnimation(inEffect, "opacity", incoming);
        inAnim->setDuration(180);
        inAnim->setStartValue(0.0);
        inAnim->setEndValue(1.0);
        inAnim->setEasingCurve(QEasingCurve::OutCubic);
        QObject::connect(inAnim, &QPropertyAnimation::finished, incoming, [incoming]() {
            incoming->setGraphicsEffect(nullptr);
        });
        inAnim->start(QAbstractAnimation::DeleteWhenStopped);
    });
    outAnim->start(QAbstractAnimation::DeleteWhenStopped);
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
        toggleBtn->setToolTip(tr("Collapse/Expand sidebar"));
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
        table->setColumnWidth(last, 90);
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
    dlg.setWindowTitle(tr("Facial Recognition"));
    dlg.resize(640, 420);

    auto* root = new QVBoxLayout(&dlg);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* heading = new QLabel(tr("Facial Recognition"), &dlg);
    heading->setProperty("type", "heading");
    root->addWidget(heading);

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

    footer->addWidget(status);
    footer->addItem(spacer);
    footer->addWidget(btnStart);
    footer->addWidget(btnClose);
    root->addLayout(footer);

    bool running = false;
    QObject::connect(btnStart, &QPushButton::clicked, &dlg, [&, status, btnStart]() mutable {
        running = !running;
        status->setText(running ? tr("Camera running…") : tr("Camera idle"));
        btnStart->setText(running ? tr("Stop") : tr("Start"));
    });
    QObject::connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::reject);

    dlg.exec();
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
    QString acidite     = ui->aciditeComboBox->currentText();
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
    query.addBindValue(acidite);
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
    // Créer un conteneur pour les statistiques
    QWidget* statWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(statWidget);

    // Titre
    QLabel* title = new QLabel("STATISTIQUES DES HUILES");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; margin: 10px;");
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    // Layout pour les graphiques (2 colonnes)
    QHBoxLayout* chartsLayout = new QHBoxLayout();

    // 1. Graphique en camembert
    QWidget* pieContainer = new QWidget();
    pieContainer->setStyleSheet("background-color: white; border-radius: 10px;");
    QVBoxLayout* pieLayout = new QVBoxLayout(pieContainer);
    QLabel* pieTitle = new QLabel("Répartition par Statut");
    pieTitle->setStyleSheet("font-weight: bold; margin: 5px;");
    pieTitle->setAlignment(Qt::AlignCenter);
    pieLayout->addWidget(pieTitle);

    m_pieChartView = new QChartView();
    m_pieChartView->setRenderHint(QPainter::Antialiasing);
    pieLayout->addWidget(m_pieChartView);
    chartsLayout->addWidget(pieContainer);

    // 2. Graphique en barres
    QWidget* barContainer = new QWidget();
    barContainer->setStyleSheet("background-color: white; border-radius: 10px;");
    QVBoxLayout* barLayout = new QVBoxLayout(barContainer);
    QLabel* barTitle = new QLabel("Quantités produites par lot");
    barTitle->setStyleSheet("font-weight: bold; margin: 5px;");
    barTitle->setAlignment(Qt::AlignCenter);
    barLayout->addWidget(barTitle);

    m_barChartView = new QChartView();
    m_barChartView->setRenderHint(QPainter::Antialiasing);
    barLayout->addWidget(m_barChartView);
    chartsLayout->addWidget(barContainer);

    mainLayout->addLayout(chartsLayout);

    // 3. Graphique en lignes
    QWidget* lineContainer = new QWidget();
    lineContainer->setStyleSheet("background-color: white; border-radius: 10px;");
    QVBoxLayout* lineLayout = new QVBoxLayout(lineContainer);
    QLabel* lineTitle = new QLabel("Tendances pH et Acidité");
    lineTitle->setStyleSheet("font-weight: bold; margin: 5px;");
    lineTitle->setAlignment(Qt::AlignCenter);
    lineLayout->addWidget(lineTitle);

    m_lineChartView = new QChartView();
    m_lineChartView->setRenderHint(QPainter::Antialiasing);
    lineLayout->addWidget(m_lineChartView);
    mainLayout->addWidget(lineContainer);

    // Ajouter le widget à la page statistiques
    QStackedWidget* metiersHuile = ui->metiershuile;
    if (metiersHuile && metiersHuile->count() > 2) {
        QWidget* oldWidget = metiersHuile->widget(2);
        metiersHuile->removeWidget(oldWidget);
        metiersHuile->insertWidget(2, statWidget);
        delete oldWidget;
    }
}

void MainWindow::chargerStatistiquesHuile()
{
    // 1. Camembert - Répartition par statut
    QPieSeries* pieSeries = new QPieSeries();

    QSqlQuery queryStatus;
    queryStatus.prepare("SELECT statut_qualite, COUNT(*) as count FROM QUALITE GROUP BY statut_qualite");

    if (queryStatus.exec()) {
        while (queryStatus.next()) {
            QString status = queryStatus.value("statut_qualite").toString();
            int count = queryStatus.value("count").toInt();
            pieSeries->append(status, count);
        }
    }

    for (QPieSlice* slice : pieSeries->slices()) {
        slice->setLabelVisible(true);
        slice->setLabel(QString("%1: %2").arg(slice->label()).arg(slice->value()));
    }

    QChart* pieChart = new QChart();
    pieChart->addSeries(pieSeries);
    pieChart->setTitle("Répartition par Statut");
    pieChart->legend()->setAlignment(Qt::AlignRight);
    m_pieChartView->setChart(pieChart);

    // 2. Barres - Quantités produites
    QBarSeries* barSeries = new QBarSeries();
    QBarSet* barSet = new QBarSet("Quantité (L)");

    QStringList categories;
    QSqlQuery queryQuantite;
    queryQuantite.prepare("SELECT id_lot, quantite_produite FROM QUALITE ORDER BY id_lot");

    if (queryQuantite.exec()) {
        while (queryQuantite.next()) {
            QString idLot = queryQuantite.value("id_lot").toString();
            double quantite = queryQuantite.value("quantite_produite").toDouble();
            *barSet << quantite;
            categories << idLot;
        }
    }

    barSeries->append(barSet);

    QChart* barChart = new QChart();
    barChart->addSeries(barSeries);
    barChart->setTitle("Quantités produites par lot");
    barChart->setAnimationOptions(QChart::SeriesAnimations);

    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Quantité (L)");

    barChart->addAxis(axisX, Qt::AlignBottom);
    barChart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisX);
    barSeries->attachAxis(axisY);

    m_barChartView->setChart(barChart);

    // 3. Lignes - pH et Acidité
    QLineSeries* phSeries = new QLineSeries();
    phSeries->setName("pH");
    QLineSeries* aciditeSeries = new QLineSeries();
    aciditeSeries->setName("Acidité");

    QSqlQuery queryTendance;
    queryTendance.prepare("SELECT id_lot, ph, acidite FROM QUALITE ORDER BY id_lot");

    int index = 0;
    if (queryTendance.exec()) {
        while (queryTendance.next()) {
            double ph = queryTendance.value("ph").toDouble();
            QString aciditeStr = queryTendance.value("acidite").toString();
            double aciditeValue = 0;

            if (aciditeStr == "Acide") aciditeValue = 1;
            else if (aciditeStr == "Basique") aciditeValue = 2;
            else if (aciditeStr == "Neutre") aciditeValue = 0.5;

            phSeries->append(index, ph);
            aciditeSeries->append(index, aciditeValue);
            index++;
        }
    }

    QChart* lineChart = new QChart();
    lineChart->addSeries(phSeries);
    lineChart->addSeries(aciditeSeries);
    lineChart->setTitle("Tendances pH et Acidité");
    lineChart->setAnimationOptions(QChart::SeriesAnimations);

    QValueAxis* xAxis = new QValueAxis();
    xAxis->setTitleText("Lot ID");
    if (index > 0) {
        xAxis->setRange(0, index - 1);
        xAxis->setTickCount(qMin(6, index));
    }

    QValueAxis* yAxisPh = new QValueAxis();
    yAxisPh->setTitleText("pH");
    yAxisPh->setRange(0, 14);

    QValueAxis* yAxisAcidite = new QValueAxis();
    yAxisAcidite->setTitleText("Acidité");
    yAxisAcidite->setRange(0, 3);

    lineChart->addAxis(xAxis, Qt::AlignBottom);
    lineChart->addAxis(yAxisPh, Qt::AlignLeft);
    lineChart->addAxis(yAxisAcidite, Qt::AlignRight);

    phSeries->attachAxis(xAxis);
    phSeries->attachAxis(yAxisPh);
    aciditeSeries->attachAxis(xAxis);
    aciditeSeries->attachAxis(yAxisAcidite);

    lineChart->legend()->setAlignment(Qt::AlignBottom);

    m_lineChartView->setChart(lineChart);
}

void MainWindow::on_btnStatHuile_clicked()
{
    if (ui->metiershuile) {
        ui->metiershuile->setCurrentIndex(2);
        chargerStatistiquesHuile();
    }
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
        // Aller à l'index de la page "metieravancee"
        // L'index 3 correspond à la page "metieravancee" dans metiershuile
        ui->metiershuile->setCurrentIndex(3);

        QMessageBox::information(this, "Mode Avancé",
                                 "Fonctionnalités avancées - Page en développement");
    }
}
