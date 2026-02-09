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
#include <QGridLayout>
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
#include <QShortcut>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QGuiApplication>
#include <QScreen>
#include <QGraphicsDropShadowEffect>
#include <QEvent>
#include <QMouseEvent>

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

    // Add a soft drop shadow to the modules container for a card-like feel
    if (ui->modules) {
        auto* shadow = new QGraphicsDropShadowEffect(ui->modules);
        shadow->setBlurRadius(24);
        shadow->setOffset(0, 8);
        shadow->setColor(QColor(0, 0, 0, 40));
        ui->modules->setGraphicsEffect(shadow);
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
    // Initialize charts for other modules' statistics pages
    setupCiterneChart();
    setupStocksChart();
    setupQualiteChart();
    // Initialize the personnel table with action buttons
    setupPersonnelTable();
    setupActionsForAllTables();

    // Add sidebar toggle button (hamburger) and interaction hooks
    setupInteractiveHooks();

    // Ensure user info is anchored to the top-right of the modules area
    repositionUserInfo();
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

// Module 2 (Stocks) toolbar actions: map to metiersstocks pages
void MainWindow::on_btnConsulterstc_clicked()
{
    if (ui->modules->currentIndex() != 5)
        crossFadeToIndex(ui->modules, 5);
    crossFadeToIndex(ui->metiersstocks, 1); // consulterqtolives
}

void MainWindow::on_btnAjouterstc_clicked()
{
    if (ui->modules->currentIndex() != 5)
        crossFadeToIndex(ui->modules, 5);
    crossFadeToIndex(ui->metiersstocks, 0); // ajoutqtolives
}

void MainWindow::on_btnStatstc_clicked()
{
    if (ui->modules->currentIndex() != 5)
        crossFadeToIndex(ui->modules, 5);
    crossFadeToIndex(ui->metiersstocks, 2); // statqtolives
}

void MainWindow::on_toolButton_5_clicked()
{
    if (ui->modules->currentIndex() != 5)
        crossFadeToIndex(ui->modules, 5);
    crossFadeToIndex(ui->metiersstocks, 3); // metieravancee_2
}

// Module 3 (Citernes) toolbar actions
void MainWindow::on_AjoutCiterne_clicked()
{
    if (ui->modules->currentIndex() != 1)
        crossFadeToIndex(ui->modules, 1);
    crossFadeToIndex(ui->metiersCiternes, 0); // ajoutCiternes
}

void MainWindow::on_ConsulterCiterne_clicked()
{
    if (ui->modules->currentIndex() != 1)
        crossFadeToIndex(ui->modules, 1);
    crossFadeToIndex(ui->metiersCiternes, 1); // consulterciterne
}

void MainWindow::on_StatistiqueCiterne_clicked()
{
    if (ui->modules->currentIndex() != 1)
        crossFadeToIndex(ui->modules, 1);
    crossFadeToIndex(ui->metiersCiternes, 2); // statCiterne
}

void MainWindow::on_MetierAvanceCiterne_clicked()
{
    if (ui->modules->currentIndex() != 1)
        crossFadeToIndex(ui->modules, 1);
    crossFadeToIndex(ui->metiersCiternes, 3); // AvCiterne
}

// Module 4 (Qualité) toolbar actions
void MainWindow::on_btnConsulterQualite_clicked()
{
    if (ui->modules->currentIndex() != 2)
        crossFadeToIndex(ui->modules, 2);
    crossFadeToIndex(ui->metiersqualite, 1); // consulterpersonnel_2
}

void MainWindow::on_btnAjouterQualite_clicked()
{
    if (ui->modules->currentIndex() != 2)
        crossFadeToIndex(ui->modules, 2);
    crossFadeToIndex(ui->metiersqualite, 0); // ajoutpersonnel_2
}

void MainWindow::on_btnStatQualite_clicked()
{
    if (ui->modules->currentIndex() != 2)
        crossFadeToIndex(ui->modules, 2);
    crossFadeToIndex(ui->metiersqualite, 2); // statPersonnel_2
}

void MainWindow::on_btnAdvEmp_2_clicked()
{
    if (ui->modules->currentIndex() != 2)
        crossFadeToIndex(ui->modules, 2);
    crossFadeToIndex(ui->metiersqualite, 3); // metieravancee_3
}

// Sidebar navigation: map buttons to modules indices
// Order in UI: module1 (0), module3 (1), module4 (2), module5 (3), module6 (4), module2 (5)
void MainWindow::on_btnmod1_clicked()
{
    ui->stackedWidget->setCurrentIndex(1); // ensure mainprogram
    crossFadeToIndex(ui->modules, 0);
    setActiveModuleButton(0);
}

void MainWindow::on_btnmod2_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    crossFadeToIndex(ui->modules, 5);
    setActiveModuleButton(5);
}

void MainWindow::on_btnmod3_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    crossFadeToIndex(ui->modules, 1);
    setActiveModuleButton(1);
}

void MainWindow::on_btnmod4_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    crossFadeToIndex(ui->modules, 2);
    setActiveModuleButton(2);
}

void MainWindow::on_btnmod5_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    crossFadeToIndex(ui->modules, 3);
    setActiveModuleButton(3);
}

void MainWindow::on_btnmod6_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    crossFadeToIndex(ui->modules, 4);
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

// Keep the user info container anchored at the top-right of the modules area
void MainWindow::repositionUserInfo()
{
    if (!ui->userInfoContainer || !ui->modules) return;
    // Maintain a fixed size for the user info block
    ui->userInfoContainer->setFixedSize(220, 56);
    QRect m = ui->modules->geometry();
    int x = m.right() - ui->userInfoContainer->width() - 10;
    int y = m.top() + 10;
    ui->userInfoContainer->setGeometry(x, y, ui->userInfoContainer->width(), ui->userInfoContainer->height());
    ui->userInfoContainer->raise();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    repositionUserInfo();
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    // Allow clicking the collapsed sidebar to expand it
    if (obj == ui->sidebar) {
        if (event->type() == QEvent::MouseButtonPress) {
            if (m_sidebarCollapsed || ui->sidebar->width() <= 60) {
                animateSidebarToggle(false); // expand
                return true; // consume to avoid accidental clicks on hidden children
            }
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            // Toggle on double-click when expanded
            animateSidebarToggle(ui->sidebar->width() > 60);
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// Sidebar collapse/expand animation
void MainWindow::animateSidebarToggle(bool collapse)
{
    // Animate the sidebar's minimum width and keep maximum in sync
    const int expandedMin = 200;
    const int expandedMax = 220;
    const int collapsedW  = 48;
    int from = ui->sidebar->minimumWidth();
    int to = collapse ? collapsedW : expandedMin; // collapsed vs expanded target widths
    auto* anim = new QPropertyAnimation(ui->sidebar, "minimumWidth", this);
    anim->setDuration(220);
    anim->setStartValue(from);
    anim->setEndValue(to);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(anim, &QPropertyAnimation::valueChanged, this, [this](const QVariant&){
        // keep max width in sync to avoid layout jitter
        ui->sidebar->setMaximumWidth(ui->sidebar->minimumWidth());
    });
    QObject::connect(anim, &QPropertyAnimation::finished, this, [this, collapse, expandedMin, expandedMax, collapsedW]() {
        // Snap constraints to target state to allow restoring original size range when expanded
        if (collapse) {
            ui->sidebar->setMinimumWidth(collapsedW);
            ui->sidebar->setMaximumWidth(collapsedW);
        } else {
            ui->sidebar->setMinimumWidth(expandedMin);
            ui->sidebar->setMaximumWidth(expandedMax);
        }
        m_sidebarCollapsed = collapse;
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// Hook up interactive behaviors (toggle button, live search)
void MainWindow::setupInteractiveHooks()
{
    // Sidebar toggle button injected into the header layout inside the sidebar
    if (auto* headerLayout = ui->sidebar->findChild<QHBoxLayout*>(QStringLiteral("logoandnamesidebar"))) {
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

    // Install event filter on sidebar to allow click-to-expand
    if (ui->sidebar) {
        ui->sidebar->installEventFilter(this);
        // Adjust cursor when collapsed
        QObject::connect(this, &MainWindow::windowTitleChanged, this, [this](const QString&){
            // Use an existing signal to update cursor opportunistically
            if (m_sidebarCollapsed || ui->sidebar->width() <= 60)
                ui->sidebar->setCursor(Qt::PointingHandCursor);
            else
                ui->sidebar->unsetCursor();
        });
    }

    // Keyboard shortcut to toggle sidebar (Ctrl+B)
    auto* toggleShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_B), this);
    QObject::connect(toggleShortcut, &QShortcut::activated, this, [this]() {
        animateSidebarToggle(!m_sidebarCollapsed);
    });

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
    
    // Create grid layout if it doesn't exist and add charts side-by-side
    QGridLayout *grid = qobject_cast<QGridLayout*>(ui->chartStatusContainer->layout());
    if (!grid) {
        grid = new QGridLayout(ui->chartStatusContainer);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(12);
        grid->setVerticalSpacing(12);
    }

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

    // Size policies and minimum heights for better presence
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    barView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    lineView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartView->setMinimumHeight(260);
    barView->setMinimumHeight(260);
    lineView->setMinimumHeight(260);

    // Place pie and bar side-by-side, line spanning full width below
    grid->addWidget(chartView, 0, 0);
    grid->addWidget(barView,   0, 1);
    grid->addWidget(lineView,  1, 0, 1, 2);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
}

void MainWindow::setupCiterneChart()
{
    if (!ui->chartStatusContainer_3) return;
    QGridLayout *grid = qobject_cast<QGridLayout*>(ui->chartStatusContainer_3->layout());
    if (!grid) {
        grid = new QGridLayout(ui->chartStatusContainer_3);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(12);
        grid->setVerticalSpacing(12);
    } else if (grid->count() > 0) {
        return; // already populated
    }

    // Pie: Répartition de l'état des citernes
    QPieSeries *etatSeries = new QPieSeries();
    etatSeries->append("Opérationnel", 12);
    etatSeries->append("Maintenance", 3);
    etatSeries->append("Hors service", 1);
    QChart *etatChart = new QChart();
    etatChart->addSeries(etatSeries);
    etatChart->setTitle("État des citernes");
    etatChart->legend()->setAlignment(Qt::AlignRight);
    QChartView *etatView = new QChartView(etatChart);
    etatView->setRenderHint(QPainter::Antialiasing);

    // Bar: Capacités par type d'huile
    QBarSet *setOlive = new QBarSet("Olive");
    QBarSet *setTournesol = new QBarSet("Tournesol");
    QBarSet *setColza = new QBarSet("Colza");
    *setOlive << 50 << 40 << 30;
    *setTournesol << 20 << 25 << 15;
    *setColza << 10 << 12 << 8;
    QBarSeries *capSeries = new QBarSeries();
    capSeries->append(setOlive);
    capSeries->append(setTournesol);
    capSeries->append(setColza);
    QChart *capChart = new QChart();
    capChart->addSeries(capSeries);
    capChart->setTitle("Capacité totale par type");
    capChart->setAnimationOptions(QChart::AllAnimations);
    QStringList cats; cats << "Nord" << "Centre" << "Sud";
    QBarCategoryAxis *axisX = new QBarCategoryAxis(); axisX->append(cats);
    QValueAxis *axisY = new QValueAxis(); axisY->setTitleText("m³"); axisY->setRange(0, 80);
    capChart->addAxis(axisX, Qt::AlignBottom); capChart->addAxis(axisY, Qt::AlignLeft);
    capSeries->attachAxis(axisX); capSeries->attachAxis(axisY);
    capChart->legend()->setAlignment(Qt::AlignBottom);
    QChartView *capView = new QChartView(capChart);
    capView->setRenderHint(QPainter::Antialiasing);

    // Line: Température moyenne (semaine)
    QLineSeries *tempSeries = new QLineSeries();
    tempSeries->setName("Température moyenne");
    tempSeries->append(0, 18); tempSeries->append(1, 19); tempSeries->append(2, 20);
    tempSeries->append(3, 21); tempSeries->append(4, 20); tempSeries->append(5, 19);
    QChart *tempChart = new QChart();
    tempChart->addSeries(tempSeries);
    tempChart->setTitle("Température (7 jours)");
    tempChart->setAnimationOptions(QChart::AllAnimations);
    QValueAxis *tx = new QValueAxis(); tx->setTitleText("Jour"); tx->setRange(0, 6);
    QValueAxis *ty = new QValueAxis(); ty->setTitleText("°C"); ty->setRange(16, 24);
    tempChart->addAxis(tx, Qt::AlignBottom); tempChart->addAxis(ty, Qt::AlignLeft);
    tempSeries->attachAxis(tx); tempSeries->attachAxis(ty);
    tempChart->legend()->setVisible(false);
    QChartView *tempView = new QChartView(tempChart);
    tempView->setRenderHint(QPainter::Antialiasing);

    // Size policies and minima
    etatView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    capView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tempView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    etatView->setMinimumHeight(260);
    capView->setMinimumHeight(260);
    tempView->setMinimumHeight(260);

    // Arrange: two charts side-by-side, one full width below
    grid->addWidget(etatView, 0, 0);
    grid->addWidget(capView,  0, 1);
    grid->addWidget(tempView, 1, 0, 1, 2);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
}

void MainWindow::setupStocksChart()
{
    if (!ui->chartStatusContainer_2) return;
    QGridLayout *grid2 = qobject_cast<QGridLayout*>(ui->chartStatusContainer_2->layout());
    if (!grid2) {
        grid2 = new QGridLayout(ui->chartStatusContainer_2);
        // Push charts below the existing filter controls
        grid2->setContentsMargins(0, 140, 0, 0);
        grid2->setHorizontalSpacing(12);
        grid2->setVerticalSpacing(12);
    } else if (grid2->count() > 0) {
        return;
    }

    // Pie: Répartition des stocks par catégorie
    QPieSeries *catSeries = new QPieSeries();
    catSeries->append("Extra", 30);
    catSeries->append("Fine", 25);
    catSeries->append("Standard", 35);
    catSeries->append("Déclassé", 10);
    QChart *catChart = new QChart();
    catChart->addSeries(catSeries);
    catChart->setTitle("Stocks par catégorie");
    catChart->legend()->setAlignment(Qt::AlignRight);
    QChartView *catView = new QChartView(catChart);
    catView->setRenderHint(QPainter::Antialiasing);

    // Bar: Entrées vs sorties (mois)
    QBarSet *entrees = new QBarSet("Entrées");
    QBarSet *sorties = new QBarSet("Sorties");
    *entrees << 120 << 150 << 130 << 160 << 140;
    *sorties << 100 << 140 << 120 << 150 << 130;
    QBarSeries *flowSeries = new QBarSeries();
    flowSeries->append(entrees); flowSeries->append(sorties);
    QChart *flowChart = new QChart();
    flowChart->addSeries(flowSeries);
    flowChart->setTitle("Flux mensuels");
    flowChart->setAnimationOptions(QChart::AllAnimations);
    QStringList mois; mois << "Jan" << "Fév" << "Mar" << "Avr" << "Mai";
    QBarCategoryAxis *fx = new QBarCategoryAxis(); fx->append(mois);
    QValueAxis *fy = new QValueAxis(); fy->setTitleText("Qté"); fy->setRange(0, 200);
    flowChart->addAxis(fx, Qt::AlignBottom); flowChart->addAxis(fy, Qt::AlignLeft);
    flowSeries->attachAxis(fx); flowSeries->attachAxis(fy);
    flowChart->legend()->setAlignment(Qt::AlignBottom);
    QChartView *flowView = new QChartView(flowChart);
    flowView->setRenderHint(QPainter::Antialiasing);

    catView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    flowView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    catView->setMinimumHeight(280);
    flowView->setMinimumHeight(280);
    // Arrange side-by-side in one row
    grid2->addWidget(catView,  0, 0);
    grid2->addWidget(flowView, 0, 1);
    grid2->setColumnStretch(0, 1);
    grid2->setColumnStretch(1, 1);
    grid2->setRowStretch(0, 1);
}

void MainWindow::setupQualiteChart()
{
    if (!ui->chartStatusContainer_4) return;
    QGridLayout *grid4 = qobject_cast<QGridLayout*>(ui->chartStatusContainer_4->layout());
    if (!grid4) {
        grid4 = new QGridLayout(ui->chartStatusContainer_4);
        // Leave room for top controls (label, combobox, table snippet)
        grid4->setContentsMargins(0, 170, 0, 0);
        grid4->setHorizontalSpacing(12);
        grid4->setVerticalSpacing(12);
    } else if (grid4->count() > 0) {
        return;
    }

    // Pie: Notes qualité
    QPieSeries *notes = new QPieSeries();
    notes->append("A", 40);
    notes->append("B", 35);
    notes->append("C", 20);
    notes->append("D", 5);
    QChart *notesChart = new QChart();
    notesChart->addSeries(notes);
    notesChart->setTitle("Distribution des notes");
    notesChart->legend()->setAlignment(Qt::AlignRight);
    QChartView *notesView = new QChartView(notesChart);
    notesView->setRenderHint(QPainter::Antialiasing);

    // Line: Tendance de la qualité (mois)
    QLineSeries *qualTrend = new QLineSeries();
    qualTrend->setName("Indice qualité");
    qualTrend->append(0, 82); qualTrend->append(1, 84); qualTrend->append(2, 83);
    qualTrend->append(3, 85); qualTrend->append(4, 87); qualTrend->append(5, 88);
    QChart *trendChart = new QChart();
    trendChart->addSeries(qualTrend);
    trendChart->setTitle("Tendance qualité (semestre)");
    trendChart->setAnimationOptions(QChart::AllAnimations);
    QValueAxis *qx = new QValueAxis(); qx->setTitleText("Mois"); qx->setRange(0, 5); qx->setTickCount(6);
    QValueAxis *qy = new QValueAxis(); qy->setTitleText("Indice"); qy->setRange(75, 95);
    trendChart->addAxis(qx, Qt::AlignBottom); trendChart->addAxis(qy, Qt::AlignLeft);
    qualTrend->attachAxis(qx); qualTrend->attachAxis(qy);
    trendChart->legend()->setVisible(false);
    QChartView *trendView = new QChartView(trendChart);
    trendView->setRenderHint(QPainter::Antialiasing);

    notesView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    trendView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    notesView->setMinimumHeight(280);
    trendView->setMinimumHeight(280);
    // Arrange side-by-side in one row
    grid4->addWidget(notesView,  0, 0);
    grid4->addWidget(trendView, 0, 1);
    grid4->setColumnStretch(0, 1);
    grid4->setColumnStretch(1, 1);
    grid4->setRowStretch(0, 1);
}

void MainWindow::setupPersonnelTable()
{
    if (!ui->tableWidget) return;
    addActionsColumnTo(ui->tableWidget);
}

void MainWindow::setupActionsForAllTables()
{
    // Find all QTableWidgets in the UI and apply the actions column
    const auto tables = this->findChildren<QTableWidget*>();
    for (QTableWidget* tbl : tables) {
        if (!tbl) continue;
        addActionsColumnTo(tbl);
    }
}

void MainWindow::addActionButtonsToRow(QTableWidget* table, int row)
{
    if (!table) return;

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
        QTableWidget* t = findOwningTable(sender());
        if (!t) return;
        int row = findRowForButton(t, sender());
        if (row < 0) return;
        t->selectRow(row);
        QMessageBox::information(this, tr("Modifier"), tr("Modifier la ligne %1").arg(row + 1));
    });

    connect(btnDelete, &QPushButton::clicked, this, [this]() {
        QTableWidget* t = findOwningTable(sender());
        if (!t) return;
        int row = findRowForButton(t, sender());
        if (row < 0) return;
        auto reply = QMessageBox::question(this, tr("Supprimer"), tr("Supprimer la ligne %1 ?").arg(row + 1));
        if (reply == QMessageBox::Yes) {
            t->removeRow(row);
        }
    });
}

int MainWindow::findRowForButton(QTableWidget* table, QObject* button) const
{
    if (!button || !table) return -1;

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

QTableWidget* MainWindow::findOwningTable(QObject* child) const
{
    QObject* p = child;
    while (p) {
        if (auto* tbl = qobject_cast<QTableWidget*>(p)) return tbl;
        p = p->parent();
    }
    return nullptr;
}

void MainWindow::addActionsColumnTo(QTableWidget* table)
{
    if (!table) return;

    // If Actions is already the last column, ensure rows have buttons and return
    if (table->columnCount() > 0) {
        int last = table->columnCount() - 1;
        auto* hLast = table->horizontalHeaderItem(last);
        if (hLast && hLast->text().trimmed().compare(QStringLiteral("Actions"), Qt::CaseInsensitive) == 0) {
            for (int r = 0; r < table->rowCount(); ++r) {
                if (!table->cellWidget(r, last)) addActionButtonsToRow(table, r);
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

    // Row presentation tweaks
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    if (table->verticalHeader()) {
        table->verticalHeader()->setDefaultSectionSize(32);
    }

    // Create action buttons for each existing row
    for (int r = 0; r < table->rowCount(); ++r) {
        addActionButtonsToRow(table, r);
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

