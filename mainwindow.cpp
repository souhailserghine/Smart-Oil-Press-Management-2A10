#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "citernes.h"
#include "stocks.h"
#include "employe.h"
#include "face_capture_dialog.h"
#include "face_recognition_dialog.h"
#include "face_recognition_service.h"
#include "fingerprintservice.h"
#include "agriculteurmodule.h"
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
#include <QAction>
#include <QBrush>
#include <QFont>
#include <QLayout>
#include <QLayoutItem>
#include <QPainter>
#include <QPixmap>
#include <QBuffer>
#include <QImage>
#include <QImageReader>
#include <QSpacerItem>
#include <QVector>
#include <QPair>
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
#include <QDir>
#include <QDataStream>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QMetaType>
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
#include <QScrollBar>
#include <QPrinter>
#include <QTextDocument>
#include <QDateTime>
#include <QMap>
#include <QPen>
#include <QPageSize>
#include <QPageLayout>
#include <QCheckBox>
#include <QFrame>
#include <QStatusBar>
#include <QTimer>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <algorithm>
#include <cmath>
#include <limits>


namespace {

QPixmap pixmapFromImageBytesRespectingOrientation(const QByteArray& bytes)
{
    QPixmap pixmap;
    if (bytes.isEmpty()) return pixmap;

    // Employee photos commonly come from phones. Many phone JPEGs store the
    // real orientation in EXIF instead of rotating pixels. QPixmap::loadFromData
    // ignores that metadata, which can make the top-bar avatar appear rotated.
    QBuffer buffer;
    buffer.setData(bytes);
    if (buffer.open(QIODevice::ReadOnly)) {
        QImageReader reader(&buffer);
        reader.setAutoTransform(true); // apply EXIF orientation when available
        const QImage image = reader.read();
        if (!image.isNull())
            pixmap = QPixmap::fromImage(image);
    }

    // Fallback for unusual formats/drivers if QImageReader could not decode.
    if (pixmap.isNull())
        pixmap.loadFromData(bytes);
    return pixmap;
}

void setSafeGeometry(QWidget* widget, int x, int y, int w, int h)
{
    if (!widget || w <= 0 || h <= 0) return;
    widget->setGeometry(x, y, w, h);
}

void centerFixedChild(QWidget* parent, QWidget* child, int preferredW, int preferredH, int topY)
{
    if (!parent || !child) return;
    const int availableW = qMax(parent->width(), preferredW + 40);
    const int availableH = qMax(parent->height(), preferredH + topY + 40);
    const int w = qMin(preferredW, qMax(360, availableW - 80));
    const int h = qMin(preferredH, qMax(360, availableH - topY - 40));
    const int x = qMax(24, (availableW - w) / 2);
    child->setGeometry(x, topY, w, h);
}

void setReferenceTableMetrics(QTableWidget* table, int actionsColumn = -1)
{
    if (!table) return;
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setShowGrid(false);
    table->setWordWrap(false);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (table->verticalHeader()) {
        table->verticalHeader()->setVisible(false);
        table->verticalHeader()->setDefaultSectionSize(48);
    }
    if (table->horizontalHeader()) {
        table->horizontalHeader()->setMinimumHeight(46);
        table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
        table->horizontalHeader()->setStretchLastSection(false);
        const int last = table->columnCount() - 1;
        const int actionCol = (actionsColumn >= 0) ? actionsColumn : last;
        for (int c = 0; c < table->columnCount(); ++c) {
            if (c == actionCol)
                table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Fixed);
            else
                table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
        }
        if (actionCol >= 0 && actionCol < table->columnCount())
            table->setColumnWidth(actionCol, 210);
    }
}

QPushButton* createReferenceTableButton(const QString& text, const char* objectName, const QString& type, QWidget* parent)
{
    auto* button = new QPushButton(text, parent);
    button->setObjectName(QString::fromLatin1(objectName));
    button->setProperty("type", type);
    button->setProperty("role", "tableAction");
    button->setFocusPolicy(Qt::NoFocus);
    button->setMinimumSize(text.contains(QStringLiteral("Supprimer")) ? QSize(98, 30) : QSize(92, 30));
    button->setMaximumHeight(30);
    button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    return button;
}

void applyModuleLayoutRefinements(QWidget* module, QWidget* toolbar, QStackedWidget* stack)
{
    if (!module) return;
    if (auto* lay = qobject_cast<QVBoxLayout*>(module->layout())) {
        lay->setContentsMargins(18, 14, 18, 18);
        lay->setSpacing(12);
        if (QLayoutItem* headerItem = lay->itemAt(0)) {
            if (auto* header = headerItem->layout()) {
                header->setContentsMargins(0, 0, 0, 0);
                header->setSpacing(10);
            }
        }
    }
    if (toolbar) {
        toolbar->setMinimumSize(460, 64);
        toolbar->setMaximumWidth(560);
        toolbar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
    if (stack) {
        stack->setMinimumSize(980, 760);
        stack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
}

void applyRuntimeUxPolish(Ui::MainWindow* ui)
{
    if (!ui) return;

    applyModuleLayoutRefinements(ui->module2, ui->horizontalLayoutWidget_4, ui->metiersstocks);
    applyModuleLayoutRefinements(ui->module3, ui->horizontalLayoutWidget_5, ui->metiersCiternes);
    applyModuleLayoutRefinements(ui->module4, ui->horizontalLayoutWidget_6, ui->metiershuile);
    applyModuleLayoutRefinements(ui->module5, ui->horizontalLayoutWidget_7, ui->metierspersonnel_2);
    applyModuleLayoutRefinements(ui->module6, ui->horizontalLayoutWidget_8, ui->metiersagriculteurs);
    applyModuleLayoutRefinements(ui->module7, nullptr, nullptr);

    for (QLabel* title : { ui->label_2, ui->label_5, ui->label_huile, ui->label_7, ui->label_8, ui->label_settings_title }) {
        if (!title) continue;
        title->setMinimumHeight(42);
        title->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    for (QTableWidget* table : { ui->tableEmp, ui->tableWidget_2, ui->ListeCiterne, ui->tableWidget_4, ui->tablemachine, ui->affTable }) {
        if (table) setReferenceTableMetrics(table);
    }

    if (ui->tablemachine) ui->tablemachine->setColumnWidth(8, 210);
    if (ui->tableWidget_4 && ui->tableWidget_4->columnCount() > 0)
        ui->tableWidget_4->setColumnWidth(ui->tableWidget_4->columnCount() - 1, 210);
    if (ui->tableEmp && ui->tableEmp->columnCount() > 0)
        ui->tableEmp->setColumnWidth(ui->tableEmp->columnCount() - 1, 210);

    if (ui->chartStatusContainer) ui->chartStatusContainer->setMinimumHeight(1800);
}

void installReferenceModuleLayout(QWidget* module,
                                  QToolButton* icon,
                                  QLabel* title,
                                  QWidget* toolbar,
                                  QStackedWidget* stack)
{
    if (!module || module->layout()) return;

    auto* moduleLayout = new QVBoxLayout(module);
    moduleLayout->setContentsMargins(18, 14, 18, 18);
    moduleLayout->setSpacing(12);

    auto* headerRow = new QHBoxLayout();
    headerRow->setContentsMargins(0, 0, 0, 0);
    headerRow->setSpacing(10);

    if (icon) {
        icon->setParent(module);
        icon->setFixedSize(36, 36);
        icon->setIconSize(QSize(28, 28));
        icon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        headerRow->addWidget(icon, 0, Qt::AlignVCenter);
    }

    if (title) {
        title->setParent(module);
        title->setMinimumHeight(40);
        title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        title->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        headerRow->addWidget(title, 1, Qt::AlignVCenter);
    } else {
        headerRow->addStretch(1);
    }

    if (toolbar) {
        toolbar->setParent(module);
        toolbar->setMinimumWidth(430);
        toolbar->setMaximumWidth(520);
        toolbar->setMinimumHeight(64);
        toolbar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        headerRow->addWidget(toolbar, 0, Qt::AlignRight | Qt::AlignVCenter);
    }

    moduleLayout->addLayout(headerRow);

    if (stack) {
        stack->setParent(module);
        stack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        stack->setMinimumSize(920, 680);
        moduleLayout->addWidget(stack, 1);
    } else {
        moduleLayout->addStretch(1);
    }
}

void normalizeMainModuleGeometry(Ui::MainWindow* ui)
{
    if (!ui || !ui->modules) return;

    const int moduleW = qMax(980, ui->modules->width());
    const int moduleH = qMax(805, ui->modules->height());
    const int titleW = qMax(360, moduleW - 600);
    const int toolbarW = 465;
    const int sideMargin = 32;
    const int titleX = 70;
    const int titleY = 34;
    const int iconX = 30;
    const int iconY = 31;
    const int toolbarX = qMax(500, moduleW - toolbarW - sideMargin);
    const int toolbarY = 22;
    const int toolbarH = 64;
    const int stackX = 32;
    const int stackY = 112;
    const int stackW = qMax(920, moduleW - (stackX * 2));
    const int stackH = qMax(680, moduleH - stackY - 24);

    auto apply = [&](QWidget* module, QLabel* title, QToolButton* icon, QWidget* toolbar, QStackedWidget* stack) {
        if (!module) return;
        module->setMinimumSize(moduleW, moduleH);

        const bool layoutManaged = module->layout() != nullptr;
        if (!layoutManaged) {
            if (icon) setSafeGeometry(icon, iconX, iconY, 36, 36);
            if (title) setSafeGeometry(title, titleX, titleY, titleW, 38);
            if (toolbar) setSafeGeometry(toolbar, toolbarX, toolbarY, toolbarW, toolbarH);
            if (stack) stack->setGeometry(stackX, stackY, stackW, stackH);
        } else if (stack) {
            stack->setMinimumSize(920, 680);
        }

        if (stack) {
            stack->setMinimumSize(920, 680);
            for (int i = 0; i < stack->count(); ++i) {
                if (QWidget* page = stack->widget(i)) {
                    page->resize(stack->size());
                    page->setMinimumSize(stack->size());
                    if (QLayout* pageLayout = page->layout()) {
                        pageLayout->setContentsMargins(0, 0, 0, 0);
                        pageLayout->setSpacing(qMax(8, pageLayout->spacing()));
                        pageLayout->activate();
                    }
                }
            }
        }
    };

    // Do not restyle Personnel itself: it is the reference layout. Only keep the stats page scrollable below.
    if (ui->module1 && ui->metierspersonnel) {
        ui->metierspersonnel->setMinimumSize(920, 740);
        if (ui->chartStatusContainer) ui->chartStatusContainer->setMinimumHeight(1550);
    }

    apply(ui->module2, ui->label_2, ui->module2Icon, ui->horizontalLayoutWidget_4, ui->metiersstocks);
    apply(ui->module3, ui->label_5, ui->module3Icon, ui->horizontalLayoutWidget_5, ui->metiersCiternes);
    apply(ui->module4, ui->label_huile, ui->module4Icon, ui->horizontalLayoutWidget_6, ui->metiershuile);
    apply(ui->module5, ui->label_7, ui->module5Icon, ui->horizontalLayoutWidget_7, ui->metierspersonnel_2);
    apply(ui->module6, ui->label_8, ui->module6Icon, ui->horizontalLayoutWidget_8, ui->metiersagriculteurs);
    apply(ui->module7, ui->label_settings_title, ui->module7Icon, nullptr, nullptr);

    // Center form-only pages so they no longer feel glued to the left.
    if (ui->ajoutqtolives && ui->formLayoutWidget_2)
        centerFixedChild(ui->ajoutqtolives, ui->formLayoutWidget_2, 760, 735, 20);
    if (ui->ajoutCiternes && ui->formLayoutWidget_3)
        centerFixedChild(ui->ajoutCiternes, ui->formLayoutWidget_3, 760, 540, 26);
    if (ui->ajouthuile) {
        const int pageW = qMax(980, ui->ajouthuile->width());
        const int gap = 28;
        const int panelW = qMin(430, qMax(360, (pageW - gap - 120) / 2));
        const int x1 = qMax(40, (pageW - (panelW * 2 + gap)) / 2);
        if (ui->formLayoutWidget_5) ui->formLayoutWidget_5->setGeometry(x1, 24, panelW, 560);
        if (ui->formLayoutWidget_4) ui->formLayoutWidget_4->setGeometry(x1 + panelW + gap, 24, panelW, 560);
    }
    if (ui->ajoutpersonnel_3 && ui->formLayoutWidget_6)
        centerFixedChild(ui->ajoutpersonnel_3, ui->formLayoutWidget_6, 760, 560, 28);
    // Module 6 is now handled by embedded AgriculteurModule widgets.

    // Advanced Citernes page contains a small absolute widget; keep it centered.
    if (ui->AvCiterne) {
        if (QWidget* panel = ui->AvCiterne->findChild<QWidget*>(QStringLiteral("layoutWidget"))) {
            setSafeGeometry(panel, qMax(24, (ui->AvCiterne->width() - 260) / 2), 32, 260, 44);
        }
    }

    applyRuntimeUxPolish(ui);
}
}



namespace {
QString premiumInterfaceTheme()
{
    return QStringLiteral(R"QSS(
/* ───────────────── Smart Oil Premium UI Theme ───────────────── */
QMainWindow,
QWidget#centralwidget,
QWidget#mainprogram,
QWidget#contentArea,
QStackedWidget#MainStacked {
    background: #f6f8f2;
    color: #25331f;
    font-family: "Segoe UI", "Inter", "Arial";
    font-size: 10.5pt;
}

QWidget#loginpage {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                stop:0 #eef4df,
                                stop:0.48 #f8faf3,
                                stop:1 #dfeac9);
}
QLabel#loginarea {
    background: rgba(255, 255, 255, 248);
    border: 1px solid rgba(126, 145, 82, 75);
    border-radius: 34px;
}
QLabel#loginpic {
    border-radius: 34px;
    background-position: center;
    background-repeat: no-repeat;
}
QLabel#logintext {
    background: transparent;
    color: #263414;
    font-size: 26px;
    font-weight: 900;
    letter-spacing: 1px;
}
QLineEdit#userinput,
QLineEdit#pwdinput {
    background: #ffffff;
    border: 1px solid #d7e2c6;
    border-radius: 16px;
    color: #243018;
    padding: 0 16px;
    min-height: 44px;
}
QLineEdit#userinput:focus,
QLineEdit#pwdinput:focus {
    border: 2px solid #7a9444;
    background: #fbfdf7;
}
QPushButton#loginbtn {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #6b7f3d,
                                stop:1 #95a95a);
    color: white;
    border: none;
    border-radius: 18px;
    padding: 10px 18px;
    font-weight: 800;
    min-height: 42px;
}
QPushButton#loginbtn:hover { background: #5f7334; }
QPushButton#faceBtn {
    background: #eef4df;
    color: #445922;
    border: 1px solid #d8e2c8;
    border-radius: 16px;
    padding: 9px 16px;
    font-weight: 700;
}
QPushButton#faceBtn:hover { background: #e4edcf; }

QWidget#sidebar {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                stop:0 #6f8343,
                                stop:0.55 #5f7438,
                                stop:1 #53672f);
    border-right: 1px solid rgba(68, 86, 36, 90);
}
QLabel#sidebartitle,
QLabel#sidebarsubtitle {
    color: #fbfff3;
    background: transparent;
}
QToolButton#sidebarToggleBtn {
    background: rgba(255, 255, 255, 34);
    border: 1px solid rgba(255, 255, 255, 70);
    border-radius: 12px;
    padding: 6px;
    color: #ffffff;
}
QToolButton#sidebarToggleBtn:hover {
    background: rgba(255, 255, 255, 58);
}
QPushButton#btnmod1,
QPushButton#btnmod2,
QPushButton#btnmod3,
QPushButton#btnmod4,
QPushButton#btnmod5,
QPushButton#btnmod6,
QPushButton#btnSettings {
    background: transparent;
    color: #fbfff3;
    border: 1px solid transparent;
    border-radius: 16px;
    padding: 10px 14px;
    text-align: left;
    font-weight: 750;
    min-height: 42px;
}
QPushButton#btnmod1:hover,
QPushButton#btnmod2:hover,
QPushButton#btnmod3:hover,
QPushButton#btnmod4:hover,
QPushButton#btnmod5:hover,
QPushButton#btnmod6:hover,
QPushButton#btnSettings:hover {
    background: rgba(255, 255, 255, 34);
    border: 1px solid rgba(255, 255, 255, 55);
}
QPushButton#btnmod1:checked,
QPushButton#btnmod2:checked,
QPushButton#btnmod3:checked,
QPushButton#btnmod4:checked,
QPushButton#btnmod5:checked,
QPushButton#btnmod6:checked,
QPushButton#btnSettings:checked {
    background: rgba(255, 255, 255, 72);
    border: 1px solid rgba(255, 255, 255, 100);
    color: #ffffff;
}

QWidget#userInfoContainer,
QGroupBox,
QFrame[role="card"],
QWidget#chartStatusContainer,
QWidget#chartStatusContainer_2,
QWidget#chartStatusContainer_3 {
    background: #ffffff;
    border: 1px solid #e0e8d4;
    border-radius: 20px;
}
QLabel#label_personnel,
QLabel#label_2,
QLabel#label_5,
QLabel#label_huile,
QLabel#label_7,
QLabel#label_8,
QLabel#label_settings_title,
QLabel#label_citerne,
QLabel#label_machine,
QLabel#label_agriculteur,
QLabel#label_stock {
    color: #263414;
    font-size: 22px;
    font-weight: 900;
    letter-spacing: 0.4px;
    background: transparent;
    padding-top: 2px;
    padding-bottom: 2px;
}

QPushButton {
    background: #e7ecd8;
    color: #263414;
    border: 1px solid #d9e4c7;
    border-radius: 14px;
    padding: 8px 16px;
    font-weight: 700;
    min-height: 32px;
}
QPushButton:hover { background: #dfe9cc; border-color: #c8d8ad; }
QPushButton:pressed { background: #cdddb4; }
QPushButton[type="primary"] {
    background: #667c3a;
    border-color: #667c3a;
    color: white;
}
QPushButton[type="primary"]:hover { background: #566d2d; }
QPushButton[type="warning"] {
    background: #e6a817;
    border-color: #e6a817;
    color: white;
}
QPushButton[type="danger"] {
    background: #c94a45;
    border-color: #c94a45;
    color: white;
}
QPushButton[type="success"] {
    background: #3f8f62;
    border-color: #3f8f62;
    color: white;
}
QPushButton[role="tableAction"],
QToolButton[role="tableAction"] {
    min-height: 0px;
    max-height: 30px;
    padding: 0px 10px;
    border-radius: 10px;
    font-size: 11px;
    font-weight: 800;
}
QToolButton[role="tableAction"] {
    background: #e7ecd8;
    color: #263414;
    border: 1px solid #d9e4c7;
}
QToolButton[role="tableAction"][type="warning"] {
    background: #e6a817;
    border-color: #e6a817;
    color: white;
}
QToolButton[role="tableAction"][type="danger"] {
    background: #c94a45;
    border-color: #c94a45;
    color: white;
}
QWidget[role="tableActionsContainer"] {
    background: transparent;
    border: none;
}

QToolButton {
    background: #ffffff;
    color: #415524;
    border: 1px solid #dde7d0;
    border-radius: 14px;
    padding: 8px 10px;
    font-weight: 700;
    min-height: 34px;
}
QToolButton:hover { background: #eff5e5; border-color: #c8d8ad; }
QToolButton:checked { background: #dfeacc; color: #263414; }
QToolButton[autoRaise="true"] { background: transparent; border: 1px solid transparent; }
QToolButton[autoRaise="true"]:hover { background: #eff5e5; border-color: #d7e3c6; }

QLineEdit,
QTextEdit,
QPlainTextEdit,
QComboBox,
QDateEdit,
QSpinBox,
QDoubleSpinBox {
    background: #ffffff;
    border: 1px solid #d9e2cd;
    border-radius: 12px;
    padding: 7px 10px;
    color: #25331f;
    selection-background-color: #80964b;
    selection-color: white;
    min-height: 32px;
}
QLineEdit:focus,
QTextEdit:focus,
QPlainTextEdit:focus,
QComboBox:focus,
QDateEdit:focus,
QSpinBox:focus,
QDoubleSpinBox:focus {
    border: 2px solid #7d9746;
    background: #fbfdf7;
}
QLineEdit[readOnly="true"] {
    background: #f2f5ec;
    color: #68715e;
}
QComboBox::drop-down {
    border: none;
    width: 26px;
}

QTableWidget,
QTableView {
    background: #ffffff;
    alternate-background-color: #f8faf4;
    border: 1px solid #dfe8d4;
    border-radius: 18px;
    gridline-color: #edf2e6;
    selection-background-color: #e5efd3;
    selection-color: #263414;
    outline: none;
}
QTableWidget::item,
QTableView::item {
    padding: 7px 8px;
    border-bottom: 1px solid #edf2e6;
}
QTableWidget::item:selected,
QTableView::item:selected {
    background: #e5efd3;
    color: #263414;
}
QHeaderView::section {
    background: #6b7f3d;
    color: white;
    border: none;
    border-right: 1px solid rgba(255,255,255,45);
    padding: 9px 8px;
    font-weight: 800;
}
QHeaderView::section:first {
    border-top-left-radius: 12px;
}
QHeaderView::section:last {
    border-top-right-radius: 12px;
}

QTabWidget::pane,
QStackedWidget#modules {
    border: none;
    background: transparent;
}
QScrollArea,
QScrollArea > QWidget > QWidget {
    background: transparent;
    border: none;
}
QGroupBox {
    margin-top: 18px;
    padding: 18px;
    font-weight: 800;
    color: #405323;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 18px;
    padding: 0 8px;
    color: #405323;
}
QCheckBox {
    spacing: 8px;
    color: #344521;
}
QStatusBar {
    background: #f6f8f2;
    color: #536143;
}
QToolTip {
    background: #263414;
    color: #ffffff;
    border:
