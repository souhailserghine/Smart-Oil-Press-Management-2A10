#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QChart>
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
#include <QLineEdit>
#include <QComboBox>
#include <QTimer>

// ─────────────────────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ── Layout sidebar + contenu ──────────────────────────────────────────
    {
        auto mainLayout = new QHBoxLayout(ui->mainprogram);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        ui->sidebar->setMinimumWidth(200);
        ui->sidebar->setMaximumWidth(220);
        ui->sidebar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        mainLayout->addWidget(ui->sidebar);

        auto* contentArea   = new QWidget(ui->mainprogram);
        auto* contentLayout = new QVBoxLayout(contentArea);

        qreal dpiX = QGuiApplication::primaryScreen()
                         ? QGuiApplication::primaryScreen()->logicalDotsPerInchX() : 96.0;
        qreal dpiY = QGuiApplication::primaryScreen()
                         ? QGuiApplication::primaryScreen()->logicalDotsPerInchY() : 96.0;

        int leftMarginPx = static_cast<int>((20.0 / 25.4) * dpiX);
        int topMarginPx  = static_cast<int>((10.0 / 25.4) * dpiY);

        contentLayout->setContentsMargins(leftMarginPx, topMarginPx, 0, 0);
        contentLayout->setSpacing(0);
        contentLayout->addWidget(ui->modules);
        mainLayout->addWidget(contentArea);
    }

    // ── Boutons sidebar checkables ────────────────────────────────────────
    if (ui->sidebar) {
        ui->btnmod1->setCheckable(true);
        ui->btnmod2->setCheckable(true);
        ui->btnmod3->setCheckable(true);
        ui->btnmod4->setCheckable(true);
        ui->btnmod5->setCheckable(true);
        ui->btnmod6->setCheckable(true);
        setActiveModuleButton(0);
    }

    setupPersonnelChart();
    setupPersonnelTable();
    setupInteractiveHooks();

    // ── Connexion Arduino non-bloquante ───────────────────────────────────
    // On lance la détection APRÈS que la fenêtre soit affichée (délai 0 ms)
    // pour ne jamais geler l'interface, même si Arduino est branché.
    QTimer::singleShot(0, this, &MainWindow::connectArduinoAsync);
}

MainWindow::~MainWindow()
{
    if (m_arduino) {
        m_arduino->close_arduino();
        delete m_arduino;
    }
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
// CONNEXION ARDUINO NON-BLOQUANTE
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::connectArduinoAsync()
{
    // Étape 1 : ouvrir le port (rapide, ne bloque plus car msleep supprimé)
    m_arduino = new Arduino();
    int result = m_arduino->connect_arduino();

    if (result == 0) {
        // Étape 2 : attendre 2 s que l'Arduino finisse son reset,
        //           SANS bloquer l'UI grâce à QTimer::singleShot
        statusBar()->showMessage(
            QString("Arduino détecté sur %1 — attente démarrage...")
                .arg(m_arduino->getarduino_port_name()), 2500);

        QTimer::singleShot(2000, this, &MainWindow::finishArduinoConnect);

    } else if (result == 1) {
        // Port trouvé mais impossible à ouvrir (déjà utilisé, droits, etc.)
        QMessageBox::warning(this, "Arduino",
                             "Arduino détecté mais impossible d'ouvrir le port.\n"
                             "Vérifiez qu'aucun autre programme n'utilise le port.");
        delete m_arduino;
        m_arduino = nullptr;
    } else {
        // -1 : aucun Arduino — l'application continue normalement
        qDebug() << "[MainWindow] Arduino non détecté. L'application fonctionne sans Arduino.";
        delete m_arduino;
        m_arduino = nullptr;
    }
}

void MainWindow::finishArduinoConnect()
{
    // Appelé 2 s après l'ouverture du port : l'Arduino a eu le temps de démarrer
    if (!m_arduino || !m_arduino->getserial() || !m_arduino->getserial()->isOpen()) {
        qDebug() << "[MainWindow] finishArduinoConnect : port fermé, abandon.";
        return;
    }

    qDebug() << "[MainWindow] Arduino prêt sur :" << m_arduino->getarduino_port_name();
    statusBar()->showMessage(
        QString("Arduino connecté sur %1").arg(m_arduino->getarduino_port_name()), 4000);
}

// ─────────────────────────────────────────────────────────────────────────────
// NAVIGATION
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::on_loginbtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

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

// ── Boutons sidebar → modules ─────────────────────────────────────────────

void MainWindow::on_btnmod1_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->modules->setCurrentIndex(0);
    setActiveModuleButton(0);

    if (m_arduino && m_arduino->getserial() && m_arduino->getserial()->isOpen())
        m_arduino->write_to_arduino("1");
}

void MainWindow::on_btnmod2_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->modules->setCurrentIndex(5);
    setActiveModuleButton(5);

    if (m_arduino && m_arduino->getserial() && m_arduino->getserial()->isOpen())
        m_arduino->write_to_arduino("0");
}

void MainWindow::on_btnmod3_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->modules->setCurrentIndex(1);
    setActiveModuleButton(1);
}

void MainWindow::on_btnmod4_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->modules->setCurrentIndex(2);
    setActiveModuleButton(2);
}

void MainWindow::on_btnmod5_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->modules->setCurrentIndex(3);
    setActiveModuleButton(3);
}

void MainWindow::on_btnmod6_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->modules->setCurrentIndex(4);
    setActiveModuleButton(4);
}

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setActiveModuleButton(int index)
{
    QPushButton* buttons[6] = {
        ui->btnmod1, ui->btnmod2, ui->btnmod3,
        ui->btnmod4, ui->btnmod5, ui->btnmod6
    };
    for (auto* b : buttons) b->setChecked(false);

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
    if (!stack || newIndex < 0 || newIndex >= stack->count()) return;
    QWidget* current = stack->currentWidget();
    QWidget* next    = stack->widget(newIndex);
    if (current == next) return;

    auto* outEffect = new QGraphicsOpacityEffect(current);
    current->setGraphicsEffect(outEffect);
    auto* outAnim = new QPropertyAnimation(outEffect, "opacity", current);
    outAnim->setDuration(180);
    outAnim->setStartValue(1.0);
    outAnim->setEndValue(0.0);
    outAnim->setEasingCurve(QEasingCurve::OutCubic);

    QObject::connect(outAnim, &QPropertyAnimation::finished, this,
                     [this, stack, newIndex, current]() {
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
                         QObject::connect(inAnim, &QPropertyAnimation::finished, incoming,
                                          [incoming]() { incoming->setGraphicsEffect(nullptr); });
                         inAnim->start(QAbstractAnimation::DeleteWhenStopped);
                     });
    outAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::animateSidebarToggle(bool collapse)
{
    int from = ui->sidebar->width();
    int to   = collapse ? 48 : 200;

    auto* anim = new QPropertyAnimation(ui->sidebar, "minimumWidth", this);
    anim->setDuration(220);
    anim->setStartValue(from);
    anim->setEndValue(to);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    QObject::connect(anim, &QPropertyAnimation::valueChanged, this,
                     [this](const QVariant&) {
                         ui->sidebar->setMaximumWidth(ui->sidebar->minimumWidth());
                     });
    QObject::connect(anim, &QPropertyAnimation::finished, this,
                     [this, collapse]() { m_sidebarCollapsed = collapse; });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::setupInteractiveHooks()
{
    if (auto* headerLayout = ui->centralwidget->findChild<QHBoxLayout*>(
            QStringLiteral("logoandnamesidebar"))) {
        auto* toggleBtn = new QToolButton(ui->sidebar);
        toggleBtn->setAutoRaise(true);
        toggleBtn->setToolTip(tr("Réduire/Agrandir la sidebar"));
        toggleBtn->setIcon(QIcon(QStringLiteral(":/img/menu.svg")));
        toggleBtn->setIconSize(QSize(18, 18));
        headerLayout->addStretch();
        headerLayout->addWidget(toggleBtn);
        QObject::connect(toggleBtn, &QToolButton::clicked, this,
                         [this]() { animateSidebarToggle(!m_sidebarCollapsed); });
    }

    if (ui->lineEdit && ui->comboBox && ui->tableWidget) {
        QObject::connect(ui->lineEdit, &QLineEdit::textChanged,
                         this, [this](const QString&) { filterPersonnelTable(); });
        QObject::connect(ui->comboBox, &QComboBox::currentTextChanged,
                         this, [this](const QString&) { filterPersonnelTable(); });
    }
}

void MainWindow::filterPersonnelTable()
{
    if (!ui->tableWidget || !ui->comboBox) return;
    QString needle = ui->lineEdit ? ui->lineEdit->text().trimmed() : QString();
    QString mode   = ui->comboBox->currentText();
    int col = 0;
    if      (mode.compare("Name",   Qt::CaseInsensitive) == 0 ||
        mode.compare("Nom",    Qt::CaseInsensitive) == 0) col = 1;
    else if (mode.compare("Status", Qt::CaseInsensitive) == 0) col = 2;
    else col = 0;

    for (int r = 0; r < ui->tableWidget->rowCount(); ++r) {
        auto* item = ui->tableWidget->item(r, col);
        bool match = needle.isEmpty() ||
                     (item && item->text().contains(needle, Qt::CaseInsensitive));
        ui->tableWidget->setRowHidden(r, !match);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GRAPHIQUES PERSONNEL
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setupPersonnelChart()
{
    QPieSeries *series = new QPieSeries();
    series->append("Actifs",    42);
    series->append("En congé",   8);
    series->append("Suspendus",  3);

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

    // Bar chart
    QBarSet *setActifs = new QBarSet("Actifs");
    QBarSet *setConge  = new QBarSet("En congé");
    QBarSet *setSusp   = new QBarSet("Suspendus");
    *setActifs << 18 << 15 << 9;
    *setConge  <<  3 <<  2 << 3;
    *setSusp   <<  1 <<  1 << 1;

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

    // Line chart
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

// ─────────────────────────────────────────────────────────────────────────────
// TABLE PERSONNEL
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setupPersonnelTable()
{
    if (!ui->tableWidget) return;
    QTableWidget* table = ui->tableWidget;

    if (table->columnCount() > 0) {
        int last = table->columnCount() - 1;
        auto* hLast = table->horizontalHeaderItem(last);
        if (hLast && hLast->text().trimmed().compare("Actions", Qt::CaseInsensitive) == 0) {
            for (int r = 0; r < table->rowCount(); ++r)
                if (!table->cellWidget(r, last)) addActionButtonsToRow(r);
            return;
        }
    }

    int actionsCol = table->columnCount();
    table->insertColumn(actionsCol);

    QStringList headers;
    for (int c = 0; c < table->columnCount(); ++c) {
        if (c == actionsCol) {
            headers << "Actions";
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
    if (!ui->tableWidget) return;
    auto* table = ui->tableWidget;

    QWidget* container = new QWidget(table);
    auto* h = new QHBoxLayout(container);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(6);
    h->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    container->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    QPushButton* btnModify = new QPushButton(QString(), container);
    btnModify->setProperty("type", "warning");
    btnModify->setObjectName("modifyBtn");
    btnModify->setFocusPolicy(Qt::NoFocus);
    btnModify->setToolTip(tr("Modifier"));
    btnModify->setIcon(QIcon(":/img/edit.svg"));
    btnModify->setIconSize(QSize(16, 16));
    btnModify->setFixedSize(28, 24);

    QPushButton* btnDelete = new QPushButton(QString(), container);
    btnDelete->setProperty("type", "danger");
    btnDelete->setObjectName("deleteBtn");
    btnDelete->setFocusPolicy(Qt::NoFocus);
    btnDelete->setToolTip(tr("Supprimer"));
    btnDelete->setIcon(QIcon(":/img/delete.svg"));
    btnDelete->setIconSize(QSize(16, 16));
    btnDelete->setFixedSize(28, 24);

    h->addWidget(btnModify);
    h->addWidget(btnDelete);
    container->setLayout(h);

    table->setCellWidget(row, table->columnCount() - 1, container);
    table->setRowHeight(row, 34);

    connect(btnModify, &QPushButton::clicked, this, [this]() {
        int r = findRowForButton(sender());
        if (r < 0) return;
        ui->tableWidget->selectRow(r);
        QMessageBox::information(this, tr("Modifier"), tr("Modifier la ligne %1").arg(r + 1));
    });

    connect(btnDelete, &QPushButton::clicked, this, [this]() {
        int r = findRowForButton(sender());
        if (r < 0) return;
        auto reply = QMessageBox::question(this, tr("Supprimer"),
                                           tr("Supprimer la ligne %1 ?").arg(r + 1));
        if (reply == QMessageBox::Yes)
            ui->tableWidget->removeRow(r);
    });
}

int MainWindow::findRowForButton(QObject* button) const
{
    if (!button || !ui->tableWidget) return -1;
    auto* table = ui->tableWidget;
    for (int r = 0; r < table->rowCount(); ++r) {
        QWidget* cell = table->cellWidget(r, table->columnCount() - 1);
        if (!cell) continue;
        auto* mod = cell->findChild<QPushButton*>("modifyBtn");
        auto* del = cell->findChild<QPushButton*>("deleteBtn");
        if (mod == button || del == button) return r;
    }
    return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// RECONNAISSANCE FACIALE (UI uniquement)
// ─────────────────────────────────────────────────────────────────────────────

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

    auto* footer   = new QHBoxLayout();
    auto* status   = new QLabel(tr("Camera idle"), &dlg);
    auto* spacer   = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto* btnStart = new QPushButton(tr("Start"), &dlg);
    auto* btnClose = new QPushButton(tr("Close"), &dlg);
    btnStart->setProperty("type", "primary");

    footer->addWidget(status);
    footer->addItem(spacer);
    footer->addWidget(btnStart);
    footer->addWidget(btnClose);
    root->addLayout(footer);

    bool running = false;
    QObject::connect(btnStart, &QPushButton::clicked, &dlg,
                     [&, status, btnStart]() mutable {
                         running = !running;
                         status->setText(running ? tr("Camera running…") : tr("Camera idle"));
                         btnStart->setText(running ? tr("Stop") : tr("Start"));
                     });
    QObject::connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::reject);

    dlg.exec();
}
