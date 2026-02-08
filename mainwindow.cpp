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
    // Initialize the personnel table with action buttons
    setupPersonnelTable();

    // Add sidebar toggle button (hamburger) and interaction hooks
    setupInteractiveHooks();
}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::on_loginbtn_clicked()
{
    // Switch to the 2nd page (index 1)
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::on_btnAjouterEmp_clicked()
{
    // Ensure we're on the personnel module first
    if (ui->modules->currentIndex() != 0)
        crossFadeToIndex(ui->modules, 0);
    // Switch to "Add Personnel" page (index 0)
    crossFadeToIndex(ui->metierspersonnel, 0);
}

void MainWindow::on_btnConsulterEmp_clicked()
{
    // Ensure we're on the personnel module first
    if (ui->modules->currentIndex() != 0)
        crossFadeToIndex(ui->modules, 0);
    // Switch to "View Personnel" page (index 1)
    crossFadeToIndex(ui->metierspersonnel, 1);
}

void MainWindow::on_btnStatEmp_clicked()
{
    // Ensure we're on the personnel module first
    if (ui->modules->currentIndex() != 0)
        crossFadeToIndex(ui->modules, 0);
    // Switch to "Statistics" page (index 2)
    crossFadeToIndex(ui->metierspersonnel, 2);
}

void MainWindow::on_btnAdvEmp_clicked()
{
    // Ensure we're on the personnel module first
    if (ui->modules->currentIndex() != 0)
        crossFadeToIndex(ui->modules, 0);
    // Switch to "Advanced" page (index 3)
    crossFadeToIndex(ui->metierspersonnel, 3);
}

// Sidebar navigation: map buttons to modules indices
// Order in UI: module1 (0), module3 (1), module4 (2), module5 (3), module6 (4), module2 (5)
void MainWindow::on_btnmod1_clicked()
{
    ui->stackedWidget->setCurrentIndex(1); // ensure mainprogram
    ui->modules->setCurrentIndex(0);
    setActiveModuleButton(0);
}

void MainWindow::on_btnmod2_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->modules->setCurrentIndex(5);
    setActiveModuleButton(5);
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

// Helper to visually mark active module button
void MainWindow::setActiveModuleButton(int index)
{
    // Map module index to button
    // modules: 0=module1, 1=module3, 2=module4, 3=module5, 4=module6, 5=module2
    // buttons: btnmod1..btnmod6
    QPushButton* buttons[6] = { ui->btnmod1, ui->btnmod2, ui->btnmod3, ui->btnmod4, ui->btnmod5, ui->btnmod6 };
    for (auto* b : buttons) {
        b->setChecked(false);
    }
    switch (index) {
    case 0: ui->btnmod1->setChecked(true); break; // module1
    case 1: ui->btnmod3->setChecked(true); break; // module3
    case 2: ui->btnmod4->setChecked(true); break; // module4
    case 3: ui->btnmod5->setChecked(true); break; // module5
    case 4: ui->btnmod6->setChecked(true); break; // module6
    case 5: ui->btnmod2->setChecked(true); break; // module2
    default: ui->btnmod1->setChecked(true); break;
    }
}

// Smooth fade transition for stacked widgets
void MainWindow::crossFadeToIndex(QStackedWidget* stack, int newIndex)
{
    if (!stack || newIndex < 0 || newIndex >= stack->count()) {
        return;
    }
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

// Sidebar collapse/expand animation
void MainWindow::animateSidebarToggle(bool collapse)
{
    int from = ui->sidebar->width();
    int to = collapse ? 48 : 200; // collapsed vs expanded target widths
    auto* anim = new QPropertyAnimation(ui->sidebar, "minimumWidth", this);
    anim->setDuration(220);
    anim->setStartValue(from);
    anim->setEndValue(to);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(anim, &QPropertyAnimation::valueChanged, this, [this](const QVariant&){
        // keep max width in sync to avoid layout jitter
        ui->sidebar->setMaximumWidth(ui->sidebar->minimumWidth());
    });
    QObject::connect(anim, &QPropertyAnimation::finished, this, [this, collapse]() {
        m_sidebarCollapsed = collapse;
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// Hook up interactive behaviors (toggle button, live search)
void MainWindow::setupInteractiveHooks()
{
    // Sidebar toggle button injected into the header layout
    if (auto* headerLayout = ui->centralwidget->findChild<QHBoxLayout*>(QStringLiteral("logoandnamesidebar"))) {
        auto* toggleBtn = new QToolButton(ui->sidebar);
        toggleBtn->setAutoRaise(true);
        toggleBtn->setToolTip(tr("Collapse/Expand sidebar"));
        toggleBtn->setIcon(QIcon(QStringLiteral(":/img/menu.svg")));
        toggleBtn->setIconSize(QSize(18,18));
        headerLayout->addStretch();
        headerLayout->addWidget(toggleBtn);
        QObject::connect(toggleBtn, &QToolButton::clicked, this, [this]() {
            animateSidebarToggle(!m_sidebarCollapsed);
        });
    }

    // Live filter for personnel table
    if (ui->lineEdit && ui->comboBox && ui->tableWidget) {
        QObject::connect(ui->lineEdit, &QLineEdit::textChanged, this, [this](const QString&){ filterPersonnelTable(); });
        QObject::connect(ui->comboBox, &QComboBox::currentTextChanged, this, [this](const QString&){ filterPersonnelTable(); });
    }
}

// Simple filter on the personnel table based on combo selection
void MainWindow::filterPersonnelTable()
{
    if (!ui->tableWidget || !ui->comboBox) return;
    QString needle = ui->lineEdit ? ui->lineEdit->text().trimmed() : QString();
    QString mode = ui->comboBox->currentText();
    int col = 0;
    if (mode.compare(QStringLiteral("Name"), Qt::CaseInsensitive) == 0 || mode.compare(QStringLiteral("Nom"), Qt::CaseInsensitive) == 0) col = 1;
    else if (mode.compare(QStringLiteral("Status"), Qt::CaseInsensitive) == 0) col = 2;
    else col = 0; // Date/Id fallback to first column

    for (int r = 0; r < ui->tableWidget->rowCount(); ++r) {
        auto* item = ui->tableWidget->item(r, col);
        bool match = needle.isEmpty() || (item && item->text().contains(needle, Qt::CaseInsensitive));
        ui->tableWidget->setRowHidden(r, !match);
    }
}

void MainWindow::setupPersonnelChart()
{
    // Create pie chart data
    QPieSeries *series = new QPieSeries();
    series->append("Actifs", 42);
    series->append("En congé", 8);
    series->append("Suspendus", 3);

    // Create and configure the chart
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Répartition des employés");
    chart->legend()->setAlignment(Qt::AlignRight);
    
    // Create chart view with antialiasing
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    
    // Create layout if it doesn't exist and add chart
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(ui->chartStatusContainer->layout());
    if (!layout) {
        layout = new QVBoxLayout(ui->chartStatusContainer);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(12);
    }
    
    layout->addWidget(chartView);

    // Add a stacked bar chart: status distribution across categories
    QBarSet *setActifs = new QBarSet("Actifs");
    QBarSet *setConge  = new QBarSet("En congé");
    QBarSet *setSusp   = new QBarSet("Suspendus");
    // Example data for three departments
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

    // Add a line chart: headcount trend over months
    QLineSeries *lineSeries = new QLineSeries();
    lineSeries->setName("Effectif total");
    // Example monthly totals
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
    if (!ui->tableWidget)
        return;

    QTableWidget* table = ui->tableWidget;

    // If Actions is already the last column, don't add again
    if (table->columnCount() > 0) {
        int last = table->columnCount() - 1;
        auto* hLast = table->horizontalHeaderItem(last);
        if (hLast && hLast->text().trimmed().compare(QStringLiteral("Actions"), Qt::CaseInsensitive) == 0) {
            // Ensure each row has action buttons present
            for (int r = 0; r < table->rowCount(); ++r) {
                if (!table->cellWidget(r, last)) addActionButtonsToRow(r);
            }
            return;
        }
    }

    // Append an Actions column at the far right
    int actionsCol = table->columnCount();
    table->insertColumn(actionsCol);

    // Build headers preserving existing titles and add Actions at end
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

    // Row presentation tweaks for better button fit
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    if (table->verticalHeader()) {
        table->verticalHeader()->setDefaultSectionSize(32);
    }

    // Create action buttons for each existing row
    for (int r = 0; r < table->rowCount(); ++r) {
        addActionButtonsToRow(r);
    }

    // Column sizing: compact width for Actions (last), stretch others
    if (table->horizontalHeader()) {
        table->horizontalHeader()->setStretchLastSection(false);
        int last = table->columnCount() - 1;
        for (int c = 0; c < last; ++c) {
            table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
        }
        table->horizontalHeader()->setSectionResizeMode(last, QHeaderView::ResizeToContents);
        table->setColumnWidth(last, 90);
    }
}

void MainWindow::addActionButtonsToRow(int row)
{
    if (!ui->tableWidget) return;
    auto* table = ui->tableWidget;

    // Container widget with horizontal layout to hold the two buttons
    QWidget* container = new QWidget(table);
    auto* h = new QHBoxLayout(container);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(6);
    h->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    container->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    // Modify (warning / yellow)
    QPushButton* btnModify = new QPushButton(QString(), container);
    btnModify->setProperty("type", "warning");
    btnModify->setProperty("size", "small");
    btnModify->setObjectName("modifyBtn");
    btnModify->setFocusPolicy(Qt::NoFocus);
    btnModify->setToolTip(tr("Modifier"));
    btnModify->setIcon(QIcon(QStringLiteral(":/img/edit.svg")));
    btnModify->setIconSize(QSize(16,16));
    btnModify->setFixedSize(28, 24);

    // Delete (danger / red)
    QPushButton* btnDelete = new QPushButton(QString(), container);
    btnDelete->setProperty("type", "danger");
    btnDelete->setProperty("size", "small");
    btnDelete->setObjectName("deleteBtn");
    btnDelete->setFocusPolicy(Qt::NoFocus);
    btnDelete->setToolTip(tr("Supprimer"));
    btnDelete->setIcon(QIcon(QStringLiteral(":/img/delete.svg")));
    btnDelete->setIconSize(QSize(16,16));
    btnDelete->setFixedSize(28, 24);

    h->addWidget(btnModify);
    h->addWidget(btnDelete);
    container->setLayout(h);

    table->setCellWidget(row, table->columnCount() - 1, container);
    table->setRowHeight(row, 34);

    // Connect actions using runtime row lookup to stay correct after deletions
    connect(btnModify, &QPushButton::clicked, this, [this]() {
        int row = findRowForButton(sender());
        if (row < 0) return;
        // Example behavior: show a quick info and select the row
        ui->tableWidget->selectRow(row);
        QMessageBox::information(this, tr("Modifier"), tr("Modifier la ligne %1").arg(row + 1));
    });

    connect(btnDelete, &QPushButton::clicked, this, [this]() {
        int row = findRowForButton(sender());
        if (row < 0) return;
        auto reply = QMessageBox::question(this, tr("Supprimer"), tr("Supprimer la ligne %1 ?").arg(row + 1));
        if (reply == QMessageBox::Yes) {
            ui->tableWidget->removeRow(row);
        }
    });
}

int MainWindow::findRowForButton(QObject* button) const
{
    if (!button || !ui->tableWidget) return -1;
    auto* table = ui->tableWidget;

    for (int r = 0; r < table->rowCount(); ++r) {
        QWidget* cell = table->cellWidget(r, table->columnCount() - 1);
        if (!cell) continue;
        // Look for either modify or delete child matching the sender
        auto* mod = cell->findChild<QPushButton*>("modifyBtn");
        auto* del = cell->findChild<QPushButton*>("deleteBtn");
        if (mod == button || del == button) {
            return r;
        }
    }
    return -1;
}

void MainWindow::on_faceBtn_clicked()
{
    // Simple modal dialog as the facial recognition interface (UI only)
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Facial Recognition"));
    dlg.resize(640, 420);

    auto* root = new QVBoxLayout(&dlg);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* heading = new QLabel(tr("Facial Recognition"), &dlg);
    heading->setProperty("type", "heading");
    root->addWidget(heading);

    // Preview placeholder
    auto* preview = new QWidget(&dlg);
    preview->setObjectName("facePreview");
    preview->setMinimumSize(560, 300);
    root->addWidget(preview, 1);

    // Status + controls
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

    // Behavior (UI only)
    bool running = false;
    QObject::connect(btnStart, &QPushButton::clicked, &dlg, [&, status, btnStart]() mutable {
        running = !running;
        status->setText(running ? tr("Camera running…") : tr("Camera idle"));
        btnStart->setText(running ? tr("Stop") : tr("Start"));
    });
    QObject::connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::reject);

    dlg.exec();
}

