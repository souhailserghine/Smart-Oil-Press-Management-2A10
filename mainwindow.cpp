#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "employe.h"
#include <QFileDialog>
#include <QFile>
#include <QPixmap>
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
#include <QGridLayout>
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
#include <QEvent>
#include <algorithm>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QStackedWidget>
#include <QFrame>
#include <QSqlQuery>
#include <QSqlError>
#include <QtCharts/QPieSlice>
#include <QMap>
#include <QFont>
#include <QPdfWriter>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextTable>
#include <QTextTableFormat>
#include <QTextCharFormat>
#include <QTextBlockFormat>
#include <QMenu>
#include <QDir>
#include <QCursor>
#include <QGraphicsDropShadowEffect>
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>

#include "face_recognition_service.h"
#include "face_recognition_dialog.h"
#include "face_capture_dialog.h"

#include <QTimer>
#include <QDateTime>
#include <QRegularExpression>
#include <QDataStream>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QSqlRecord>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Facial recognition moved out of MainWindow (keeps UI file cleaner)
    m_faceService = new FaceRecognitionService();
    m_faceService->ensureModelsLoaded();
    
    // Initialize fingerprint service (handles Arduino communication)
    initFingerprintService();

    // Ensure the avatar image is rendered as a circle
    makeAvatarCircular();
    
    // Layout the sidebar and modules side-by-side to avoid overlap
    {
        auto* mainLayout = ui->mainprogramLayout;
        auto* contentArea = ui->contentArea;
        auto* contentLayout = ui->contentLayout;
        if (!mainLayout || !contentArea || !contentLayout) {
            qWarning() << "mainprogram/content layouts are missing in UI";
        } else {
            if (ui->sidebar && mainLayout->indexOf(ui->sidebar) < 0) {
                mainLayout->addWidget(ui->sidebar);
            }
            // Place the user info bar above modules within the content area so it's layout-managed
            if (ui->userInfoContainer && contentLayout->indexOf(ui->userInfoContainer) < 0) {
                contentLayout->addWidget(ui->userInfoContainer);
            }

            // Modules sit below the user info bar
            if (ui->modules && contentLayout->indexOf(ui->modules) < 0) {
                contentLayout->addWidget(ui->modules);
            }
            if (mainLayout->indexOf(contentArea) < 0) {
                mainLayout->addWidget(contentArea);
            }
        }
    }

    // Floating chat launcher is owned by .ui.
    if (!m_chatLauncher && ui->chatLauncher) {
        m_chatLauncher = ui->chatLauncher;

        m_chatLauncher->setIcon(QIcon(QStringLiteral(":/img/chat.svg")));
        m_chatLauncher->setIconSize(QSize(24,24));
        m_chatLauncher->setToolTip(tr("Ouvrir le chat"));
        m_chatLauncher->setAutoRaise(false);
        m_chatLauncher->setFixedSize(48, 48);
        m_chatLauncher->raise();

        QObject::connect(m_chatLauncher, &QToolButton::clicked, this, [this]() {
            QMessageBox::information(this, tr("Chat"),
                                     tr("Le chat n'est pas encore implémenté dans cette version."));
        });
    }

    // Add a soft drop shadow to the modules container for a card-like feel
    if (ui->modules) {
        auto* shadow = new QGraphicsDropShadowEffect(ui->modules);
        shadow->setBlurRadius(24);
        shadow->setOffset(0, 8);
        shadow->setColor(QColor(0, 0, 0, 40));
        ui->modules->setGraphicsEffect(shadow);
    }

    // Centered system time/date at the bottom in the status bar (.ui-owned widgets)
    if (!m_clockLabel && ui->clockLabel && ui->clockLeftSpacer && ui->clockRightSpacer) {
        m_clockLabel = ui->clockLabel;
        m_clockLeftSpacer = ui->clockLeftSpacer;
        m_clockRightSpacer = ui->clockRightSpacer;

        m_clockLabel->setMinimumWidth(140);
        m_clockLabel->setAlignment(Qt::AlignCenter);
        m_clockLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        m_clockLeftSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        m_clockRightSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        ui->statusbar->addPermanentWidget(m_clockLeftSpacer, 1);
        ui->statusbar->addPermanentWidget(m_clockLabel, 0);
        ui->statusbar->addPermanentWidget(m_clockRightSpacer, 1);

        // Update every second
        m_clockTimer = new QTimer(this);
        QObject::connect(m_clockTimer, &QTimer::timeout, this, &MainWindow::updateClock,
                         Qt::UniqueConnection);
        m_clockTimer->start(1000);
        updateClock();
    }

    // Sidebar button checkable state now lives in .ui; keep initial selection in code.
    setActiveModuleButton(0);

    // Toolbar rows are fixed and fit the available width; no runtime adjustments needed.



    // Wrap tall stacked pages in scroll areas so bottom action rows (e.g., qjouter*) are always accessible
    auto wrapStackPagesInScroll = [](QStackedWidget* sw){
        if (!sw) return;
        for (int i = 0; i < sw->count(); ++i) {
            QWidget* page = sw->widget(i);
            if (!page) continue;
            const bool hasLayout = (page->layout() != nullptr);
            // If this stacked page is already a QScrollArea, skip wrapping
            // (previous logic checked parent type and could re-wrap scroll areas, hiding content)
            if (qobject_cast<QScrollArea*>(page)) continue;
            auto* sa = new QScrollArea(sw);
            sa->setFrameShape(QFrame::NoFrame);
            // Layout-based pages can resize with viewport; absolute-geometry pages must keep
            // their designed size so scrollbars appear instead of squashing controls/text.
            sa->setWidgetResizable(hasLayout);
            sa->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            sa->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            // Add bottom padding to avoid clipping of the last action row (e.g., Ajouter)
            sa->viewport()->setContentsMargins(0, 0, 0, 32);
            // Ensure forms and page content have a white background inside module pages
            sa->setStyleSheet(
                "QScrollArea { background: #ffffff; }\n"
                "QScrollArea > QWidget#qt_scrollarea_viewport { background: #ffffff; }"
            );
            // Move the existing page into the scroll area
            sw->removeWidget(page);
            if (hasLayout) {
                // Layout-driven page: expand with viewport.
                page->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            } else {
                // Absolute-geometry page: preserve designed content bounds, then scroll when needed.
                // `size()`/`sizeHint()` can be too small before first show; derive from children geometry.
                QRect contentBounds;
                const auto children = page->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
                for (QWidget* child : children) {
                    if (!child) continue;
                    contentBounds = contentBounds.united(child->geometry());
                }

                QSize base;
                if (!contentBounds.isNull()) {
                    // Add a little breathing room so placeholders/labels don't look clipped.
                    // Use right/bottom extents (position + size), not only bounds.size(),
                    // otherwise pages with offset children (x>0/y>0) get unintentionally shrunk.
                    const int requiredW = contentBounds.right() + 1 + 24;
                    const int requiredH = contentBounds.bottom() + 1 + 32;
                    base = QSize(requiredW, requiredH);
                } else {
                    base = page->size().isValid() ? page->size() : page->sizeHint();
                }

                // Never shrink below stacked-widget viewport width.
                base.setWidth(qMax(base.width(), sw->width()));
                base.setHeight(qMax(base.height(), page->height()));

                if (base.width() > 0 && base.height() > 0) {
                    page->setMinimumSize(base);
                    page->resize(base);
                }
                page->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            }
            sa->setWidget(page);
            // Ensure the page itself paints white behind its child controls
            page->setAutoFillBackground(true);
            page->setStyleSheet(QStringLiteral("background-color: #ffffff;"));
            // Increase bottom margin and add a small spacer so last rows are not clipped
            if (QLayout* pl = page->layout()) {
                const QMargins m = pl->contentsMargins();
                pl->setContentsMargins(m.left(), m.top(), m.right(), m.bottom() + 24);
                pl->addItem(new QSpacerItem(0, 16, QSizePolicy::Minimum, QSizePolicy::Fixed));
            }
            sw->insertWidget(i, sa);
        }
        // Keep the current page visible (index remains consistent, but be explicit)
        sw->setCurrentIndex(sw->currentIndex());
    };
    wrapStackPagesInScroll(ui->metierspersonnel);
    wrapStackPagesInScroll(ui->metiersstocks);
    wrapStackPagesInScroll(ui->metiersCiternes);
    wrapStackPagesInScroll(ui->metiersqualite);
    wrapStackPagesInScroll(ui->metierspersonnel_2);
    wrapStackPagesInScroll(ui->metiersagriculteurs);

    // Targeted fix: Module 6 (Agriculteurs) "Ajouter" page sometimes clips the bottom button.
    // If the page lacks a layout (uses a child form widget with absolute geometry), add a
    // lightweight VBox layout with preserved margins and a bottom spacer to guarantee visibility.
    if (ui->metiersagriculteurs) {
        for (int i = 0; i < ui->metiersagriculteurs->count(); ++i) {
            if (auto* sa = qobject_cast<QScrollArea*>(ui->metiersagriculteurs->widget(i))) {
                QWidget* page = sa->widget();
                if (!page) continue;
                if (page->objectName() == QLatin1String("ajoutagriculteur")) {
                    // Increase viewport bottom padding further for this page
                    sa->viewport()->setContentsMargins(0, 0, 0, 64);
                    // If the page has no layout, create one and add the existing form container
                    if (!page->layout()) {
                        auto* v = new QVBoxLayout(page);
                        // Preserve original visual offsets similar to Designer geometry (x≈30, y≈20)
                        v->setContentsMargins(30, 20, 16, 48);
                        v->setSpacing(12);
                        // Prefer the Designer-owned agriculteur form container.
                        QWidget* form = ui->formLayoutWidget_7;
                        if (!form) {
                            // Fallback: pick the first direct child
                            const auto children = page->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
                            form = children.isEmpty() ? nullptr : children.first();
                        }
                        if (form) {
                            form->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                            v->addWidget(form);
                        }
                        // Bottom spacer to make sure the last row (Ajouter) isn't clipped
                        v->addItem(new QSpacerItem(0, 28, QSizePolicy::Minimum, QSizePolicy::Fixed));
                    }
                }
            }
        }
    }

    // Citernes order is now persisted in the .ui; no runtime swapping needed
    
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
    setupAffectationStatusFilter();
    setupAffectationOpenEndedOption();
    setupEmployeeFormValidation();
    setupSettingsAutoAssignOption();
    refreshStockSerieChoices();
    loadAffectationSettings();

    // Ensure toolbars are above content and have enough height for text-under-icon
    setupToolbarsTweaks();

    // With the user info bar now layout-managed, explicit repositioning is not required
    repositionUserInfo();
}

// ── Lifecycle: destruction / resource cleanup ───────────────────────────────
MainWindow::~MainWindow()
{
    // Always release serial port before UI teardown.
    m_fingerprintTerminal.close_arduino();
    delete ui;
}

// ── Face-recognition service adapters (thin wrappers over m_faceService) ───

QByteArray MainWindow::encodeFaceFromFile(const QString& imagePath)
{
    return m_faceService ? m_faceService->encodeFaceFromFile(imagePath) : QByteArray{};
}

void MainWindow::setFingerprintStatus(const QString& text, const QString& style)
{
    if (!ui || !ui->faceStatusLabel) return;
    ui->faceStatusLabel->setText(text);
    ui->faceStatusLabel->setStyleSheet(style);
}

void MainWindow::initFingerprintService()
{
    if (m_fingerprintService) {
        delete m_fingerprintService;
    }

    m_fingerprintService = new FingerprintService(this);

    // Connect service signals to UI slots
    connect(m_fingerprintService, &FingerprintService::ready,
            this, &MainWindow::onFingerprintServiceReady);
    connect(m_fingerprintService, &FingerprintService::matched,
            this, &MainWindow::onFingerprintMatched);
    connect(m_fingerprintService, &FingerprintService::enrollmentResult,
            this, &MainWindow::onEnrollmentResult);
    connect(m_fingerprintService, &FingerprintService::deletionResult,
            this, &MainWindow::onFingerprintDeletionResult);
    connect(m_fingerprintService, &FingerprintService::error,
            this, &MainWindow::onFingerprintError);
    connect(m_fingerprintService, &FingerprintService::scanningStateChanged,
            this, &MainWindow::onFingerprintScanningStateChanged);

    // Auto re-enable scanning when returning to login screen
    if (ui && ui->MainStacked) {
        connect(ui->MainStacked, QOverload<int>::of(&QStackedWidget::currentChanged),
                this, [this](int index) {
            if (index == 0 && m_fingerprintService) {  // 0 = login screen
                m_fingerprintService->startScanning();
            }
        }, Qt::UniqueConnection);
    }

    // Initialize the service (connects to Arduino)
    m_fingerprintService->initialize();
}

void MainWindow::onFingerprintServiceReady()
{
    setFingerprintStatus(tr("✔ Fingerprint ready"), 
                        QStringLiteral("color: #2e7d32; font-weight: bold;"));
    qDebug() << "[MainWindow] Fingerprint service ready";
}

void MainWindow::onFingerprintMatched(int fingerprintId)
{
    qDebug() << "[MainWindow] Fingerprint matched, ID:" << fingerprintId;

    const bool onLogin = ui->MainStacked && ui->MainStacked->currentIndex() == 0;

    // Look up employee by fingerprint ID using model layer
    Employe employe;
    int empId = -1;
    QString empName;

    if (!employe.findByFingerprintId(QString::number(fingerprintId), empId, empName) || empId <= 0) {
        qWarning() << "[MainWindow] Unknown fingerprint:" << fingerprintId;
        m_fingerprintService->sendDenied();
        if (onLogin) {
            setFingerprintStatus(tr("Unknown fingerprint"),
                               QStringLiteral("color: #c62828;"));
        }
        return;
    }

    // Send employee name to Arduino
    m_fingerprintService->sendName(empName);

    // Log in
    m_loggedInId = empId;
    if (ui->userNameLabel) ui->userNameLabel->setText(empName);
    if (ui->userinput) ui->userinput->clear();
    if (ui->pwdinput) ui->pwdinput->clear();
    if (onLogin) ui->MainStacked->setCurrentIndex(1);

    // Re-enable scanning after 2 seconds for next login attempt
    QTimer::singleShot(2000, this, [this]() {
        if (m_fingerprintService) {
            m_fingerprintService->startScanning();
        }
    });

    setFingerprintStatus(tr("✔ Login successful"),
                       QStringLiteral("color: #2e7d32; font-weight: bold;"));
}

void MainWindow::onEnrollmentResult(bool success, int fingerprintId, const QString &reason)
{
    if (success && fingerprintId > 0) {
        m_pendingFingerprintId = fingerprintId;
        setFingerprintStatus(QString("✔ Enrolled ID %1").arg(fingerprintId),
                           QStringLiteral("color: #2e7d32; font-weight: bold;"));
        qDebug() << "[MainWindow] Enrollment successful, ID:" << fingerprintId;
    } else {
        setFingerprintStatus(tr("❌ Enrollment failed: %1").arg(reason),
                           QStringLiteral("color: #c62828; font-weight: bold;"));
        qWarning() << "[MainWindow] Enrollment failed:" << reason;
    }
}

void MainWindow::onFingerprintDeletionResult(int fingerprintId, bool success)
{
    if (success) {
        setFingerprintStatus(QString("✔ Deleted ID %1").arg(fingerprintId),
                           QStringLiteral("color: #2e7d32; font-weight: bold;"));
        qDebug() << "[MainWindow] Deletion successful, ID:" << fingerprintId;
    } else {
        setFingerprintStatus(QString("❌ Deletion failed for ID %1").arg(fingerprintId),
                           QStringLiteral("color: #c62828; font-weight: bold;"));
        qWarning() << "[MainWindow] Deletion failed, ID:" << fingerprintId;
    }
}

void MainWindow::onFingerprintError(const QString &message)
{
    qWarning() << "[MainWindow] Fingerprint error:" << message;
    setFingerprintStatus(QString("⚠ Error: %1").arg(message),
                       QStringLiteral("color: #ef6c00; font-weight: bold;"));
}

void MainWindow::onFingerprintScanningStateChanged(bool scanning)
{
    if (scanning) {
        qDebug() << "[MainWindow] Fingerprint scanning started";
        setFingerprintStatus(tr("Scanning..."), QStringLiteral("color: #1976d2;"));
    } else {
        qDebug() << "[MainWindow] Fingerprint scanning stopped";
        setFingerprintStatus(tr("Scanning stopped"), QStringLiteral("color: #666;"));
    }
}

void MainWindow::startFingerprintEnrollmentFromForm()
{
    if (!m_fingerprintService || !m_fingerprintService->isConnected()) {
        QMessageBox::warning(this, tr("Erreur"), tr("Terminal d'empreintes non connecté."));
        return;
    }

    setFingerprintStatus(tr("⏳ Enrôlement en cours..."), 
                        QStringLiteral("color: #ef6c00; font-weight: bold;"));
    m_fingerprintService->requestEnrollment();
}

// ══════════════════════════════════════════════════════════════════════════════
// Legacy fingerprint functions removed - now handled by FingerprintService
// ══════════════════════════════════════════════════════════════════════════════

// ── Authentication / personnel main entry points ────────────────────────────

void MainWindow::on_loginbtn_clicked()
{
    const QString email = ui->userinput->text().trimmed();
    const QString mdp   = ui->pwdinput->text();

    // ── Basic field validation ────────────────────────────────────────────
    if (email.isEmpty() || mdp.isEmpty()) {
        QMessageBox::warning(this, tr("Champs requis"),
                             tr("Veuillez entrer votre email et votre mot de passe."));
        return;
    }

    // ── Authenticate against DB ───────────────────────────────────────────
    Employe emp;
    const int userId = emp.authenticate(email, mdp);

    if (userId < 0) {
        // Wrong credentials — shake the login area and show an error
        QMessageBox::critical(this, tr("Échec de connexion"),
                              tr("Email ou mot de passe incorrect.\n"
                                 "Veuillez réessayer."));
        ui->pwdinput->clear();
        ui->pwdinput->setFocus();
        return;
    }

    // ── Success — remember who is logged in ───────────────────────────────
    m_loggedInId = userId;

    // Fetch the employee's name to display in the top-right info bar
    Employe emp;
    QString fullName = emp.getFullNameById(userId);
    if (!fullName.isEmpty() && ui->userNameLabel) {
        ui->userNameLabel->setText(fullName);
    }

    // Clear credentials so they are not visible if the user later logs out
    ui->userinput->clear();
    ui->pwdinput->clear();

    // Navigate to the main application page (index 1 of MainStacked)
    ui->MainStacked->setCurrentIndex(1);
}

void MainWindow::on_btnAjouterEmp_clicked()
{
    // Ensure we're on the personnel module first
    if (ui->modules->currentIndex() != 0)
        crossFadeToIndex(ui->modules, 0);

    // Always open in ADD mode from the toolbar action
    ui->ajouterEmpBtn->setProperty("editingId", QVariant());
    ui->ajouterEmpBtn->setText(tr("Ajouter"));

    // Switch to "Add Personnel" page (index 0)
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

    const auto buildFaceBlob = [this]() {
        QByteArray faceBlob = m_capturedFaceBlob;
        if (faceBlob.isEmpty() && !ui->photoPathLineEdit->text().isEmpty()) {
            faceBlob = encodeFaceFromFile(ui->photoPathLineEdit->text());
        }
        return faceBlob;
    };

    if (isEditing) {
        // ── UPDATE mode ─────────────────────────────────────────────────────
        // Priority: live webcam capture > photo-file encoding > keep existing in DB
        QByteArray faceBlob = buildFaceBlob();

        Employe emp(editingId, nom, prenom, email, role, mdp, QDate(),
                    m_selectedPhoto, QByteArray(), faceBlob);
        if (!emp.modifier()) {
            QMessageBox::critical(this, tr("Erreur de modification"),
                tr("Impossible de modifier l'employé :\n%1").arg(emp.lastError().text()));
            return;
        }

        // Link pending fingerprint if one was enrolled
        if (m_pendingFingerprintId > 0) {
            Employe empHelper;
            if (empHelper.updateFingerprintId(editingId, QString::number(m_pendingFingerprintId))) {
                qDebug() << "[MainWindow] Linked fingerprint" << m_pendingFingerprintId << "to employee" << editingId;
                m_pendingFingerprintId = -1;
            }
        }

        QMessageBox::information(this, tr("Succès"),
            tr("L'employé (ID : %1) a été modifié avec succès.").arg(editingId));

        // Reset button to Add mode
        ui->ajouterEmpBtn->setProperty("editingId", QVariant());
        ui->ajouterEmpBtn->setText(tr("Ajouter"));

    } else {
        // ── INSERT mode ─────────────────────────────────────────────────────
        // Priority: live webcam capture > photo-file encoding
        QByteArray faceBlob = buildFaceBlob();

        Employe emp(0, nom, prenom, email, role, mdp, QDate(),
                    m_selectedPhoto, QByteArray(), faceBlob);
        if (!emp.ajouter()) {
            QMessageBox::critical(this, tr("Erreur d'ajout"),
                tr("Impossible d'ajouter l'employé :\n%1").arg(emp.lastError().text()));
            return;
        }

        // Link pending fingerprint if one was enrolled
        if (m_pendingFingerprintId > 0) {
            if (emp.updateFingerprintId(emp.getIdEmp(), QString::number(m_pendingFingerprintId))) {
                qDebug() << "[MainWindow] Linked fingerprint" << m_pendingFingerprintId << "to new employee" << emp.getIdEmp();
                m_pendingFingerprintId = -1;
            }
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
    m_pendingFingerprintId = -1;
    m_fingerprintEnrollInProgress = false;
    setFingerprintStatus(tr("Aucun visage capturé"), QString());
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
            setFingerprintStatus(tr("✔ Visage détecté depuis la photo"),
                                 QStringLiteral("color: #2e7d32;"));
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
    setFingerprintStatus(tr("✔ Visage capturé avec succès"),
                         QStringLiteral("color: #2e7d32; font-weight: bold;"));
}

void MainWindow::on_btnConsulterEmp_clicked()
{
    ensureModuleIndex(0);
    // Load fresh data then switch to the consult page (index 1)
    loadEmployeeTable();
    crossFadeToIndex(ui->metierspersonnel, 1);
}

void MainWindow::on_btnStatEmp_clicked()
{
    ensureModuleIndex(0);
    // Switch to "Statistics" page (index 2)
    crossFadeToIndex(ui->metierspersonnel, 2);
    loadEmployeeStats();
}

void MainWindow::on_btnAdvEmp_clicked()
{
    ensureModuleIndex(0);
    // Switch to "Advanced" page (index 3) and refresh data
    crossFadeToIndex(ui->metierspersonnel, 3);
    populateAffCombos();
    loadAffectationTable();
    // Start on the table view
    ui->affStack->setCurrentIndex(1);
}

// ── Affectation: setup + data helpers ───────────────────────────────────────
// Schema:
//   EMP_MACH  (ID_EMP, ID_SERIE)          — composite PK, only 2 columns
//   SERIE     (ID_SERIE, NOM_SERIE, ...)   — ID_SERIE is PK
//   MACHINE   (ID_MACHINE, NOM_MACHINE, ..., ID_SERIE)  — FK to SERIE

void MainWindow::populateAffCombos()
{
    // Purpose:
    //   Fill the form combo boxes used to create/edit an assignment:
    //   - Employee selector (EMPLOYE)
    //   - Series selector (SERIE_MACHINE + MACHINE display label)
    // Side effect:
    //   Recomputes remaining assignment capacity after repopulating selections.

    // ── Employees ────────────────────────────────────────────────────────────
    ui->affEmpCombo->clear();
    Employe emp;
    QList<QPair<int, QString>> employees = emp.getAllEmployeesWithNames();
    for (const auto& pair : employees) {
        ui->affEmpCombo->addItem(pair.second, pair.first);
    }

    // ── Séries (joined with Machine name for display) ─────────────────────
    // MACHINE has ID_SERIE FK → we join to show "NomMachine – NomSerie"
    ui->affSerieCombo->clear();
    QSqlQuery qSerie(
        "SELECT s.id_serie, "
        "       m.nom_machine || ' – ' || s.nom_serie "
        "FROM   SERIE_MACHINE s "
        "LEFT JOIN MACHINE m ON m.id_serie = s.id_serie "
        "ORDER BY m.nom_machine, s.nom_serie");
    while (qSerie.next())
        ui->affSerieCombo->addItem(qSerie.value(1).toString(), qSerie.value(0).toInt());

    updateAffectationRemainingInfo();
}

void MainWindow::setupAffectationStatusFilter()
{
    // The status filter combo is owned by .ui.
    if (!ui->affStatusFilterCombo) return;

    if (!ui->affStatusFilterCombo->property("wiredCurrentTextChanged").toBool()) {
        QObject::connect(ui->affStatusFilterCombo, &QComboBox::currentTextChanged,
                         this, [this](const QString&) { filterAffTable(); });
        ui->affStatusFilterCombo->setProperty("wiredCurrentTextChanged", true);
    }
}

void MainWindow::setupAffectationOpenEndedOption()
{
    // The open-ended checkbox is owned by .ui.
    if (!ui->affDateFinEdit) return;
    if (!ui->affOpenEndedCheck) return;

    ui->affDateFinEdit->setEnabled(!ui->affOpenEndedCheck->isChecked());
    if (!ui->affOpenEndedCheck->property("wiredToggled").toBool()) {
        QObject::connect(ui->affOpenEndedCheck, &QCheckBox::toggled, this, [this](bool checked) {
            if (!ui->affDateFinEdit) return;
            ui->affDateFinEdit->setEnabled(!checked);
            if (!checked && !ui->affDateFinEdit->date().isValid()) {
                ui->affDateFinEdit->setDate(QDate::currentDate().addMonths(1));
            }
        });
        ui->affOpenEndedCheck->setProperty("wiredToggled", true);
    }
}

// Computes and displays remaining assignment slots for the selected employee,
// and enables/disables the save action accordingly.
void MainWindow::updateAffectationRemainingInfo()
{
    // Business rule:
    //   A user-defined max number of active assignments per employee is enforced.
    // This function computes current usage and updates:
    //   1) informational label (remaining slots)
    //   2) save button enabled state (block when limit reached)

    const bool hasRemainingLabel = (ui->affRemainingInfoLabel != nullptr);
    const auto setSaveEnabled = [this](bool enabled) {
        if (ui->affSaveBtn) ui->affSaveBtn->setEnabled(enabled);
    };
    bool canSave = true;

    const int empId = ui->affEmpCombo ? ui->affEmpCombo->currentData().toInt() : -1;
    if (empId <= 0) {
        canSave = false;
        if (hasRemainingLabel) {
            ui->affRemainingInfoLabel->setText(
                tr("Sélectionnez un employé pour voir les places restantes."));
            ui->affRemainingInfoLabel->setStyleSheet("color:#546e7a; font-weight:600;");
        }
        setSaveEnabled(canSave);
        return;
    }

    Employe emp;
    int used = emp.countAssignments(empId);
    if (used < 0) {
        canSave = false;
        if (hasRemainingLabel) {
            ui->affRemainingInfoLabel->setText(
                tr("Impossible de calculer les affectations restantes."));
            ui->affRemainingInfoLabel->setStyleSheet("color:#c62828; font-weight:600;");
        }
        setSaveEnabled(canSave);
        return;
    }

    const int remaining = qMax(0, m_maxAffectationsPerEmployee - used);
    const bool isEditMode = (m_editingAffIdEmp > 0 && m_editingAffIdSerie > 0);
    const bool increasesCount = !isEditMode || (empId != m_editingAffIdEmp);
    const bool limitBlocksSave = increasesCount && (remaining <= 0);

    canSave = !limitBlocksSave;

    if (hasRemainingLabel) {
        if (limitBlocksSave) {
            ui->affRemainingInfoLabel->setText(
                tr("Affectations utilisées : %1 / %2 — restantes : %3 (limite atteinte)")
                    .arg(used)
                    .arg(m_maxAffectationsPerEmployee)
                    .arg(remaining));
            ui->affRemainingInfoLabel->setStyleSheet("color:#c62828; font-weight:700;");
        } else if (remaining <= 0 && isEditMode) {
            ui->affRemainingInfoLabel->setText(
                tr("Affectations utilisées : %1 / %2 — restantes : %3 (modification autorisée)")
                    .arg(used)
                    .arg(m_maxAffectationsPerEmployee)
                    .arg(remaining));
            ui->affRemainingInfoLabel->setStyleSheet("color:#ef6c00; font-weight:700;");
        } else {
            ui->affRemainingInfoLabel->setText(
                tr("Affectations utilisées : %1 / %2 — restantes : %3")
                    .arg(used)
                    .arg(m_maxAffectationsPerEmployee)
                    .arg(remaining));

            if (remaining == 1) {
                ui->affRemainingInfoLabel->setStyleSheet("color:#ef6c00; font-weight:700;");
            } else {
                ui->affRemainingInfoLabel->setStyleSheet("color:#2e7d32; font-weight:700;");
            }
        }
    }

    setSaveEnabled(canSave);
}

bool MainWindow::hasDuplicateAffectation(int empId, int serieId) const
{
    Employe emp;
    return emp.hasAffectationFor(empId, serieId);
}

void MainWindow::prepareInsertAffectationQuery(QSqlQuery& query,
                                               int empId,
                                               int serieId,
                                               const QString& poste,
                                               const QDate& dateDeb,
                                               const QVariant& dateFinValue) const
{
    query.prepare(
        "INSERT INTO EMP_MACH (id_emp, id_serie, poste, date_debut, date_fin) "
        "VALUES (:id_emp, :id_serie, :poste, :date_debut, :date_fin)");
    query.bindValue(":id_emp", empId);
    query.bindValue(":id_serie", serieId);
    query.bindValue(":poste", poste);
    query.bindValue(":date_debut", dateDeb);
    query.bindValue(":date_fin", dateFinValue);
}

// Loads affectation rows into the table and wires row-level edit/delete actions.
void MainWindow::loadAffectationTable()
{
    // Purpose:
    //   Hydrate the affectation table from DB with denormalized display columns
    //   (employee full name, series name, machine name, dates, status).
    // Notes:
    //   - date_fin NULL => ACTIVE
    //   - date_fin set  => TERMINEE

    QTableWidget* t = ui->affTable;
    t->setRowCount(0);
    t->setSortingEnabled(false);
    t->setColumnCount(9);
    t->setHorizontalHeaderLabels({
        tr("ID Emp"), tr("Employé"), tr("Série"), tr("Machine"),
        tr("Poste"), tr("Date début"), tr("Date fin"), tr("État"), tr("Actions")
    });

    // Columns: 0=ID_EMP  1=Employé  2=Série  3=Machine  4=Poste
    //          5=Date début 6=Date fin 7=État 8=Actions
    QSqlQuery q(
        "SELECT em.id_emp, "
        "       e.nom_emp || ' ' || e.prenom_emp, "
        "       s.nom_serie, "
        "       em.id_serie, "
        "       NVL(m.nom_machine, '-'), "
        "       NVL(em.poste, '-'), "
        "       TO_CHAR(em.date_debut, 'DD/MM/YYYY'), "
        "       TO_CHAR(em.date_fin,   'DD/MM/YYYY'), "
        "       CASE WHEN em.date_fin IS NULL THEN 'ACTIVE' ELSE 'TERMINEE' END "
        "FROM   EMP_MACH em "
        "JOIN   EMPLOYE       e ON e.id_emp   = em.id_emp "
        "JOIN   SERIE_MACHINE s ON s.id_serie = em.id_serie "
        "LEFT JOIN MACHINE    m ON m.id_serie = em.id_serie "
        "ORDER BY e.nom_emp, s.nom_serie"
    );

    auto setCell = [&](int r, int c, const QString& txt) {
        auto* item = new QTableWidgetItem(txt);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        t->setItem(r, c, item);
    };

    while (q.next()) {
        int row = t->rowCount();
        t->insertRow(row);

        int idEmp   = q.value(0).toInt();
        int idSerie = q.value(3).toInt();
        setCell(row, 0, QString::number(idEmp));
        setCell(row, 1, q.value(1).toString());
        setCell(row, 2, q.value(2).toString());
        setCell(row, 3, q.value(4).toString());  // machine name
        setCell(row, 4, q.value(5).toString());  // poste
        setCell(row, 5, q.value(6).toString());  // date_debut
        setCell(row, 6, q.value(7).toString());  // date_fin
        setCell(row, 7, q.value(8).toString());  // état
        if (auto* st = t->item(row, 7)) {
            const bool active = (st->text().compare(QStringLiteral("ACTIVE"), Qt::CaseInsensitive) == 0);
            st->setText(active ? tr("ACTIVE") : tr("TERMINEE"));
            st->setTextAlignment(Qt::AlignCenter);
            st->setForeground(active ? QColor("#2e7d32") : QColor("#c62828"));
        }

    // ── Action buttons ────────────────────────────────────────────────
    // Each row gets Edit/Delete controls bound to composite key (id_emp,id_serie).
        auto* cell  = new QWidget();
        auto* hlay  = new QHBoxLayout(cell);
        hlay->setContentsMargins(2, 2, 2, 2);
        hlay->setSpacing(4);

        auto* btnEdit = new QToolButton();
        btnEdit->setIcon(QIcon(":/img/edit.svg"));
        btnEdit->setToolTip(tr("Modifier"));
        btnEdit->setAutoRaise(true);

        auto* btnDel = new QToolButton();
        btnDel->setIcon(QIcon(":/img/delete.svg"));
        btnDel->setToolTip(tr("Supprimer l'affectation"));
        btnDel->setAutoRaise(true);

        hlay->addStretch();
        hlay->addWidget(btnEdit);
        hlay->addWidget(btnDel);
    t->setCellWidget(row, 8, cell);

    // Edit flow:
    //   1) load selected row values into the form
    //   2) track original composite PK in m_editingAffId*
    //   3) switch form to "Modifier" mode
        connect(btnEdit, &QToolButton::clicked, this, [this, idEmp, idSerie, row]() {
            // Pre-select employee
            int empIdx = ui->affEmpCombo->findData(idEmp);
            if (empIdx >= 0) ui->affEmpCombo->setCurrentIndex(empIdx);
            // Pre-select serie
            int serieIdx = ui->affSerieCombo->findData(idSerie);
            if (serieIdx >= 0) ui->affSerieCombo->setCurrentIndex(serieIdx);
            // Poste
            QString poste = ui->affTable->item(row, 4)
                            ? ui->affTable->item(row, 4)->text() : QString();
            int posteIdx = ui->affPosteCombo->findText(poste);
            if (posteIdx >= 0) ui->affPosteCombo->setCurrentIndex(posteIdx);
            // Dates
            QString sdeb = ui->affTable->item(row, 5)
                           ? ui->affTable->item(row, 5)->text() : QString();
            QString sfin = ui->affTable->item(row, 6)
                           ? ui->affTable->item(row, 6)->text() : QString();
            ui->affDateDebEdit->setDate(
                sdeb.isEmpty() ? QDate::currentDate()
                               : QDate::fromString(sdeb, "dd/MM/yyyy"));
            const bool openEnded = sfin.isEmpty();
            if (ui->affOpenEndedCheck) {
                ui->affOpenEndedCheck->setChecked(openEnded);
            }
            if (!openEnded) {
                ui->affDateFinEdit->setDate(QDate::fromString(sfin, "dd/MM/yyyy"));
            } else {
                ui->affDateFinEdit->setDate(QDate::currentDate().addMonths(1));
            }

            m_editingAffIdEmp   = idEmp;
            m_editingAffIdSerie = idSerie;
            ui->affSaveBtn->setText(tr("Modifier"));
            updateAffectationRemainingInfo();
            ui->affStack->setCurrentIndex(0);
        });

    // Delete flow:
    //   confirm => DELETE by composite PK => remove row from UI on success
        connect(btnDel, &QToolButton::clicked, this, [this, idEmp, idSerie, row]() {
            if (QMessageBox::question(this, tr("Confirmer la suppression"),
                    tr("Supprimer l'affectation de cet employé à cette série ?"))
                    != QMessageBox::Yes)
                return;
            QSqlQuery qd;
            qd.prepare(
                "DELETE FROM EMP_MACH "
                "WHERE  id_emp   = :id_emp "
                "AND    id_serie = :id_serie");
            qd.bindValue(":id_emp",   idEmp);
            qd.bindValue(":id_serie", idSerie);
            if (qd.exec()) {
                ui->affTable->removeRow(row);
            } else {
                QMessageBox::critical(this, tr("Erreur"),
                    tr("Impossible de supprimer :\n%1").arg(qd.lastError().text()));
            }
        });
    }

    // Column sizing
    t->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    t->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    t->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    t->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    t->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    t->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    t->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    t->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    t->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    t->setSortingEnabled(true);

    // Keep table aligned with current text/state filters after reloading.
    filterAffTable();
}

// Applies text/state filters to the affectation table without re-querying the DB.
void MainWindow::filterAffTable()
{
    // Filtering strategy:
    //   - Text search across employee/series/machine/poste
    //   - Optional state filter based on computed active state from date_fin
    //     (empty date_fin in table => ACTIVE)

    QString search = ui->affSearchEdit->text().trimmed().toLower();
    const QString stateFilter = ui->affStatusFilterCombo
        ? ui->affStatusFilterCombo->currentText()
        : tr("Toutes");
    const bool wantActive = (stateFilter.compare(tr("Actives"), Qt::CaseInsensitive) == 0);
    const bool wantDone   = (stateFilter.compare(tr("Terminées"), Qt::CaseInsensitive) == 0);

    for (int r = 0; r < ui->affTable->rowCount(); ++r) {
        auto* empItem   = ui->affTable->item(r, 1);  // Employé
        auto* serieItem = ui->affTable->item(r, 2);  // Série
        auto* machItem  = ui->affTable->item(r, 3);  // Machine
        auto* posteItem = ui->affTable->item(r, 4);  // Poste
        auto* finItem   = ui->affTable->item(r, 6);  // Date fin

        const bool isActive = !finItem || finItem->text().trimmed().isEmpty();
        const bool matchState = (!wantActive && !wantDone)
            || (wantActive && isActive)
            || (wantDone && !isActive);

        bool matchSearch = search.isEmpty()
            || (empItem   && empItem->text().toLower().contains(search))
            || (serieItem && serieItem->text().toLower().contains(search))
            || (machItem  && machItem->text().toLower().contains(search))
            || (posteItem && posteItem->text().toLower().contains(search));

        ui->affTable->setRowHidden(r, !(matchSearch && matchState));
    }
}

// ── Affectation slots ────────────────────────────────────────────────────────

void MainWindow::on_affNewBtn_clicked()
{
    // Prepare a clean form for INSERT mode.
    resetAffectationEditState();
    ui->affEmpCombo->setCurrentIndex(0);
    ui->affSerieCombo->setCurrentIndex(0);
    ui->affPosteCombo->setCurrentIndex(0);
    ui->affDateDebEdit->setDate(QDate::currentDate());
    ui->affDateFinEdit->setDate(QDate::currentDate().addMonths(1));
    if (ui->affOpenEndedCheck) ui->affOpenEndedCheck->setChecked(true);
    if (ui->affLimitInfoLabel) {
        ui->affLimitInfoLabel->setText(
            tr("Limite actuelle : %1 affectation(s) max par employé.")
                .arg(m_maxAffectationsPerEmployee));
    }
    updateAffectationRemainingInfo();
    ui->affStack->setCurrentIndex(0);
}

void MainWindow::on_affCancelBtn_clicked()
{
    // Cancel editing/inserting and return to table page.
    resetAffectationEditState();
    if (ui->affOpenEndedCheck) ui->affOpenEndedCheck->setChecked(true);
    ui->affStack->setCurrentIndex(1);
}

void MainWindow::on_affRefreshBtn_clicked()
{
    // Full UI refresh from DB (table + limit label/button state).
    loadAffectationTable();
    updateAffectationRemainingInfo();
}

void MainWindow::on_affSearchEdit_textChanged(const QString&)
{
    // Lightweight client-side filtering (no SQL round-trip).
    filterAffTable();
}

void MainWindow::on_affSaveBtn_clicked()
{
    // Save contract:
    //   - INSERT mode when m_editingAffId* are not set
    //   - EDIT mode otherwise
    // Composite PK handling:
    //   EMP_MACH uses (id_emp, id_serie), so PK changes require re-insert logic.

    int     newEmpId   = ui->affEmpCombo->currentData().toInt();
    int     newSerieId = ui->affSerieCombo->currentData().toInt();
    QString poste      = ui->affPosteCombo->currentText();
    QDate   dateDeb    = ui->affDateDebEdit->date();
    QDate   dateFin    = ui->affDateFinEdit->date();
    const bool openEnded = (ui->affOpenEndedCheck && ui->affOpenEndedCheck->isChecked());

    if (newEmpId <= 0 || newSerieId <= 0) {
        QMessageBox::warning(this, tr("Champs requis"),
            tr("Veuillez sélectionner un employé et une série."));
        return;
    }
    if (!openEnded && dateFin < dateDeb) {
        QMessageBox::warning(this, tr("Dates invalides"),
            tr("La date de fin doit être postérieure à la date de début."));
        return;
    }

    const QVariant dateFinValue = openEnded
        ? QVariant(QMetaType(QMetaType::QDate))
        : QVariant(dateFin);

    // Enforce configurable max number of active assignments per employee.
    // We only block if this operation increases active assignment count.
    const bool isEditMode = (m_editingAffIdEmp > 0 && m_editingAffIdSerie > 0);
    const bool increasesTargetEmployeeCount = !isEditMode || (newEmpId != m_editingAffIdEmp);

    if (increasesTargetEmployeeCount && m_maxAffectationsPerEmployee > 0) {
        QSqlQuery qCount;
        qCount.prepare(
            "SELECT COUNT(*) "
            "FROM EMP_MACH "
            "WHERE id_emp = :id_emp "
            "  AND date_fin IS NULL");
        qCount.bindValue(":id_emp", newEmpId);
        if (!qCount.exec() || !qCount.next()) {
            QMessageBox::critical(this, tr("Erreur"),
                tr("Impossible de vérifier la limite d'affectations :\n%1")
                    .arg(qCount.lastError().text()));
            return;
        }

        const int currentCount = qCount.value(0).toInt();
        if (currentCount >= m_maxAffectationsPerEmployee) {
            QMessageBox::warning(this, tr("Limite atteinte"),
                tr("Cet employé a déjà %1 affectation(s).\n"
                   "La limite configurée est %2.")
                    .arg(currentCount)
                    .arg(m_maxAffectationsPerEmployee));
            return;
        }
    }

    QSqlQuery q;

    if (isEditMode) {
    // ── EDIT mode ────────────────────────────────────────────────────────
    // If PK unchanged => UPDATE non-key columns.
    // If PK changed   => delete old row + insert new row.
        if (newEmpId == m_editingAffIdEmp && newSerieId == m_editingAffIdSerie) {
            // Same PK — just UPDATE the extra columns in place
            q.prepare(
                "UPDATE EMP_MACH SET "
                "  poste      = :poste, "
                "  date_debut = :date_debut, "
                "  date_fin   = :date_fin "
                "WHERE id_emp   = :id_emp "
                "AND   id_serie = :id_serie");
            q.bindValue(":poste",      poste);
            q.bindValue(":date_debut", dateDeb);
            q.bindValue(":date_fin",   dateFinValue);
            q.bindValue(":id_emp",     m_editingAffIdEmp);
            q.bindValue(":id_serie",   m_editingAffIdSerie);
        } else {
            // PK changed — duplicate guard first (same employee + same series).
            if (hasDuplicateAffectation(newEmpId, newSerieId)) {
                QMessageBox::warning(this, tr("Doublon"),
                    tr("Cet employé est déjà affecté à cette série."));
                return;
            }
            // Delete old PK row, then insert the new PK row.
            QSqlQuery qDel;
            qDel.prepare(
                "DELETE FROM EMP_MACH "
                "WHERE id_emp = :old_emp AND id_serie = :old_serie");
            qDel.bindValue(":old_emp",   m_editingAffIdEmp);
            qDel.bindValue(":old_serie", m_editingAffIdSerie);
            if (!qDel.exec()) {
                QMessageBox::critical(this, tr("Erreur"),
                    tr("Impossible de modifier l'affectation :\n%1")
                        .arg(qDel.lastError().text()));
                return;
            }
            // Insert new row
            prepareInsertAffectationQuery(q, newEmpId, newSerieId, poste, dateDeb, dateFinValue);
        }
    } else {
    // ── INSERT mode ───────────────────────────────────────────────────────
    // Guard against duplicate composite key before insert.
        if (hasDuplicateAffectation(newEmpId, newSerieId)) {
            QMessageBox::warning(this, tr("Doublon"),
                tr("Cet employé est déjà affecté à cette série."));
            return;
        }
        prepareInsertAffectationQuery(q, newEmpId, newSerieId, poste, dateDeb, dateFinValue);
    }

    if (!q.exec()) {
        QMessageBox::critical(this, tr("Erreur"),
            tr("Impossible d'enregistrer l'affectation :\n%1").arg(q.lastError().text()));
        return;
    }

    bool wasEdit = (m_editingAffIdEmp > 0);
    resetAffectationEditState();
    loadAffectationTable();
    updateAffectationRemainingInfo();
    ui->affStack->setCurrentIndex(1);
    QMessageBox::information(this, tr("Succès"),
        wasEdit ? tr("Affectation modifiée avec succès.")
                : tr("Affectation enregistrée avec succès."));
}

void MainWindow::resetAffectationEditState()
{
    m_editingAffIdEmp = -1;
    m_editingAffIdSerie = -1;
    if (ui && ui->affSaveBtn) {
        ui->affSaveBtn->setText(tr("Affecter"));
    }
}

// ── Affectation settings + stock auto-assignment policy ────────────────────

// Adds runtime settings controls related to affectation behavior.
void MainWindow::setupSettingsAutoAssignOption()
{
    // The auto-assign toggle is now owned by .ui.
    if (m_settingsAutoAssignCheck) return;
    m_settingsAutoAssignCheck = ui->settingsAutoAssignCheck;
}

void MainWindow::loadAffectationSettings()
{
    // Loads persisted affectation policy from settings.dat:
    //   - max active assignments per employee
    //   - stock->auto-assign toggle
    // Falls back to defaults when file is absent/unreadable.

    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty())
        baseDir = QCoreApplication::applicationDirPath();
    QDir().mkpath(baseDir);

    const QString path = QDir(baseDir).filePath(QStringLiteral("settings.dat"));
    QFile file(path);
    if (!file.exists()) {
        if (ui->settingsMaxAffectationsSpin)
            ui->settingsMaxAffectationsSpin->setValue(m_maxAffectationsPerEmployee);
        if (m_settingsAutoAssignCheck)
            m_settingsAutoAssignCheck->setChecked(m_autoAssignFromStock);
        if (ui->affLimitInfoLabel) {
            ui->affLimitInfoLabel->setText(
                tr("Limite actuelle : %1 affectation(s) max par employé.")
                    .arg(m_maxAffectationsPerEmployee));
        }
        updateAffectationRemainingInfo();
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (ui->settingsMaxAffectationsSpin)
            ui->settingsMaxAffectationsSpin->setValue(m_maxAffectationsPerEmployee);
        if (m_settingsAutoAssignCheck)
            m_settingsAutoAssignCheck->setChecked(m_autoAssignFromStock);
        if (ui->affLimitInfoLabel) {
            ui->affLimitInfoLabel->setText(
                tr("Limite actuelle : %1 affectation(s) max par employé.")
                    .arg(m_maxAffectationsPerEmployee));
        }
        updateAffectationRemainingInfo();
        return;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_5);

    quint32 magic = 0;
    qint32 version = 0;
    qint32 maxAff = m_maxAffectationsPerEmployee;
    bool autoAssign = m_autoAssignFromStock;
    in >> magic >> version >> maxAff;

    if (in.status() == QDataStream::Ok && magic == 0x534f504d && version >= 2) {
        in >> autoAssign;
    }

    // Simple integrity/version checks
    if (in.status() == QDataStream::Ok && magic == 0x534f504d && version >= 1 && maxAff > 0) {
        m_maxAffectationsPerEmployee = maxAff;
        m_autoAssignFromStock = (version >= 2) ? autoAssign : false;
    }

    if (ui->settingsMaxAffectationsSpin)
        ui->settingsMaxAffectationsSpin->setValue(m_maxAffectationsPerEmployee);
    if (m_settingsAutoAssignCheck)
        m_settingsAutoAssignCheck->setChecked(m_autoAssignFromStock);
    if (ui->affLimitInfoLabel) {
        ui->affLimitInfoLabel->setText(
            tr("Limite actuelle : %1 affectation(s) max par employé.")
                .arg(m_maxAffectationsPerEmployee));
    }
    updateAffectationRemainingInfo();
}

// Persists affectation-related settings to settings.dat.
bool MainWindow::saveAffectationSettings()
{
    // Persists policy to settings.dat (magic + versioned payload).
    if (ui->settingsMaxAffectationsSpin)
        m_maxAffectationsPerEmployee = ui->settingsMaxAffectationsSpin->value();
    if (m_settingsAutoAssignCheck)
        m_autoAssignFromStock = m_settingsAutoAssignCheck->isChecked();

    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty())
        baseDir = QCoreApplication::applicationDirPath();
    QDir().mkpath(baseDir);

    const QString path = QDir(baseDir).filePath(QStringLiteral("settings.dat"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_5);
    out << quint32(0x534f504d) << qint32(2)
        << qint32(m_maxAffectationsPerEmployee)
        << bool(m_autoAssignFromStock);

    return out.status() == QDataStream::Ok;
}

// Applies settings form changes and refreshes affectation UI indicators.
void MainWindow::on_settingsSaveBtn_clicked()
{
    // Commits settings changes and refreshes labels/limits immediately.
    if (!saveAffectationSettings()) {
        QMessageBox::critical(this, tr("Erreur"),
            tr("Impossible d'enregistrer les paramètres dans le fichier settings.dat."));
        return;
    }

    if (ui->affLimitInfoLabel) {
        ui->affLimitInfoLabel->setText(
            tr("Limite actuelle : %1 affectation(s) max par employé.")
                .arg(m_maxAffectationsPerEmployee));
    }
    updateAffectationRemainingInfo();

    QMessageBox::information(this, tr("Paramètres"),
        tr("Paramètres enregistrés avec succès.\n"
           "• Limite : %1 affectation(s) max par employé\n"
           "• Auto affectation depuis stock : %2")
            .arg(m_maxAffectationsPerEmployee)
            .arg(m_autoAssignFromStock ? tr("Activée") : tr("Désactivée")));
}

void MainWindow::refreshStockSerieChoices()
{
    if (!ui->stockSerieCombo) return;

    const QVariant previous = ui->stockSerieCombo->currentData();
    ui->stockSerieCombo->clear();
    ui->stockSerieCombo->addItem(tr("Choisir une série..."), QVariant());

    QSqlQuery q(
        "SELECT id_serie, nom_serie "
        "FROM SERIE_MACHINE "
        "ORDER BY nom_serie, id_serie");

    if (q.lastError().isValid()) {
        ui->stockSerieCombo->addItem(tr("Erreur de chargement des séries"), QVariant());
        return;
    }

    while (q.next()) {
        const int id = q.value(0).toInt();
        const QString name = q.value(1).toString();
        ui->stockSerieCombo->addItem(QStringLiteral("%1 (ID %2)").arg(name).arg(id), id);
    }

    if (previous.isValid()) {
        const int idx = ui->stockSerieCombo->findData(previous);
        if (idx >= 0) ui->stockSerieCombo->setCurrentIndex(idx);
    }
}

// Tries to create an automatic assignment for a series when stock is inserted.
bool MainWindow::tryAutoAssignForSerie(int serieId, QString& detailMessage)
{
    // Auto-assignment algorithm:
    //   1) Choose one employee with the lowest current active assignment count
    //      who is not already assigned to this series and still below max limit.
    //   2) Insert EMP_MACH row (id_serie, id_emp).
    //   3) Return human-readable detail message for UI feedback.

    detailMessage.clear();
    if (serieId <= 0) {
        detailMessage = tr("Série invalide pour l'affectation automatique.");
        return false;
    }

    if (m_maxAffectationsPerEmployee <= 0) {
        detailMessage = tr("La limite d'affectations est invalide.");
        return false;
    }

    QSqlQuery pick;
    pick.prepare(
        "SELECT id_emp FROM ("
        "  SELECT e.id_emp, "
        "         SUM(CASE WHEN em.id_emp IS NOT NULL AND em.date_fin IS NULL THEN 1 ELSE 0 END) AS cnt "
        "  FROM EMPLOYE e "
        "  LEFT JOIN EMP_MACH em ON em.id_emp = e.id_emp "
        "  WHERE NOT EXISTS ("
        "    SELECT 1 FROM EMP_MACH ex "
        "    WHERE ex.id_emp = e.id_emp "
        "      AND ex.id_serie = :serie "
        "      AND ex.date_fin IS NULL"
        "  ) "
        "  GROUP BY e.id_emp "
        "  HAVING SUM(CASE WHEN em.id_emp IS NOT NULL AND em.date_fin IS NULL THEN 1 ELSE 0 END) < :max_aff "
        "  ORDER BY cnt ASC, e.id_emp ASC"
        ") "
    );
    pick.bindValue(":serie", serieId);
    pick.bindValue(":max_aff", m_maxAffectationsPerEmployee);

    if (!pick.exec()) {
        detailMessage = tr("Échec de recherche d'un employé disponible : %1").arg(pick.lastError().text());
        return false;
    }

    if (!pick.next()) {
        detailMessage = tr("Aucun employé disponible pour auto affectation.");
        return false;
    }

    const int empId = pick.value(0).toInt();
    if (empId <= 0) {
        detailMessage = tr("Employé sélectionné invalide.");
        return false;
    }

    QSqlQuery qIns;
    qIns.prepare(
        "INSERT INTO EMP_MACH (id_serie, id_emp) "
        "VALUES (:id_serie, :id_emp)");
    qIns.bindValue(":id_serie", serieId);
    qIns.bindValue(":id_emp", empId);

    if (!qIns.exec()) {
        detailMessage = tr("Impossible de créer l'affectation automatique : %1").arg(qIns.lastError().text());
        return false;
    }

    QString empName;
    QSqlQuery qEmp;
    qEmp.prepare("SELECT nom_emp || ' ' || prenom_emp FROM EMPLOYE WHERE id_emp = :id");
    qEmp.bindValue(":id", empId);
    if (qEmp.exec() && qEmp.next())
        empName = qEmp.value(0).toString().trimmed();

    if (empName.isEmpty())
        detailMessage = tr("Affectation auto réussie (employé ID %1)." ).arg(empId);
    else
        detailMessage = tr("Affectation auto réussie : %1.").arg(empName);

    return true;
}

// ── Stock module CRUD/navigation helpers ────────────────────────────────────
void MainWindow::loadStocksTable()
{
    if (!ui->tableWidget_2) return;

    QTableWidget* t = ui->tableWidget_2;
    t->setRowCount(0);

    QSqlQuery q(
        "SELECT id_stock, nom_stock, categ_stock, "
        "       TO_CHAR(dateajt_stock, 'DD/MM/YYYY'), qt_stock, descript_stock "
        "FROM STOCK "
        "ORDER BY id_stock DESC");

    auto setCell = [t](int row, int col, const QString& text) {
        auto* it = new QTableWidgetItem(text);
        it->setFlags(it->flags() & ~Qt::ItemIsEditable);
        t->setItem(row, col, it);
    };

    while (q.next()) {
        const int row = t->rowCount();
        t->insertRow(row);
        setCell(row, 0, q.value(0).toString());
        setCell(row, 1, q.value(1).toString());
        setCell(row, 2, q.value(2).toString());
        setCell(row, 3, q.value(3).toString());
        setCell(row, 4, q.value(4).toString());
        setCell(row, 5, q.value(5).toString());
    }
}


void MainWindow::on_btnConsulterstc_clicked()
{
    const int stockIdx = moduleIndex(ui->module2);
    ensureModuleIndex(stockIdx);
    loadStocksTable();
    crossFadeToIndex(ui->metiersstocks, 1); // consulterqtolives
}

void MainWindow::on_btnAjouterstc_clicked()
{
    const int stockIdx = moduleIndex(ui->module2);
    ensureModuleIndex(stockIdx);
    refreshStockSerieChoices();
    crossFadeToIndex(ui->metiersstocks, 0); // ajoutqtolives
}

void MainWindow::on_ajouterqtoliveBtn_clicked()
{
    const QString nomStock = ui->nomLineEdit_2 ? ui->nomLineEdit_2->text().trimmed() : QString();
    const QString prenomAgri = ui->prNomLineEdit_2 ? ui->prNomLineEdit_2->text().trimmed() : QString();
    const QString categorie = ui->Categchoix ? ui->Categchoix->currentText().trimmed() : QString();
    const QDate dateAjout = ui->dateDEmbaucheDateEdit_2 ? ui->dateDEmbaucheDateEdit_2->date() : QDate::currentDate();
    const QString qteText = ui->prNomLineEdit_3 ? ui->prNomLineEdit_3->text().trimmed() : QString();
    const QString desc = ui->description ? ui->description->text().trimmed() : QString();
    const int serieId = (ui->stockSerieCombo ? ui->stockSerieCombo->currentData().toInt() : 0);

    bool okQte = false;
    const double qte = qteText.toDouble(&okQte);

    if (nomStock.isEmpty()) {
        QMessageBox::warning(this, tr("Validation"), tr("Le nom du stock est obligatoire."));
        return;
    }
    if (!okQte || qte <= 0.0) {
        QMessageBox::warning(this, tr("Validation"), tr("La quantité doit être un nombre positif."));
        return;
    }
    if (serieId <= 0) {
        QMessageBox::warning(this, tr("Validation"), tr("Veuillez choisir une série associée."));
        return;
    }

    // Optional agriculteur resolution from Nom + Prénom (kept nullable if not found).
    QVariant agriId = QVariant(QMetaType(QMetaType::Int)); // typed NULL (NUMBER) for Oracle
    QSqlQuery qAgri;
    qAgri.prepare(
        "SELECT id_agri FROM AGRICULTEUR "
        "WHERE UPPER(nom_agri) = UPPER(:nom) "
        "  AND UPPER(NVL(prenom_agri, '')) = UPPER(:prenom) "
        "  AND ROWNUM = 1");
    qAgri.bindValue(":nom", nomStock);
    qAgri.bindValue(":prenom", prenomAgri);
    if (qAgri.exec() && qAgri.next())
        agriId = qAgri.value(0);

    QSqlQuery q;
    q.prepare(
        "INSERT INTO STOCK (nom_stock, categ_stock, dateajt_stock, descript_stock, qt_stock, id_agri, id_serie) "
        "VALUES (:nom, :categ, :dateajt, :descr, :qt, :id_agri, :id_serie)");
    q.bindValue(":nom", nomStock);
    q.bindValue(":categ", categorie);
    q.bindValue(":dateajt", dateAjout);
    q.bindValue(":descr", desc);
    q.bindValue(":qt", qte);
    q.bindValue(":id_agri", agriId);
    q.bindValue(":id_serie", serieId);

    if (!q.exec()) {
        QMessageBox::critical(this, tr("Erreur"),
            tr("Impossible d'ajouter le stock :\n%1").arg(q.lastError().text()));
        return;
    }

    QString autoAssignMsg;
    if (m_autoAssignFromStock) {
        QString detail;
        const bool assigned = tryAutoAssignForSerie(serieId, detail);
        autoAssignMsg = assigned
            ? QStringLiteral("\n") + detail
            : QStringLiteral("\n") + tr("Affectation auto non réalisée : %1").arg(detail);
    }

    // Refresh UI
    if (ui->nomLineEdit_2) ui->nomLineEdit_2->clear();
    if (ui->prNomLineEdit_2) ui->prNomLineEdit_2->clear();
    if (ui->prNomLineEdit_3) ui->prNomLineEdit_3->clear();
    if (ui->description) ui->description->clear();
    if (ui->dateDEmbaucheDateEdit_2) ui->dateDEmbaucheDateEdit_2->setDate(QDate::currentDate());
    if (ui->Categchoix) ui->Categchoix->setCurrentIndex(0);
    if (ui->stockSerieCombo) ui->stockSerieCombo->setCurrentIndex(0);

    loadStocksTable();

    QMessageBox::information(this, tr("Succès"),
        tr("Stock ajouté avec succès.%1").arg(autoAssignMsg));
}

void MainWindow::on_btnStatstc_clicked()
{
    const int stockIdx = moduleIndex(ui->module2);
    ensureModuleIndex(stockIdx);
    crossFadeToIndex(ui->metiersstocks, 2); // statqtolives
}

void MainWindow::on_toolButton_5_clicked()
{
    const int stockIdx = moduleIndex(ui->module2);
    ensureModuleIndex(stockIdx);
    crossFadeToIndex(ui->metiersstocks, 3); // metieravancee_2
}

// ── Module toolbar navigation handlers (Citernes / Qualité / Machines / Agriculteurs)
// Module 3 (Citernes) toolbar actions
void MainWindow::on_AjoutCiterne_clicked()
{
    const int citerneIdx = moduleIndex(ui->module3);
    openModulePage(ui->metiersCiternes, citerneIdx, 0); // ajoutCiternes
}

void MainWindow::on_ConsulterCiterne_clicked()
{
    const int citerneIdx = moduleIndex(ui->module3);
    openModulePage(ui->metiersCiternes, citerneIdx, 1); // consulterciterne
}

void MainWindow::on_StatistiqueCiterne_clicked()
{
    const int citerneIdx = moduleIndex(ui->module3);
    openModulePage(ui->metiersCiternes, citerneIdx, 2); // statCiterne
}

void MainWindow::on_MetierAvanceCiterne_clicked()
{
    const int citerneIdx = moduleIndex(ui->module3);
    openModulePage(ui->metiersCiternes, citerneIdx, 3); // AvCiterne
}

// Module 4 (Qualité) toolbar actions
void MainWindow::on_btnConsulterQualite_clicked()
{
    const int qualiteIdx = moduleIndex(ui->module4);
    openModulePage(ui->metiersqualite, qualiteIdx, 1); // consulterpersonnel_2
}

void MainWindow::on_btnAjouterQualite_clicked()
{
    const int qualiteIdx = moduleIndex(ui->module4);
    openModulePage(ui->metiersqualite, qualiteIdx, 0); // ajoutpersonnel_2
}

void MainWindow::on_btnStatQualite_clicked()
{
    const int qualiteIdx = moduleIndex(ui->module4);
    openModulePage(ui->metiersqualite, qualiteIdx, 2); // statPersonnel_2
}

void MainWindow::on_btnAdvEmp_2_clicked()
{
    const int qualiteIdx = moduleIndex(ui->module4);
    openModulePage(ui->metiersqualite, qualiteIdx, 3); // metieravancee_3
}

// Module 5 (Machines) toolbar actions
void MainWindow::on_btnConsulterMachines_clicked()
{
    const int machinesIdx = moduleIndex(ui->module5);
    openModulePage(ui->metierspersonnel_2, machinesIdx, 1); // consulterpersonnel_3
}

void MainWindow::on_btnAjouterMachines_clicked()
{
    const int machinesIdx = moduleIndex(ui->module5);
    openModulePage(ui->metierspersonnel_2, machinesIdx, 0); // ajoutpersonnel_3
}

void MainWindow::on_btnStatMachines_clicked()
{
    const int machinesIdx = moduleIndex(ui->module5);
    openModulePage(ui->metierspersonnel_2, machinesIdx, 2); // statPersonnel_3
}

void MainWindow::on_btnAvanceMachines_clicked()
{
    const int machinesIdx = moduleIndex(ui->module5);
    openModulePage(ui->metierspersonnel_2, machinesIdx, 3); // metieravancee_4
}

// Module 6 (Agriculteurs) toolbar actions
void MainWindow::on_btnConsulterAgr_clicked()
{
    const int agriIdx = moduleIndex(ui->module6);
    openModulePage(ui->metiersagriculteurs, agriIdx, 1); // consulteragriculteur
}

void MainWindow::on_btnAjouterAgr_clicked()
{
    const int agriIdx = moduleIndex(ui->module6);
    openModulePage(ui->metiersagriculteurs, agriIdx, 0); // ajoutagriculteur
}

void MainWindow::on_btnStatAgr_clicked()
{
    const int agriIdx = moduleIndex(ui->module6);
    openModulePage(ui->metiersagriculteurs, agriIdx, 2); // statAGriculteur
}

void MainWindow::on_btnAvanceAgr_clicked()
{
    const int agriIdx = moduleIndex(ui->module6);
    openModulePage(ui->metiersagriculteurs, agriIdx, 3); // metieravancee_5
}

// ── Sidebar navigation & active-button state ────────────────────────────────
// Sidebar navigation: map buttons to modules indices
// Order in UI: module1 (0), module3 (1), module4 (2), module5 (3), module6 (4), module2 (5)
void MainWindow::on_btnmod1_clicked()
{
    openSidebarModule(0, ui->metierspersonnel, 0, 0);
}

void MainWindow::on_btnmod2_clicked()
{
    openSidebarModule(moduleIndex(ui->module2), ui->metiersstocks, 0, 5, true);
}

void MainWindow::on_btnmod3_clicked()
{
    openSidebarModule(moduleIndex(ui->module3), ui->metiersCiternes, 0, 1);
}

void MainWindow::on_btnmod4_clicked()
{
    openSidebarModule(moduleIndex(ui->module4), ui->metiersqualite, 0, 2);
}

void MainWindow::on_btnmod5_clicked()
{
    openSidebarModule(moduleIndex(ui->module5), ui->metierspersonnel_2, 0, 3);
}

void MainWindow::on_btnmod6_clicked()
{
    openSidebarModule(moduleIndex(ui->module6), ui->metiersagriculteurs, 0, 4);
}

void MainWindow::on_btnSettings_clicked()
{
    openSidebarModule(moduleIndex(ui->module7), nullptr, -1, 6);
}

// Helper to visually mark active module button
void MainWindow::setActiveModuleButton(int index)
{
    QPushButton* allButtons[] = {
        ui->btnmod1, ui->btnmod2, ui->btnmod3, ui->btnmod4,
        ui->btnmod5, ui->btnmod6, ui->btnSettings
    };
    for (QPushButton* b : allButtons) {
        if (b) b->setChecked(false);
    }

    // module index -> sidebar button mapping
    QPushButton* mapped[] = {
        ui->btnmod1,    // module1
        ui->btnmod3,    // module3
        ui->btnmod4,    // module4
        ui->btnmod5,    // module5
        ui->btnmod6,    // module6
        ui->btnmod2,    // module2
        ui->btnSettings // module7
    };

    const int clamped = (index >= 0 && index < 7) ? index : 0;
    if (mapped[clamped]) mapped[clamped]->setChecked(true);
}

// ── Generic UI helpers (page switch / avatar / layout reactions) ───────────
int MainWindow::moduleIndex(QWidget* moduleWidget) const
{
    if (!ui || !ui->modules || !moduleWidget) return -1;
    return ui->modules->indexOf(moduleWidget);
}

void MainWindow::ensureModuleIndex(int moduleIndex)
{
    if (!ui || !ui->modules) return;
    if (moduleIndex < 0 || moduleIndex >= ui->modules->count()) return;
    if (ui->modules->currentIndex() == moduleIndex) return;
    crossFadeToIndex(ui->modules, moduleIndex);
}

void MainWindow::openModulePage(QStackedWidget* modulePages, int moduleIndex, int pageIndex)
{
    ensureModuleIndex(moduleIndex);
    crossFadeToIndex(modulePages, pageIndex);
}

void MainWindow::openSidebarModule(int moduleIndex, QStackedWidget* modulePages, int pageIndex,
                                   int activeButtonIndex, bool refreshStockChoices)
{
    if (ui && ui->MainStacked) ui->MainStacked->setCurrentIndex(1);
    ensureModuleIndex(moduleIndex);
    if (modulePages && pageIndex >= 0) crossFadeToIndex(modulePages, pageIndex);
    if (refreshStockChoices) refreshStockSerieChoices();
    setActiveModuleButton(activeButtonIndex);
}

// Simplified: no transition animation; just switch pages.
void MainWindow::crossFadeToIndex(QStackedWidget* stack, int newIndex)
{
    if (!stack || newIndex < 0 || newIndex >= stack->count()) return;
    stack->setEnabled(true);
    stack->setCurrentIndex(newIndex);
}

// Keep the user info container anchored at the top-right of the modules area
void MainWindow::repositionUserInfo()
{
    // userInfoContainer is layout-managed inside contentArea; no manual geometry needed
    if (!ui->userInfoContainer || !ui->modules) return;
    if (ui->mainprogram && ui->mainprogram->layout()) return;
}

// Render the user avatar pixmap as a true circle with antialiasing
void MainWindow::makeAvatarCircular()
{
    if (!ui || !ui->userAvatar) return;

    const QSize targetSize = ui->userAvatar->size(); // e.g. 40x40
    if (targetSize.isEmpty()) return;

    // Get the source pixmap (fallback to resource if label has none)
    QPixmap src;
    QPixmap current = ui->userAvatar->pixmap();
    if (!current.isNull()) {
        src = current;
    } else {
        src.load(QStringLiteral(":/img/logo.png"));
    }
    if (src.isNull()) return;

    // Scale preserving aspect ratio (expand) and center-crop to target square
    QPixmap scaled = src.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QRect cropRect((scaled.width() - targetSize.width()) / 2,
                   (scaled.height() - targetSize.height()) / 2,
                   targetSize.width(), targetSize.height());
    QPixmap square = scaled.copy(cropRect);

    // Draw into a transparent canvas clipped to a circular path
    QPixmap circle(targetSize);
    circle.fill(Qt::transparent);
    QPainter painter(&circle);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path;
    path.addEllipse(circle.rect());
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, square);
    painter.end();

    // Apply to the label; disable scaledContents since we match the label size
    ui->userAvatar->setScaledContents(false);
    ui->userAvatar->setPixmap(circle);
}

// Position the chat launcher at the bottom-right of the main content (central widget)
void MainWindow::repositionChatLauncher()
{
    if (!m_chatLauncher || !ui->centralwidget) return;
    const int margin = 12;
    const QPoint origin = QPoint(0,0);
    int x = origin.x() + ui->centralwidget->width() - m_chatLauncher->width() - margin;
    int y = origin.y() + ui->centralwidget->height() - m_chatLauncher->height() - margin;
    m_chatLauncher->move(x, y);
    m_chatLauncher->raise();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    repositionUserInfo();
    repositionChatLauncher();
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    // Track touched state for employee-form fields (only after user focuses a field)
    if ((obj == ui->nomLineEdit || obj == ui->prNomLineEdit || obj == ui->emailLineEdit
        || obj == ui->roleComboBox || obj == ui->mdpLineEdit)
        && event->type() == QEvent::FocusIn) {
        if (auto* w = qobject_cast<QWidget*>(obj)) {
            w->setProperty("touched", true);
            validateEmployeeForm(true);
        }
    }

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

// Update the bottom-center clock label with current system time/date
void MainWindow::updateClock()
{
    if (!m_clockLabel || !ui->statusbar) return;
    const int w = ui->statusbar->width();
    QString fmt;
    if (w < 360) {
        fmt = QStringLiteral("hh:mm");
    } else if (w < 520) {
        fmt = QStringLiteral("hh:mm  dd/MM");
    } else {
        fmt = QStringLiteral("hh:mm:ss  dd/MM/yyyy");
    }
    m_clockLabel->setText(QDateTime::currentDateTime().toString(fmt));
}

// Sidebar collapse/expand animation
void MainWindow::animateSidebarToggle(bool collapse)
{
    // Simplified: no animation. Snap sidebar width.
    const int expandedMin = 200;
    const int expandedMax = 220;
    const int collapsedW  = 48;

    if (!ui || !ui->sidebar) return;
    if (collapse) {
        ui->sidebar->setMinimumWidth(collapsedW);
        ui->sidebar->setMaximumWidth(collapsedW);
    } else {
        ui->sidebar->setMinimumWidth(expandedMin);
        ui->sidebar->setMaximumWidth(expandedMax);
    }
    m_sidebarCollapsed = collapse;
}

// Hook up interactive behaviors (toggle button, live search)
void MainWindow::setupInteractiveHooks()
{
    if (ui->sidebar) {
        // Sidebar toggle button now lives in .ui.
        if (ui->sidebarToggleBtn && !ui->sidebarToggleBtn->property("wiredClicked").toBool()) {
            QObject::connect(ui->sidebarToggleBtn, &QToolButton::clicked, this, [this]() {
                animateSidebarToggle(!m_sidebarCollapsed);
            });
            ui->sidebarToggleBtn->setProperty("wiredClicked", true);
        }

        // Install event filter on sidebar to allow click-to-expand.
        ui->sidebar->installEventFilter(this);

        // Adjust cursor when collapsed.
        const auto updateSidebarCursor = [this]() {
            if (!ui || !ui->sidebar) return;
            if (m_sidebarCollapsed || ui->sidebar->width() <= 60)
                ui->sidebar->setCursor(Qt::PointingHandCursor);
            else
                ui->sidebar->unsetCursor();
        };

        if (!this->property("sidebarCursorHooked").toBool()) {
            QObject::connect(this, &MainWindow::windowTitleChanged, this,
                             [updateSidebarCursor](const QString&) { updateSidebarCursor(); });
            this->setProperty("sidebarCursorHooked", true);
        }
        updateSidebarCursor();
    }

    // (Simplified) No keyboard shortcut hook.

    // Live filter for personnel table
    if (ui->lineEdit && ui->comboBox && ui->tableEmp) {
        if (!ui->lineEdit->property("wiredTextChanged").toBool()) {
            QObject::connect(ui->lineEdit, &QLineEdit::textChanged, this,
                             [this](const QString&) { filterPersonnelTable(); });
            ui->lineEdit->setProperty("wiredTextChanged", true);
        }
        if (!ui->comboBox->property("wiredCurrentTextChanged").toBool()) {
            QObject::connect(ui->comboBox, &QComboBox::currentTextChanged, this,
                             [this](const QString&) { filterPersonnelTable(); });
            ui->comboBox->setProperty("wiredCurrentTextChanged", true);
        }
    }

    // Live remaining-slots info for affectation form
    if (ui->affEmpCombo) {
        if (!ui->affEmpCombo->property("wiredCurrentIndexChanged").toBool()) {
            QObject::connect(ui->affEmpCombo, &QComboBox::currentIndexChanged, this,
                             [this](int) { updateAffectationRemainingInfo(); });
            ui->affEmpCombo->setProperty("wiredCurrentIndexChanged", true);
        }
    }
}

// ── Form validation and UX behavior setup ───────────────────────────────────
void MainWindow::setupEmployeeFormValidation()
{
    if (!ui->formLayout || !ui->formLayoutWidget)
        return;

    const auto resolveValidationHost = [this]() -> QWidget* {
        if (ui->module1) return static_cast<QWidget*>(ui->module1);
        if (ui->ajoutpersonnel) return static_cast<QWidget*>(ui->ajoutpersonnel);
        return ui->formLayoutWidget ? ui->formLayoutWidget->parentWidget() : nullptr;
    };

    // Keep the form compact enough so the submit row is visible without excessive clipping.
    ui->formLayout->setVerticalSpacing(22);

    // Prevent placeholder/text clipping on dense DPI/font setups.
    const int fieldMinH = 34;
    for (QWidget* field : { static_cast<QWidget*>(ui->nomLineEdit),
                            static_cast<QWidget*>(ui->prNomLineEdit),
                            static_cast<QWidget*>(ui->emailLineEdit),
                            static_cast<QWidget*>(ui->mdpLineEdit),
                            static_cast<QWidget*>(ui->roleComboBox),
                            static_cast<QWidget*>(ui->dateDEmbaucheDateEdit) }) {
        if (field) field->setMinimumHeight(fieldMinH);
    }
    if (ui->ajouterEmpBtn) {
        ui->ajouterEmpBtn->setMinimumHeight(36);
        ui->ajouterEmpBtn->show();
        ui->ajouterEmpBtn->raise();
    }

    if (ui->enrollFingerprintBtn) {
        ui->enrollFingerprintBtn->setMinimumHeight(34);
        if (!ui->enrollFingerprintBtn->property("wiredClicked").toBool()) {
            QObject::connect(ui->enrollFingerprintBtn, &QPushButton::clicked,
                             this, [this]() { startFingerprintEnrollmentFromForm(); });
            ui->enrollFingerprintBtn->setProperty("wiredClicked", true);
        }
    }

    QWidget* validationHost = resolveValidationHost();
    if (!validationHost)
        validationHost = ui->formLayoutWidget;

    QLabel* feedback = ui->employeeValidationLabel;
    if (!feedback)
        return;

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
        QLabel* fb = ui->employeeValidationLabel;
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
    QWidget* touchFields[] = {
        ui->nomLineEdit,
        ui->prNomLineEdit,
        ui->emailLineEdit,
        ui->roleComboBox,
        ui->mdpLineEdit
    };
    for (QWidget* field : touchFields) {
        if (!field) continue;
        field->setProperty("touched", false);
        field->installEventFilter(this);
    }

    auto liveValidate = [this]() { validateEmployeeForm(true); };
    QLineEdit* liveLineEdits[] = {
        ui->nomLineEdit,
        ui->prNomLineEdit,
        ui->emailLineEdit,
        ui->mdpLineEdit
    };
    for (QLineEdit* edit : liveLineEdits) {
        if (!edit) continue;
        QObject::connect(edit, &QLineEdit::textChanged, this,
                         [liveValidate](const QString&) { liveValidate(); });
    }
    if (ui->roleComboBox) {
        QObject::connect(ui->roleComboBox, &QComboBox::currentTextChanged, this,
                         [liveValidate](const QString&) { liveValidate(); });
    }

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

// Validates employee form fields and updates inline visual feedback.
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
    QLabel* feedback = ui->employeeValidationLabel;
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

// Raise toolbar containers to avoid overlap and enforce comfortable button height
void MainWindow::setupToolbarsTweaks()
{
    // Raise toolbar layout widgets to ensure they are not overlapped by stacked content
    auto raiseIf = [](QWidget* w){ if (w) w->raise(); };
    raiseIf(ui->horizontalLayoutWidget_3);
    raiseIf(ui->horizontalLayoutWidget_4);
    raiseIf(ui->horizontalLayoutWidget_5);
    raiseIf(ui->horizontalLayoutWidget_6);
    raiseIf(ui->horizontalLayoutWidget_7);
    raiseIf(ui->horizontalLayoutWidget_8);

    // Also set a minimum height on the actual toolbuttons inside each container
    auto tuneButtons = [](QWidget* container){
        if (!container) return;
        const auto buttons = container->findChildren<QToolButton*>();
        for (auto* b : buttons) {
            b->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            b->setIconSize(QSize(24,24));
            b->setMinimumHeight(64);
            b->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        }
    };
    tuneButtons(ui->horizontalLayoutWidget_3);
    tuneButtons(ui->horizontalLayoutWidget_4);
    tuneButtons(ui->horizontalLayoutWidget_5);
    tuneButtons(ui->horizontalLayoutWidget_6);
    tuneButtons(ui->horizontalLayoutWidget_7);
    tuneButtons(ui->horizontalLayoutWidget_8);

    // Ensure button text is never elided: compute required width via style metrics and set minimums
    auto ensureTextVisible = [](QWidget* container){
        if (!container) return;
        const auto buttons = container->findChildren<QToolButton*>();
        for (auto* b : buttons) {
            // Ask the style for an accurate size that accounts for icon, text, padding, and underline
            QStyleOptionToolButton opt;
            opt.initFrom(b);
            opt.text = b->text();
            opt.icon = b->icon();
            opt.toolButtonStyle = b->toolButtonStyle();
            opt.font = b->font();
            if (b->menu()) opt.features |= QStyleOptionToolButton::Menu;
            const QSize styleSz = b->style()->sizeFromContents(QStyle::CT_ToolButton, &opt, QSize(), b);

            // Fallback: also compute robust text width for safety across styles
            QFontMetrics fm(b->font());
            const int textW = fm.boundingRect(b->text()).width();
            const int computed = qMax(styleSz.width(), textW + 32);

            if (computed > b->minimumWidth()) {
                b->setMinimumWidth(computed);
                b->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            }
            // Ensure no artificial max width forces elision
            b->setMaximumWidth(QWIDGETSIZE_MAX);
        }
    };
    ensureTextVisible(ui->horizontalLayoutWidget_3);
    ensureTextVisible(ui->horizontalLayoutWidget_4);
    ensureTextVisible(ui->horizontalLayoutWidget_5);
    ensureTextVisible(ui->horizontalLayoutWidget_6);
    ensureTextVisible(ui->horizontalLayoutWidget_7);
    ensureTextVisible(ui->horizontalLayoutWidget_8);
}

// Simple filter on the personnel table based on combo selection
void MainWindow::filterPersonnelTable()
{
    if (!ui->tableEmp || !ui->comboBox) return;
    QString needle = ui->lineEdit ? ui->lineEdit->text().trimmed() : QString();
    QString mode = ui->comboBox->currentText();

    int col = 0; // default: ID
    if (mode.compare(QStringLiteral("Nom"), Qt::CaseInsensitive) == 0)
        col = 1;
    else if (mode.compare(QStringLiteral("Email"), Qt::CaseInsensitive) == 0)
        col = 3;
    else if (mode.compare(QStringLiteral("R\u00f4le"), Qt::CaseInsensitive) == 0)
        col = 4;

    for (int r = 0; r < ui->tableEmp->rowCount(); ++r) {
        auto* item = ui->tableEmp->item(r, col);
        bool match = needle.isEmpty() || (item && item->text().contains(needle, Qt::CaseInsensitive));
        ui->tableEmp->setRowHidden(r, !match);
    }
}

// ── Charts / analytics rendering ────────────────────────────────────────────
namespace {
void clearLayoutWidgets(QLayout* layout)
{
    if (!layout) return;
    QLayoutItem* item = nullptr;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }
}

QGridLayout* ensureChartGrid(QWidget* container,
                             const QMargins& margins = QMargins(0, 0, 0, 0),
                             int hSpacing = 12,
                             int vSpacing = 12)
{
    if (!container) return nullptr;
    auto* grid = qobject_cast<QGridLayout*>(container->layout());
    if (!grid) grid = new QGridLayout(container);
    grid->setContentsMargins(margins);
    grid->setHorizontalSpacing(hSpacing);
    grid->setVerticalSpacing(vSpacing);
    return grid;
}

QChart* createBaseChart(const QString& title,
                        bool showLegend = true,
                        Qt::Alignment legendAlignment = Qt::AlignRight)
{
    auto* chart = new QChart();
    chart->setTitle(title);
    chart->setTitleFont(QFont(QStringLiteral("Segoe UI"), 11, QFont::Bold));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(6, 6, 6, 6));
    if (chart->legend()) {
        chart->legend()->setVisible(showLegend);
        chart->legend()->setAlignment(legendAlignment);
    }
    return chart;
}

QChartView* makeChartView(QChart* chart, int minHeight = 260)
{
    auto* view = new QChartView(chart);
    view->setRenderHint(QPainter::Antialiasing);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setMinimumHeight(minHeight);
    return view;
}

QChartView* makePieChartView(const QString& title,
                             const QList<QPair<QString, qreal>>& values,
                             const QList<QColor>& palette = {},
                             Qt::Alignment legendAlignment = Qt::AlignRight)
{
    auto* series = new QPieSeries();
    qreal total = 0.0;
    for (const auto& it : values) total += qMax<qreal>(0.0, it.second);

    for (int i = 0; i < values.size(); ++i) {
        const QString& label = values[i].first;
        const qreal value = qMax<qreal>(0.0, values[i].second);
        if (value <= 0.0) continue;

        const qreal pct = (total > 0.0) ? (value * 100.0 / total) : 0.0;
        auto* slice = series->append(QStringLiteral("%1 — %2 (%3%)")
                                         .arg(label)
                                         .arg(QString::number(value, 'f', 0))
                                         .arg(QString::number(pct, 'f', 1)),
                                     value);
        slice->setLabelVisible(true);
        slice->setLabelArmLengthFactor(0.10);
        slice->setBorderColor(Qt::white);
        if (i < palette.size()) slice->setColor(palette[i]);
    }

    auto* chart = createBaseChart(title, true, legendAlignment);
    chart->addSeries(series);
    return makeChartView(chart, 280);
}
}

void MainWindow::setupPersonnelChart()
{
    // Use live DB-backed stats rendering as the single personnel chart source.
    loadEmployeeStats();
}

void MainWindow::loadEmployeeStats()
{
    QWidget* container = ui->chartStatusContainer;
    if (!container) return;

    // ── clear any previous charts ─────────────────────────────────────────
    QGridLayout* grid = ensureChartGrid(container, QMargins(10, 10, 10, 10), 12, 12);
    clearLayoutWidgets(grid);

    // ── query: COUNT per role ─────────────────────────────────────────────
    QSqlQuery q;
    q.prepare("SELECT role, COUNT(*) AS nb FROM EMPLOYE GROUP BY role ORDER BY role");
    if (!q.exec()) {
        QLabel* err = new QLabel("Erreur DB: " + q.lastError().text(), container);
        err->setAlignment(Qt::AlignCenter);
        err->setStyleSheet("color: red; font-size: 12pt;");
        grid->addWidget(err, 0, 0);
        return;
    }

    QMap<QString, int> roleCount;
    int total = 0;
    while (q.next()) {
        roleCount[q.value(0).toString()] = q.value(1).toInt();
        total += q.value(1).toInt();
    }

    if (total == 0) {
        QLabel* empty = new QLabel("Aucun employé dans la base de données.", container);
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet("font-size: 12pt; color: gray;");
        grid->addWidget(empty, 0, 0);
        return;
    }

    QLabel* title = new QLabel(QString("Statistiques des employés  —  %1 employé(s) au total").arg(total));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 14pt; font-weight: bold; padding: 6px;");

    // ── fixed colours per role ────────────────────────────────────────────
    QMap<QString, QColor> roleColors;
    roleColors["Administrateur"] = QColor("#2E5265");
    roleColors["Manager"]        = QColor("#4A90D9");
    roleColors["Technicien"]     = QColor("#50C878");
    roleColors["Operateur"]      = QColor("#F4A460");

    QList<QPair<QString, qreal>> pieData;
    QList<QColor> piePalette;
    for (auto it = roleCount.constBegin(); it != roleCount.constEnd(); ++it) {
        pieData.append({it.key(), static_cast<qreal>(it.value())});
        piePalette.append(roleColors.value(it.key(), QColor("#90a4ae")));
    }
    QChartView* pieView = makePieChartView(tr("Répartition par rôle"), pieData, piePalette, Qt::AlignRight);

    // ══ CHART 2: Bar — nombre d'employés par rôle ════════════════════════
    QBarSet* barSet = new QBarSet("Employés");
    QStringList categories;
    for (auto it = roleCount.constBegin(); it != roleCount.constEnd(); ++it) {
        *barSet << it.value();
        categories << it.key();
    }
    if (roleColors.contains(categories.first()))
        barSet->setColor(roleColors[categories.first()]);

    QBarSeries* barSeries = new QBarSeries();
    barSeries->append(barSet);
    barSeries->setLabelsVisible(true);
    barSeries->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);

    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setTitleText("Rôle");

    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Nombre d'employés");
    axisY->setLabelFormat("%d");
    axisY->setRange(0, total + 1);

    QChart* barChart = createBaseChart(tr("Nombre d'employés par rôle"), false);
    barChart->addSeries(barSeries);
    barChart->addAxis(axisX, Qt::AlignBottom);
    barChart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisX);
    barSeries->attachAxis(axisY);
    QChartView* barView = makeChartView(barChart, 280);

    // ── assemble layout ───────────────────────────────────────────────────
    grid->addWidget(title, 0, 0, 1, 2);
    grid->addWidget(pieView, 1, 0);
    grid->addWidget(barView, 1, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    grid->setRowStretch(1, 1);
}

void MainWindow::setupCiterneChart()
{
    if (!ui->chartStatusContainer_3) return;
    QGridLayout *grid = ensureChartGrid(ui->chartStatusContainer_3, QMargins(0, 0, 0, 0), 12, 12);
    if (!grid || grid->count() > 0) return;

    // Pie: Répartition de l'état des citernes
    QChartView *etatView = makePieChartView(
        tr("État des citernes"),
        {{tr("Opérationnel"), 12}, {tr("Maintenance"), 3}, {tr("Hors service"), 1}},
        {QColor("#50C878"), QColor("#F4A460"), QColor("#D9534F")},
        Qt::AlignRight);

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
    QChart *capChart = createBaseChart(tr("Capacité totale par type"), true, Qt::AlignBottom);
    capChart->addSeries(capSeries);
    QStringList cats; cats << "Nord" << "Centre" << "Sud";
    QBarCategoryAxis *axisX = new QBarCategoryAxis(); axisX->append(cats);
    QValueAxis *axisY = new QValueAxis(); axisY->setTitleText("m³"); axisY->setRange(0, 80);
    capChart->addAxis(axisX, Qt::AlignBottom); capChart->addAxis(axisY, Qt::AlignLeft);
    capSeries->attachAxis(axisX); capSeries->attachAxis(axisY);
    QChartView *capView = makeChartView(capChart, 260);

    // Line: Température moyenne (semaine)
    QLineSeries *tempSeries = new QLineSeries();
    tempSeries->setName("Température moyenne");
    tempSeries->append(0, 18); tempSeries->append(1, 19); tempSeries->append(2, 20);
    tempSeries->append(3, 21); tempSeries->append(4, 20); tempSeries->append(5, 19);
    QChart *tempChart = createBaseChart(tr("Température (7 jours)"), false);
    tempChart->addSeries(tempSeries);
    QValueAxis *tx = new QValueAxis(); tx->setTitleText("Jour"); tx->setRange(0, 6);
    QValueAxis *ty = new QValueAxis(); ty->setTitleText("°C"); ty->setRange(16, 24);
    tempChart->addAxis(tx, Qt::AlignBottom); tempChart->addAxis(ty, Qt::AlignLeft);
    tempSeries->attachAxis(tx); tempSeries->attachAxis(ty);
    QChartView *tempView = makeChartView(tempChart, 260);

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
    QGridLayout *grid2 = ensureChartGrid(ui->chartStatusContainer_2, QMargins(0, 12, 0, 0), 12, 12);
    if (!grid2 || grid2->count() > 0) return;

    // Pie: Répartition des stocks par catégorie
    QChartView *catView = makePieChartView(
        tr("Stocks par catégorie"),
        {{tr("Extra"), 30}, {tr("Fine"), 25}, {tr("Standard"), 35}, {tr("Déclassé"), 10}},
        {QColor("#43a047"), QColor("#66bb6a"), QColor("#90caf9"), QColor("#ef5350")},
        Qt::AlignRight);

    // Bar: Entrées vs sorties (mois)
    QBarSet *entrees = new QBarSet("Entrées");
    QBarSet *sorties = new QBarSet("Sorties");
    *entrees << 120 << 150 << 130 << 160 << 140;
    *sorties << 100 << 140 << 120 << 150 << 130;
    QBarSeries *flowSeries = new QBarSeries();
    flowSeries->append(entrees); flowSeries->append(sorties);
    QChart *flowChart = createBaseChart(tr("Flux mensuels"), true, Qt::AlignBottom);
    flowChart->addSeries(flowSeries);
    QStringList mois; mois << "Jan" << "Fév" << "Mar" << "Avr" << "Mai";
    QBarCategoryAxis *fx = new QBarCategoryAxis(); fx->append(mois);
    QValueAxis *fy = new QValueAxis(); fy->setTitleText("Qté"); fy->setRange(0, 200);
    flowChart->addAxis(fx, Qt::AlignBottom); flowChart->addAxis(fy, Qt::AlignLeft);
    flowSeries->attachAxis(fx); flowSeries->attachAxis(fy);
    QChartView *flowView = makeChartView(flowChart, 280);
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
    QGridLayout *grid4 = ensureChartGrid(ui->chartStatusContainer_4, QMargins(0, 12, 0, 0), 12, 12);
    if (!grid4 || grid4->count() > 0) return;

    // Pie: Notes qualité
    QChartView *notesView = makePieChartView(
        tr("Distribution des notes"),
        {{QStringLiteral("A"), 40}, {QStringLiteral("B"), 35}, {QStringLiteral("C"), 20}, {QStringLiteral("D"), 5}},
        {QColor("#2e7d32"), QColor("#66bb6a"), QColor("#ffa726"), QColor("#ef5350")},
        Qt::AlignRight);

    // Line: Tendance de la qualité (mois)
    QLineSeries *qualTrend = new QLineSeries();
    qualTrend->setName("Indice qualité");
    qualTrend->append(0, 82); qualTrend->append(1, 84); qualTrend->append(2, 83);
    qualTrend->append(3, 85); qualTrend->append(4, 87); qualTrend->append(5, 88);
    QChart *trendChart = createBaseChart(tr("Tendance qualité (semestre)"), false);
    trendChart->addSeries(qualTrend);
    QValueAxis *qx = new QValueAxis(); qx->setTitleText("Mois"); qx->setRange(0, 5); qx->setTickCount(6);
    QValueAxis *qy = new QValueAxis(); qy->setTitleText("Indice"); qy->setRange(75, 95);
    trendChart->addAxis(qx, Qt::AlignBottom); trendChart->addAxis(qy, Qt::AlignLeft);
    qualTrend->attachAxis(qx); qualTrend->attachAxis(qy);
    QChartView *trendView = makeChartView(trendChart, 280);
    // Arrange side-by-side in one row
    grid4->addWidget(notesView,  0, 0);
    grid4->addWidget(trendView, 0, 1);
    grid4->setColumnStretch(0, 1);
    grid4->setColumnStretch(1, 1);
    grid4->setRowStretch(0, 1);
}

// ── Personnel table population + row action buttons ─────────────────────────
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
    // Called once at startup – just prepare layout; data loaded on demand
    if (!ui->tableEmp) return;
    ui->tableEmp->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableEmp->setSelectionMode(QAbstractItemView::SingleSelection);
    if (ui->tableEmp->verticalHeader())
    ui->tableEmp->verticalHeader()->setDefaultSectionSize(34);
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
    h->setContentsMargins(4, 2, 4, 2);
    h->setSpacing(6);
    h->setAlignment(Qt::AlignCenter);
    container->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    QPushButton* btnModify = new QPushButton(QString(), container);
    btnModify->setObjectName("modifyBtn");
    btnModify->setFocusPolicy(Qt::NoFocus);
    btnModify->setToolTip(tr("Modifier"));
    btnModify->setIcon(QIcon(QStringLiteral(":/img/edit.svg")));
    btnModify->setIconSize(QSize(14, 14));
    btnModify->setFixedSize(30, 28);
    btnModify->setCursor(Qt::PointingHandCursor);
    btnModify->setStyleSheet(QStringLiteral(
        "QPushButton#modifyBtn {"
        "  background-color: #fff8e1;"
        "  border: 1px solid #e6c66e;"
        "  border-radius: 7px;"
        "}"
        "QPushButton#modifyBtn:hover {"
        "  background-color: #ffe9b3;"
        "  border-color: #d9ad43;"
        "}"
        "QPushButton#modifyBtn:pressed {"
        "  background-color: #ffe08a;"
        "}"
    ));

    QPushButton* btnDelete = new QPushButton(QString(), container);
    btnDelete->setObjectName("deleteBtn");
    btnDelete->setFocusPolicy(Qt::NoFocus);
    btnDelete->setToolTip(tr("Supprimer"));
    btnDelete->setIcon(QIcon(QStringLiteral(":/img/delete.svg")));
    btnDelete->setIconSize(QSize(14, 14));
    btnDelete->setFixedSize(30, 28);
    btnDelete->setCursor(Qt::PointingHandCursor);
    btnDelete->setStyleSheet(QStringLiteral(
        "QPushButton#deleteBtn {"
        "  background-color: #fff1f1;"
        "  border: 1px solid #efb1b1;"
        "  border-radius: 7px;"
        "}"
        "QPushButton#deleteBtn:hover {"
        "  background-color: #ffdede;"
        "  border-color: #e58a8a;"
        "}"
        "QPushButton#deleteBtn:pressed {"
        "  background-color: #ffcaca;"
        "}"
    ));

    h->addWidget(btnModify);
    h->addWidget(btnDelete);
    container->setLayout(h);

    int actionsCol = table->columnCount() - 1;
    table->setCellWidget(row, actionsCol, container);

    // ── Modify: pre-fill the add form and switch to it ──────────────────────
    connect(btnModify, &QPushButton::clicked, this, [this]() {
        QTableWidget* t = findOwningTable(sender());
        if (!t) return;
        int r = findRowForButton(t, sender());
        if (r < 0) return;

        // Read row data (col 0=ID, 1=Nom, 2=Prénom, 3=Email, 4=Rôle)
        auto cell = [&](int c) {
            auto* it = t->item(r, c);
            return it ? it->text() : QString();
        };

        bool ok = false;
        int empId = cell(0).toInt(&ok);
        if (!ok || empId <= 0) return;

        // Store the id being edited so the submit button knows to UPDATE
        ui->nomLineEdit->setText(cell(1));
        ui->prNomLineEdit->setText(cell(2));
        ui->emailLineEdit->setText(cell(3));

        // Set role comboBox to the correct entry
        int roleIdx = ui->roleComboBox->findText(cell(4));
        if (roleIdx >= 0) ui->roleComboBox->setCurrentIndex(roleIdx);

        ui->mdpLineEdit->clear();
        ui->photoPathLineEdit->clear();
        m_selectedPhoto.clear();

        // Tag the form with the employee id being modified
        ui->ajouterEmpBtn->setProperty("editingId", empId);
        ui->ajouterEmpBtn->setText(tr("Enregistrer"));

        validateEmployeeForm(true);

        // Navigate to the add/edit form (index 0 = ajoutpersonnel)
        crossFadeToIndex(ui->metierspersonnel, 0);
    });

    // ── Delete: confirm, call DB, reload table ───────────────────────────────
    connect(btnDelete, &QPushButton::clicked, this, [this]() {
        QTableWidget* t = findOwningTable(sender());
        if (!t) return;
        int r = findRowForButton(t, sender());
        if (r < 0) return;

        auto* idItem = t->item(r, 0);
        if (!idItem) return;
        bool ok = false;
        int empId = idItem->text().toInt(&ok);
        if (!ok || empId <= 0) return;

        QString nom = t->item(r, 1) ? t->item(r, 1)->text() : QString::number(empId);
        auto reply = QMessageBox::question(
            this, tr("Confirmer la suppression"),
            tr("Supprimer l'employé \"%1\" (ID %2) ?").arg(nom).arg(empId),
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) return;

        Employe emp;
        if (!emp.supprimer(empId)) {
            QMessageBox::critical(this, tr("Erreur"),
                tr("Impossible de supprimer l'employé :\n%1").arg(emp.lastError().text()));
            return;
        }
        // Reload the table to reflect deletion
        loadEmployeeTable();
    });
}

int MainWindow::findRowForButton(QTableWidget* table, QObject* button) const
{
    if (!button || !table) return -1;

    for (int r = 0; r < table->rowCount(); ++r) {
        QWidget* cell = table->cellWidget(r, table->columnCount() - 1);
        if (!cell) continue;
        // Look for either modify or delete child matching the sender
        auto* mod = cell->findChild<QAbstractButton*>("modifyBtn");
        auto* del = cell->findChild<QAbstractButton*>("deleteBtn");
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
        table->verticalHeader()->setDefaultSectionSize(30);
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
        table->setColumnWidth(last, 96);
    }
}

// ── Face-auth + employee export flows ───────────────────────────────────────
void MainWindow::on_faceBtn_clicked()
{
    if (!m_faceService || !m_faceService->isAvailable()) {
        QMessageBox::warning(this, tr("Indisponible"),
            tr("Les modèles de reconnaissance faciale n'ont pas pu être chargés.\n"
               "Vérifiez que les fichiers .onnx sont présents à côté de l'exécutable."));
        return;
    }

    FaceRecognitionDialog dlg(m_faceService, this);
    const int matchedId = dlg.execAndGetMatchedId();
    if (matchedId <= 0) return;

    // Confirmation step is handled inside the dialog. If we got an id here, user confirmed.
    m_loggedInId = matchedId;

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

void MainWindow::on_exportEmpBtn_clicked()
{
    // Use a modal QDialog instead of QMenu::exec so the format choice always
    // appears centred over the main window and cannot be dismissed by accident.
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Exporter la liste des employés"));
    dlg.setFixedSize(320, 130);

    auto* root = new QVBoxLayout(&dlg);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto* lbl = new QLabel(tr("Choisissez le format d'export :"), &dlg);
    root->addWidget(lbl);

    auto* btnRow = new QHBoxLayout();
    auto* btnPdf = new QPushButton(QIcon(QStringLiteral(":/img/export.svg")),
                                   tr("PDF (.pdf)"), &dlg);
    auto* btnCsv = new QPushButton(QIcon(QStringLiteral(":/img/export.svg")),
                                   tr("Excel (.csv)"), &dlg);
    auto* btnCancel = new QPushButton(tr("Annuler"), &dlg);
    btnPdf->setProperty("type", "primary");
    btnCsv->setProperty("type", "primary");
    btnRow->addWidget(btnPdf);
    btnRow->addWidget(btnCsv);
    btnRow->addWidget(btnCancel);
    root->addLayout(btnRow);

    int choice = 0; // 0=cancel, 1=pdf, 2=csv
    QObject::connect(btnPdf,    &QPushButton::clicked, &dlg, [&](){ choice = 1; dlg.accept(); });
    QObject::connect(btnCsv,    &QPushButton::clicked, &dlg, [&](){ choice = 2; dlg.accept(); });
    QObject::connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted || choice == 0) return;

    if (choice == 1) {
        QString path = QFileDialog::getSaveFileName(
            this, tr("Enregistrer la liste en PDF"),
            QStringLiteral("liste_employes.pdf"),
            tr("PDF (*.pdf)"));
        if (!path.isEmpty()) exportEmployeesToPdf(path);

    } else if (choice == 2) {
        QString path = QFileDialog::getSaveFileName(
            this, tr("Enregistrer la liste Excel"),
            QStringLiteral("liste_employes.csv"),
            tr("CSV Excel (*.csv);;Tous les fichiers (*.*)"));
        if (!path.isEmpty()) exportEmployeesToCsv(path);
    }
}

// ── PDF export ────────────────────────────────────────────────────────────────
void MainWindow::exportEmployeesToPdf(const QString& filePath)
{
    QTableWidget* table = ui->tableEmp;
    if (!table) return;

    // Columns to export (skip the last "Actions" column)
    const int dataCols = table->columnCount() - 1;
    const int rows     = table->rowCount();

    // ── Build an HTML table ──────────────────────────────────────────────────
    QString html;
    html.reserve(4096);
    html += QStringLiteral(
        "<html><head><meta charset='utf-8'/>"
        "<style>"
        "  body { font-family: Arial, sans-serif; font-size: 10pt; }"
        "  h2   { color: #2E5265; margin-bottom: 4px; }"
        "  p.sub { color: #555; font-size: 9pt; margin-top: 0; }"
        "  table { border-collapse: collapse; width: 100%; margin-top: 12px; }"
        "  th { background-color: #2E5265; color: white; padding: 6px 10px;"
        "        text-align: left; font-size: 9pt; }"
        "  td { padding: 5px 10px; font-size: 9pt; border-bottom: 1px solid #ddd; }"
        "  tr:nth-child(even) td { background-color: #f4f8fb; }"
        "</style></head><body>");

    html += QStringLiteral("<h2>Liste des Employ&eacute;s</h2>");
    html += QStringLiteral("<p class='sub'>Smart Oil Press &mdash; Rapport g&eacute;n&eacute;r&eacute; le ")
          + QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy hh:mm"))
          + QStringLiteral("</p>");
    html += QStringLiteral("<table><tr>");

    // Header row
    for (int c = 0; c < dataCols; ++c) {
        auto* hItem = table->horizontalHeaderItem(c);
        html += QStringLiteral("<th>") + (hItem ? hItem->text().toHtmlEscaped() : QString()) + QStringLiteral("</th>");
    }
    html += QStringLiteral("</tr>");

    // Count visible rows (respect active filter)
    int visibleRows = 0;
    for (int r = 0; r < rows; ++r) {
        if (table->isRowHidden(r)) continue;
        ++visibleRows;
        html += QStringLiteral("<tr>");
        for (int c = 0; c < dataCols; ++c) {
            auto* item = table->item(r, c);
            QString text = item ? item->text().toHtmlEscaped() : QString();
            html += QStringLiteral("<td>") + text + QStringLiteral("</td>");
        }
        html += QStringLiteral("</tr>");
    }
    html += QStringLiteral("</table>");
    html += QStringLiteral("<p class='sub' style='margin-top:8px;'>")
          + QString::number(visibleRows) + tr(" employé(s) exporté(s)")
          + QStringLiteral("</p></body></html>");

    // ── Write PDF ────────────────────────────────────────────────────────────
    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    writer.setResolution(150);

    QTextDocument doc;
    doc.setPageSize(QSizeF(writer.width(), writer.height()));
    doc.setHtml(html);
    doc.print(&writer);

    QMessageBox::information(this, tr("Export PDF"),
        tr("La liste a été exportée avec succès :\n%1\n(%2 employé(s))")
            .arg(QDir::toNativeSeparators(filePath)).arg(visibleRows));
}

// ── CSV / Excel export ────────────────────────────────────────────────────────
void MainWindow::exportEmployeesToCsv(const QString& filePath)
{
    QTableWidget* table = ui->tableEmp;
    if (!table) return;

    const int dataCols = table->columnCount() - 1; // skip Actions
    const int rows     = table->rowCount();

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Erreur"),
            tr("Impossible d'écrire dans le fichier :\n%1").arg(filePath));
        return;
    }

    QTextStream out(&file);
    // UTF-8 BOM so Excel opens accented characters correctly
    out.setEncoding(QStringConverter::Utf8);
    out << "\xEF\xBB\xBF";

    // ── Helper: quote a cell value ───────────────────────────────────────────
    auto csvCell = [](const QString& val) -> QString {
        // If value contains comma, semicolon, newline or quote → wrap in quotes
        QString v = val;
        v.replace(QStringLiteral("\""), QStringLiteral("\"\""));
        if (v.contains(QLatin1Char(';')) || v.contains(QLatin1Char(','))
                || v.contains(QLatin1Char('\n')) || v.contains(QLatin1Char('"')))
            return QStringLiteral("\"") + v + QStringLiteral("\"");
        return v;
    };

    // Header row
    QStringList headerCells;
    for (int c = 0; c < dataCols; ++c) {
        auto* hItem = table->horizontalHeaderItem(c);
        headerCells << csvCell(hItem ? hItem->text() : QString());
    }
    out << headerCells.join(QLatin1Char(';')) << "\n";

    // Data rows (only visible ones)
    int visibleRows = 0;
    for (int r = 0; r < rows; ++r) {
        if (table->isRowHidden(r)) continue;
        ++visibleRows;
        QStringList cells;
        for (int c = 0; c < dataCols; ++c) {
            auto* item = table->item(r, c);
            cells << csvCell(item ? item->text() : QString());
        }
        out << cells.join(QLatin1Char(';')) << "\n";
    }
    file.close();

    QMessageBox::information(this, tr("Export Excel"),
        tr("La liste a été exportée avec succès :\n%1\n(%2 employé(s))")
            .arg(QDir::toNativeSeparators(filePath)).arg(visibleRows));
}
