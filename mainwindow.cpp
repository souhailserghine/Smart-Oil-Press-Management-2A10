#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "employe.h"
#include <QDebug>
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
#include <QShortcut>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QParallelAnimationGroup>
#include <QGuiApplication>
#include <QScreen>
#include <QGraphicsDropShadowEffect>
#include <QEvent>
#include <algorithm>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QScrollArea>
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

// OpenCV — facial recognition
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/dnn.hpp>


class HoverShadowFilter : public QObject {
public:
    explicit HoverShadowFilter(QObject* parent = nullptr) : QObject(parent) {}
protected:
    bool eventFilter(QObject* obj, QEvent* ev) override {
        auto* btn = qobject_cast<QToolButton*>(obj);
        if (!btn) return QObject::eventFilter(obj, ev);
        switch (ev->type()) {
            case QEvent::Enter: {
                auto* eff = new QGraphicsDropShadowEffect(btn);
                eff->setBlurRadius(24);
                eff->setOffset(0, 0);
                eff->setColor(QColor(138, 155, 95, 120)); // olive glow
                btn->setGraphicsEffect(eff);
                btn->raise();
                break;
            }
            case QEvent::Leave: {
                btn->setGraphicsEffect(nullptr);
                break;
            }
            default:
                break;
        }
        return QObject::eventFilter(obj, ev);
    }
};
#include <QTimer>
#include <QDateTime>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ── Load OpenCV face models (relative to exe; copied by CMake POST_BUILD) ──
    const std::string detectorModel   = "face_detection_yunet_2023mar.onnx";
    const std::string recognizerModel = "face_recognition_sface_2021dec.onnx";
    try {
        m_faceDetector   = cv::FaceDetectorYN::create(detectorModel,   "", cv::Size(320, 320));
        m_faceRecognizer = cv::FaceRecognizerSF::create(recognizerModel, "");
    } catch (const cv::Exception& e) {
        qWarning() << "[FaceRecog] Failed to load models:" << e.what();
        // Non-fatal — facial recognition will be unavailable but app still works
    }

    // Ensure the avatar image is rendered as a circle
    makeAvatarCircular();
    
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
        qreal dpiX = QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen()->logicalDotsPerInchX() : 96.0;
    qreal dpiY = QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen()->logicalDotsPerInchY() : 96.0;
    int leftMarginPx = static_cast<int>((8.0 / 25.4) * dpiX);  // 8 mm ~ compact left margin for more content width
    int topMarginPx  = static_cast<int>((2.0 / 25.4) * dpiY);  // reduce top offset to ~2 mm to remove excess top space
        contentLayout->setContentsMargins(leftMarginPx, topMarginPx, 0, 0);
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

    // Create floating chat launcher button (lower-most right of the main window content)
    if (ui->centralwidget && !m_chatLauncher) {
        m_chatLauncher = new QToolButton(ui->centralwidget);
        m_chatLauncher->setObjectName(QStringLiteral("chatLauncher"));
        m_chatLauncher->setIcon(QIcon(QStringLiteral(":/img/chat.svg")));
        m_chatLauncher->setIconSize(QSize(24,24));
        m_chatLauncher->setToolTip(tr("Open Chat"));
        m_chatLauncher->setAutoRaise(false);
        m_chatLauncher->setFixedSize(48, 48);
        m_chatLauncher->raise();
        QObject::connect(m_chatLauncher, &QToolButton::clicked, this, [this]() {
            QDialog dlg(this);
            dlg.setWindowTitle(tr("Assistant Chat"));
            dlg.resize(420, 560);
            auto* root = new QVBoxLayout(&dlg);
            root->setContentsMargins(12, 12, 12, 12);
            root->setSpacing(8);
            auto* header = new QLabel(tr("Assistant Chat"), &dlg);
            header->setProperty("type", "heading");
            root->addWidget(header);
            auto* history = new QPlainTextEdit(&dlg);
            history->setReadOnly(true);
            history->setPlaceholderText(tr("Chat history..."));
            root->addWidget(history, 1);
            auto* inputRow = new QHBoxLayout();
            auto* input = new QLineEdit(&dlg);
            input->setPlaceholderText(tr("Type a message"));
            auto* send = new QPushButton(tr("Send"), &dlg);
            send->setProperty("type", "primary");
            inputRow->addWidget(input, 1);
            inputRow->addWidget(send);
            root->addLayout(inputRow);
            QObject::connect(send, &QPushButton::clicked, &dlg, [history, input]() {
                if (input->text().trimmed().isEmpty()) return;
                history->appendPlainText(QStringLiteral("You: ") + input->text().trimmed());
                input->clear();
            });
            dlg.exec();
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

    // Centered system time/date at the bottom in the status bar
    if (ui->statusbar && !m_clockLabel) {
        m_clockLabel = new QLabel(this);
        m_clockLabel->setObjectName(QStringLiteral("clockLabel"));
        m_clockLabel->setMinimumWidth(140);
        m_clockLabel->setAlignment(Qt::AlignCenter);
        m_clockLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        // Create left/right expanding spacers to center the label and keep it visible with messages
        m_clockLeftSpacer = new QWidget(this);
        m_clockRightSpacer = new QWidget(this);
        m_clockLeftSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        m_clockRightSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        ui->statusbar->addPermanentWidget(m_clockLeftSpacer, 1);
        ui->statusbar->addPermanentWidget(m_clockLabel, 0);
        ui->statusbar->addPermanentWidget(m_clockRightSpacer, 1);

        // Update every second
        m_clockTimer = new QTimer(this);
        QObject::connect(m_clockTimer, &QTimer::timeout, this, &MainWindow::updateClock);
        m_clockTimer->start(1000);
        updateClock();
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

    // Make long toolbar rows horizontally scrollable to prevent rightmost button clipping
    auto wrapScrollable = [](QWidget* rowWidget) {
        if (!rowWidget) return;
        // Toolbar rows use absolute geometry (no parent layout manages them).
        // Simply ensure the widget itself does not clip its children by being too small.
        rowWidget->setFixedHeight(72);
    };

    wrapScrollable(ui->horizontalLayoutWidget_3);
    wrapScrollable(ui->horizontalLayoutWidget_4);
    wrapScrollable(ui->horizontalLayoutWidget_5);
    wrapScrollable(ui->horizontalLayoutWidget_6);
    wrapScrollable(ui->horizontalLayoutWidget_7);
    wrapScrollable(ui->horizontalLayoutWidget_8);

    // (No scroll-area wrapping needed; toolbars are 325px wide with 4 buttons — they fit.)
    // (No trailing-spacer padding needed; buttons expand naturally via QHBoxLayout stretch.)
    // (No viewport margin reduction needed.)

    // Install hover shadow filter on all tool buttons within module toolbar rows
    m_hoverShadowFilter = new HoverShadowFilter(this);
    auto installHoverOnRow = [&](QWidget* rowWidget) {
        if (!rowWidget) return;
        const auto buttons = rowWidget->findChildren<QToolButton*>();
        for (auto* b : buttons) {
            b->installEventFilter(m_hoverShadowFilter);
        }
    };
    installHoverOnRow(ui->horizontalLayoutWidget_3);
    installHoverOnRow(ui->horizontalLayoutWidget_4);
    installHoverOnRow(ui->horizontalLayoutWidget_5);
    installHoverOnRow(ui->horizontalLayoutWidget_6);
    installHoverOnRow(ui->horizontalLayoutWidget_7);
    installHoverOnRow(ui->horizontalLayoutWidget_8);

    // Wrap tall stacked pages in scroll areas so bottom action rows (e.g., qjouter*) are always accessible
    auto wrapStackPagesInScroll = [](QStackedWidget* sw){
        if (!sw) return;
        for (int i = 0; i < sw->count(); ++i) {
            QWidget* page = sw->widget(i);
            if (!page) continue;
            // If this stacked page is already a QScrollArea, skip wrapping
            // (previous logic checked parent type and could re-wrap scroll areas, hiding content)
            if (qobject_cast<QScrollArea*>(page)) continue;
            auto* sa = new QScrollArea(sw);
            sa->setFrameShape(QFrame::NoFrame);
            sa->setWidgetResizable(true);
            sa->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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
            // Ensure the inner page prefers to expand to fill the viewport
            page->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
                        // Prefer a specific child form container when present
                        QWidget* form = page->findChild<QWidget*>(QStringLiteral("formLayoutWidget_7"));
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

    // Ensure toolbars are above content and have enough height for text-under-icon
    setupToolbarsTweaks();

    // With the user info bar now layout-managed, explicit repositioning is not required
    repositionUserInfo();
}

MainWindow::~MainWindow()
{
    delete ui;
}



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
    QSqlQuery q;
    q.prepare("SELECT nom_emp, prenom_emp FROM EMPLOYE WHERE id_emp = :id");
    q.bindValue(":id", userId);
    if (q.exec() && q.next()) {
        const QString fullName = q.value(0).toString() + " " + q.value(1).toString();
        if (ui->userNameLabel)
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
    // Switch to "Add Personnel" page (index 0)
    crossFadeToIndex(ui->metierspersonnel, 0);
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

    qDebug() << "[ajouterEmp] nom=" << nom << "prenom=" << prenom
             << "email=" << email << "role=" << role
             << "mdp.size=" << mdp.size()
             << "photo.size=" << m_selectedPhoto.size();

    // ── Check if we are editing an existing employee ────────────────────────
    QVariant editingIdVar = ui->ajouterEmpBtn->property("editingId");
    bool isEditing = editingIdVar.isValid() && editingIdVar.toInt() > 0;
    int editingId  = isEditing ? editingIdVar.toInt() : 0;

    // ── Basic validation ────────────────────────────────────────────────────
    // Mot de passe is required only when adding; optional when editing
    if (nom.isEmpty() || prenom.isEmpty() || email.isEmpty()) {
        QMessageBox::warning(this, tr("Champs requis"),
                             tr("Veuillez remplir tous les champs obligatoires\n"
                                "(Nom, Prénom, Email)."));
        return;
    }
    if (!isEditing && mdp.isEmpty()) {
        QMessageBox::warning(this, tr("Champs requis"),
                             tr("Le mot de passe est obligatoire pour un nouvel employé."));
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

        qDebug() << "[ajouterEmp] faceBlob size:" << faceBlob.size()
                 << "| capturedBlob size:" << m_capturedFaceBlob.size();

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
    // so the status label gives instant feedback
    if (m_capturedFaceBlob.isEmpty() && m_faceDetector && m_faceRecognizer) {
        QByteArray blob = encodeFaceFromFile(path);
        if (!blob.isEmpty()) {
            m_capturedFaceBlob = blob;
            ui->faceStatusLabel->setText(tr("✔ Visage détecté depuis la photo"));
            ui->faceStatusLabel->setStyleSheet("color: #2e7d32;");
        }
    }
}

void MainWindow::on_captureFaceBtn_clicked()
{
    if (!m_faceDetector || !m_faceRecognizer) {
        QMessageBox::warning(this, tr("Indisponible"),
            tr("Les modèles de reconnaissance faciale ne sont pas chargés."));
        return;
    }

    // ── Build live-capture dialog ────────────────────────────────────────────
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Capture du visage"));
    dlg.resize(680, 540);

    auto* root = new QVBoxLayout(&dlg);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);

    auto* camLbl = new QLabel(&dlg);
    camLbl->setAlignment(Qt::AlignCenter);
    camLbl->setMinimumSize(640, 460);
    camLbl->setStyleSheet("background:#111; border-radius:6px;");
    root->addWidget(camLbl, 1);

    auto* infoLbl = new QLabel(
        tr("Regardez la caméra puis cliquez sur  « Capturer »"), &dlg);
    infoLbl->setAlignment(Qt::AlignCenter);
    root->addWidget(infoLbl);

    auto* btnRow   = new QHBoxLayout();
    auto* btnCap   = new QPushButton(tr("📸  Capturer"), &dlg);
    auto* btnClose = new QPushButton(tr("Annuler"), &dlg);
    btnCap->setEnabled(false);   // enabled once a face is visible
    btnRow->addStretch();
    btnRow->addWidget(btnCap);
    btnRow->addWidget(btnClose);
    root->addLayout(btnRow);

    QObject::connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::reject);

    // ── Open webcam ──────────────────────────────────────────────────────────
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        QMessageBox::critical(this, tr("Erreur"),
            tr("Impossible d'ouvrir la webcam (index 0)."));
        return;
    }

    // Shared state between timer lambda and capture button
    cv::Mat lastFrame;      // most recent raw frame
    bool faceVisible = false;

    auto* frameTimer = new QTimer(&dlg);
    QObject::connect(frameTimer, &QTimer::timeout, &dlg,
        [&, camLbl, infoLbl, btnCap]() {
            cv::Mat frame;
            cap >> frame;
            if (frame.empty()) return;

            m_faceDetector->setInputSize(frame.size());
            cv::Mat faces;
            m_faceDetector->detect(frame, faces);
            faceVisible = (faces.rows > 0);
            btnCap->setEnabled(faceVisible);

            // Draw bounding box
            for (int i = 0; i < faces.rows; ++i) {
                int fx = static_cast<int>(faces.at<float>(i, 0));
                int fy = static_cast<int>(faces.at<float>(i, 1));
                int fw = static_cast<int>(faces.at<float>(i, 2));
                int fh = static_cast<int>(faces.at<float>(i, 3));
                cv::rectangle(frame, cv::Rect(fx, fy, fw, fh),
                              cv::Scalar(0, 220, 80), 2);
            }
            infoLbl->setText(faceVisible
                ? tr("✔ Visage détecté — cliquez sur « Capturer »")
                : tr("Aucun visage détecté — repositionnez-vous"));

            // Show in label
            cv::Mat rgb;
            cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
            QImage img(rgb.data, rgb.cols, rgb.rows,
                       static_cast<int>(rgb.step), QImage::Format_RGB888);
            camLbl->setPixmap(
                QPixmap::fromImage(img.copy())
                    .scaled(camLbl->size(), Qt::KeepAspectRatio,
                            Qt::SmoothTransformation));

            lastFrame = frame.clone(); // keep most recent for capture
        }
    );
    frameTimer->start(30);

    // ── Capture button: encode from the current frame ────────────────────────
    QObject::connect(btnCap, &QPushButton::clicked, &dlg,
        [&]() {
            if (lastFrame.empty()) return;

            m_faceDetector->setInputSize(lastFrame.size());
            cv::Mat faces;
            m_faceDetector->detect(lastFrame, faces);
            if (faces.rows == 0) {
                QMessageBox::warning(&dlg, tr("Aucun visage"),
                    tr("Aucun visage n'a été détecté sur la frame courante.\n"
                       "Réessayez."));
                return;
            }

            cv::Mat aligned, embedding;
            m_faceRecognizer->alignCrop(lastFrame, faces.row(0), aligned);
            m_faceRecognizer->feature(aligned, embedding); // 1×128 float32

            QByteArray blob(
                reinterpret_cast<const char*>(embedding.data),
                static_cast<int>(embedding.total() * sizeof(float)));

            frameTimer->stop();
            cap.release();

            // Store result and close dialog
            m_capturedFaceBlob = blob;
            dlg.accept();
        }
    );

    dlg.exec();
    frameTimer->stop();
    if (cap.isOpened()) cap.release();

    // ── Update status label on the form ─────────────────────────────────────
    if (!m_capturedFaceBlob.isEmpty()) {
        ui->faceStatusLabel->setText(tr("✔ Visage capturé avec succès"));
        ui->faceStatusLabel->setStyleSheet("color: #2e7d32; font-weight: bold;");
    } else {
        ui->faceStatusLabel->setText(tr("Aucun visage capturé"));
        ui->faceStatusLabel->setStyleSheet("");
    }
}

void MainWindow::on_btnConsulterEmp_clicked()
{
    // Ensure we're on the personnel module first
    if (ui->modules->currentIndex() != 0)
        crossFadeToIndex(ui->modules, 0);
    // Load fresh data then switch to the consult page (index 1)
    loadEmployeeTable();
    crossFadeToIndex(ui->metierspersonnel, 1);
}

void MainWindow::on_btnStatEmp_clicked()
{
    // Ensure we're on the personnel module first
    if (ui->modules->currentIndex() != 0)
        crossFadeToIndex(ui->modules, 0);
    // Switch to "Statistics" page (index 2)
    crossFadeToIndex(ui->metierspersonnel, 2);
    loadEmployeeStats();
}

void MainWindow::on_btnAdvEmp_clicked()
{
    // Ensure we're on the personnel module first
    if (ui->modules->currentIndex() != 0)
        crossFadeToIndex(ui->modules, 0);
    // Switch to "Advanced" page (index 3) and refresh data
    crossFadeToIndex(ui->metierspersonnel, 3);
    populateAffCombos();
    loadAffectationTable();
    // Start on the table view
    ui->affStack->setCurrentIndex(1);
}

// ── Affectation helpers ──────────────────────────────────────────────────────
// Schema:
//   EMP_MACH  (ID_EMP, ID_SERIE)          — composite PK, only 2 columns
//   SERIE     (ID_SERIE, NOM_SERIE, ...)   — ID_SERIE is PK
//   MACHINE   (ID_MACHINE, NOM_MACHINE, ..., ID_SERIE)  — FK to SERIE

void MainWindow::populateAffCombos()
{
    // ── Employees ────────────────────────────────────────────────────────────
    ui->affEmpCombo->clear();
    QSqlQuery qEmp(
        "SELECT id_emp, nom_emp || ' ' || prenom_emp "
        "FROM   EMPLOYE "
        "ORDER BY nom_emp");
    while (qEmp.next())
        ui->affEmpCombo->addItem(qEmp.value(1).toString(), qEmp.value(0).toInt());

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
}

void MainWindow::loadAffectationTable()
{
    QTableWidget* t = ui->affTable;
    t->setRowCount(0);
    t->setSortingEnabled(false);

    // Columns: 0=ID_EMP  1=Employé  2=Série  3=Machine  4=Poste  5=Date début  6=Date fin  7=Actions
    QSqlQuery q(
        "SELECT em.id_emp, "
        "       e.nom_emp || ' ' || e.prenom_emp, "
        "       s.nom_serie, "
        "       em.id_serie, "
        "       NVL(m.nom_machine, '-'), "
        "       NVL(em.poste, '-'), "
        "       TO_CHAR(em.date_debut, 'DD/MM/YYYY'), "
        "       TO_CHAR(em.date_fin,   'DD/MM/YYYY') "
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

        // ── Action buttons ────────────────────────────────────────────────
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
        t->setCellWidget(row, 7, cell);

        // Edit: pre-fill form and switch to form page
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
            ui->affDateFinEdit->setDate(
                sfin.isEmpty() ? QDate::currentDate().addMonths(1)
                               : QDate::fromString(sfin, "dd/MM/yyyy"));

            m_editingAffIdEmp   = idEmp;
            m_editingAffIdSerie = idSerie;
            ui->affSaveBtn->setText(tr("Modifier"));
            ui->affStack->setCurrentIndex(0);
        });

        // Delete: confirm then remove from EMP_MACH
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
    t->setSortingEnabled(true);
}

void MainWindow::filterAffTable()
{
    QString search = ui->affSearchEdit->text().trimmed().toLower();

    for (int r = 0; r < ui->affTable->rowCount(); ++r) {
        auto* empItem   = ui->affTable->item(r, 1);  // Employé
        auto* serieItem = ui->affTable->item(r, 2);  // Série
        auto* machItem  = ui->affTable->item(r, 3);  // Machine
        auto* posteItem = ui->affTable->item(r, 4);  // Poste

        bool matchSearch = search.isEmpty()
            || (empItem   && empItem->text().toLower().contains(search))
            || (serieItem && serieItem->text().toLower().contains(search))
            || (machItem  && machItem->text().toLower().contains(search))
            || (posteItem && posteItem->text().toLower().contains(search));

        ui->affTable->setRowHidden(r, !matchSearch);
    }
}

// ── Affectation slots ────────────────────────────────────────────────────────

void MainWindow::on_affNewBtn_clicked()
{
    m_editingAffIdEmp   = -1;
    m_editingAffIdSerie = -1;
    ui->affSaveBtn->setText(tr("Affecter"));
    ui->affEmpCombo->setCurrentIndex(0);
    ui->affSerieCombo->setCurrentIndex(0);
    ui->affPosteCombo->setCurrentIndex(0);
    ui->affDateDebEdit->setDate(QDate::currentDate());
    ui->affDateFinEdit->setDate(QDate::currentDate().addMonths(1));
    ui->affStack->setCurrentIndex(0);
}

void MainWindow::on_affCancelBtn_clicked()
{
    m_editingAffIdEmp   = -1;
    m_editingAffIdSerie = -1;
    ui->affSaveBtn->setText(tr("Affecter"));
    ui->affStack->setCurrentIndex(1);
}

void MainWindow::on_affRefreshBtn_clicked()
{
    loadAffectationTable();
}

void MainWindow::on_affSearchEdit_textChanged(const QString&)
{
    filterAffTable();
}

void MainWindow::on_affSaveBtn_clicked()
{
    int     newEmpId   = ui->affEmpCombo->currentData().toInt();
    int     newSerieId = ui->affSerieCombo->currentData().toInt();
    QString poste      = ui->affPosteCombo->currentText();
    QDate   dateDeb    = ui->affDateDebEdit->date();
    QDate   dateFin    = ui->affDateFinEdit->date();

    if (newEmpId <= 0 || newSerieId <= 0) {
        QMessageBox::warning(this, tr("Champs requis"),
            tr("Veuillez sélectionner un employé et une série."));
        return;
    }
    if (dateFin < dateDeb) {
        QMessageBox::warning(this, tr("Dates invalides"),
            tr("La date de fin doit être postérieure à la date de début."));
        return;
    }

    QSqlQuery q;

    if (m_editingAffIdEmp > 0 && m_editingAffIdSerie > 0) {
        // ── EDIT mode ────────────────────────────────────────────────────────
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
            q.bindValue(":date_fin",   dateFin);
            q.bindValue(":id_emp",     m_editingAffIdEmp);
            q.bindValue(":id_serie",   m_editingAffIdSerie);
        } else {
            // PK changed — check duplicate first
            QSqlQuery qCheck;
            qCheck.prepare(
                "SELECT COUNT(*) FROM EMP_MACH "
                "WHERE id_emp = :id_emp AND id_serie = :id_serie");
            qCheck.bindValue(":id_emp",   newEmpId);
            qCheck.bindValue(":id_serie", newSerieId);
            qCheck.exec();
            if (qCheck.next() && qCheck.value(0).toInt() > 0) {
                QMessageBox::warning(this, tr("Doublon"),
                    tr("Cet employé est déjà affecté à cette série."));
                return;
            }
            // Delete old row
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
            q.prepare(
                "INSERT INTO EMP_MACH (id_emp, id_serie, poste, date_debut, date_fin) "
                "VALUES (:id_emp, :id_serie, :poste, :date_debut, :date_fin)");
            q.bindValue(":id_emp",     newEmpId);
            q.bindValue(":id_serie",   newSerieId);
            q.bindValue(":poste",      poste);
            q.bindValue(":date_debut", dateDeb);
            q.bindValue(":date_fin",   dateFin);
        }
    } else {
        // ── INSERT mode ───────────────────────────────────────────────────────
        QSqlQuery qCheck;
        qCheck.prepare(
            "SELECT COUNT(*) FROM EMP_MACH "
            "WHERE id_emp = :id_emp AND id_serie = :id_serie");
        qCheck.bindValue(":id_emp",   newEmpId);
        qCheck.bindValue(":id_serie", newSerieId);
        qCheck.exec();
        if (qCheck.next() && qCheck.value(0).toInt() > 0) {
            QMessageBox::warning(this, tr("Doublon"),
                tr("Cet employé est déjà affecté à cette série."));
            return;
        }
        q.prepare(
            "INSERT INTO EMP_MACH (id_emp, id_serie, poste, date_debut, date_fin) "
            "VALUES (:id_emp, :id_serie, :poste, :date_debut, :date_fin)");
        q.bindValue(":id_emp",     newEmpId);
        q.bindValue(":id_serie",   newSerieId);
        q.bindValue(":poste",      poste);
        q.bindValue(":date_debut", dateDeb);
        q.bindValue(":date_fin",   dateFin);
    }

    if (!q.exec()) {
        QMessageBox::critical(this, tr("Erreur"),
            tr("Impossible d'enregistrer l'affectation :\n%1").arg(q.lastError().text()));
        return;
    }

    bool wasEdit = (m_editingAffIdEmp > 0);
    m_editingAffIdEmp   = -1;
    m_editingAffIdSerie = -1;
    ui->affSaveBtn->setText(tr("Affecter"));
    loadAffectationTable();
    ui->affStack->setCurrentIndex(1);
    QMessageBox::information(this, tr("Succès"),
        wasEdit ? tr("Affectation modifiée avec succès.")
                : tr("Affectation enregistrée avec succès."));
}


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

// Module 5 (Machines) toolbar actions
void MainWindow::on_btnConsulterMachines_clicked()
{
    if (ui->modules->currentIndex() != 3)
        ui->modules->setCurrentIndex(3);
    crossFadeToIndex(ui->metierspersonnel_2, 1); // consulterpersonnel_3
}

void MainWindow::on_btnAjouterMachines_clicked()
{
    if (ui->modules->currentIndex() != 3)
        ui->modules->setCurrentIndex(3);
    crossFadeToIndex(ui->metierspersonnel_2, 0); // ajoutpersonnel_3
}

void MainWindow::on_btnStatMachines_clicked()
{
    if (ui->modules->currentIndex() != 3)
        ui->modules->setCurrentIndex(3);
    crossFadeToIndex(ui->metierspersonnel_2, 2); // statPersonnel_3
}

void MainWindow::on_btnAvanceMachines_clicked()
{
    if (ui->modules->currentIndex() != 3)
        ui->modules->setCurrentIndex(3);
    crossFadeToIndex(ui->metierspersonnel_2, 3); // metieravancee_4
}

// Module 6 (Agriculteurs) toolbar actions
void MainWindow::on_btnConsulterAgr_clicked()
{
    if (ui->modules->currentIndex() != 4)
        ui->modules->setCurrentIndex(4);
    crossFadeToIndex(ui->metiersagriculteurs, 1); // consulteragriculteur
}

void MainWindow::on_btnAjouterAgr_clicked()
{
    if (ui->modules->currentIndex() != 4)
        ui->modules->setCurrentIndex(4);
    crossFadeToIndex(ui->metiersagriculteurs, 0); // ajoutagriculteur
}

void MainWindow::on_btnStatAgr_clicked()
{
    if (ui->modules->currentIndex() != 4)
        ui->modules->setCurrentIndex(4);
    crossFadeToIndex(ui->metiersagriculteurs, 2); // statAGriculteur
}

void MainWindow::on_btnAvanceAgr_clicked()
{
    if (ui->modules->currentIndex() != 4)
        ui->modules->setCurrentIndex(4);
    crossFadeToIndex(ui->metiersagriculteurs, 3); // metieravancee_5
}

// Sidebar navigation: map buttons to modules indices
// Order in UI: module1 (0), module3 (1), module4 (2), module5 (3), module6 (4), module2 (5)
void MainWindow::on_btnmod1_clicked()
{
    ui->MainStacked->setCurrentIndex(1); // ensure mainprogram
    crossFadeToIndex(ui->modules, 0);
    // Always reset to the first page in the module
    crossFadeToIndex(ui->metierspersonnel, 0);
    setActiveModuleButton(0);
}

void MainWindow::on_btnmod2_clicked()
{
    ui->MainStacked->setCurrentIndex(1);
    crossFadeToIndex(ui->modules, 5);
    crossFadeToIndex(ui->metiersstocks, 0);
    setActiveModuleButton(5);
}

void MainWindow::on_btnmod3_clicked()
{
    ui->MainStacked->setCurrentIndex(1);
    crossFadeToIndex(ui->modules, 1);
    crossFadeToIndex(ui->metiersCiternes, 0);
    setActiveModuleButton(1);
}

void MainWindow::on_btnmod4_clicked()
{
    ui->MainStacked->setCurrentIndex(1);
    crossFadeToIndex(ui->modules, 2);
    crossFadeToIndex(ui->metiersqualite, 0);
    setActiveModuleButton(2);
}

void MainWindow::on_btnmod5_clicked()
{
    ui->MainStacked->setCurrentIndex(1);
    crossFadeToIndex(ui->modules, 3);
    crossFadeToIndex(ui->metierspersonnel_2, 0);
    setActiveModuleButton(3);
}

void MainWindow::on_btnmod6_clicked()
{
    ui->MainStacked->setCurrentIndex(1);
    crossFadeToIndex(ui->modules, 4);
    crossFadeToIndex(ui->metiersagriculteurs, 0);
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

// Smooth transition using overlay snapshots to avoid flicker
void MainWindow::crossFadeToIndex(QStackedWidget* stack, int newIndex)
{
    if (!stack || newIndex < 0 || newIndex >= stack->count()) {
        return;
    }

    // Always re-enable the stack first to clear any stale disabled state
    // left over from a previous animation that may not have cleaned up properly.
    stack->setEnabled(true);

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
    stack->setEnabled(false); // temporarily block input during transition

    // Safety timer: unconditionally re-enable the stack after the animation
    // duration (+ a small buffer) so a failed/skipped animation never leaves
    // the stack permanently disabled and unresponsive to user input.
    QTimer::singleShot(400, stack, [stack]() {
        stack->setEnabled(true);
    });

    // Parallel fade animations
    auto* outAnim = new QPropertyAnimation(currEff, "opacity", currentOverlay);
    outAnim->setDuration(220);
    outAnim->setStartValue(1.0);
    outAnim->setEndValue(0.0);
    outAnim->setEasingCurve(QEasingCurve::OutCubic);

    auto* inAnim = new QPropertyAnimation(nextEff, "opacity", nextOverlay);
    inAnim->setDuration(220);
    inAnim->setStartValue(0.0);
    inAnim->setEndValue(1.0);
    inAnim->setEasingCurve(QEasingCurve::OutCubic);

    auto* group = new QParallelAnimationGroup(stack);
    group->addAnimation(outAnim);
    group->addAnimation(inAnim);
    QObject::connect(group, &QParallelAnimationGroup::finished, this, [stack, currentOverlay, nextOverlay]() {
        stack->setEnabled(true);
        // Clean up overlays
        currentOverlay->deleteLater();
        nextOverlay->deleteLater();
    });
    group->start(QAbstractAnimation::DeleteWhenStopped);
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
    if (ui->lineEdit && ui->comboBox && ui->tableEmp) {
        QObject::connect(ui->lineEdit, &QLineEdit::textChanged, this, [this](const QString&){ filterPersonnelTable(); });
        QObject::connect(ui->comboBox, &QComboBox::currentTextChanged, this, [this](const QString&){ filterPersonnelTable(); });
    }
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

void MainWindow::loadEmployeeStats()
{
    QWidget* container = ui->chartStatusContainer;

    // ── clear any previous charts ─────────────────────────────────────────
    QLayout* oldLayout = container->layout();
    if (oldLayout) {
        QLayoutItem* item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        delete oldLayout;
    }

    // ── query: COUNT per role ─────────────────────────────────────────────
    QSqlQuery q;
    q.prepare("SELECT role, COUNT(*) AS nb FROM EMPLOYE GROUP BY role ORDER BY role");
    if (!q.exec()) {
        QVBoxLayout* lay = new QVBoxLayout(container);
        QLabel* err = new QLabel("Erreur DB: " + q.lastError().text(), container);
        err->setAlignment(Qt::AlignCenter);
        err->setStyleSheet("color: red; font-size: 12pt;");
        lay->addWidget(err);
        return;
    }

    QMap<QString, int> roleCount;
    int total = 0;
    while (q.next()) {
        roleCount[q.value(0).toString()] = q.value(1).toInt();
        total += q.value(1).toInt();
    }

    if (total == 0) {
        QVBoxLayout* lay = new QVBoxLayout(container);
        QLabel* empty = new QLabel("Aucun employé dans la base de données.", container);
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet("font-size: 12pt; color: gray;");
        lay->addWidget(empty);
        return;
    }

    // ── title label ───────────────────────────────────────────────────────
    QLabel* title = new QLabel(
        QString("Statistiques des employés  —  %1 employé(s) au total").arg(total));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 14pt; font-weight: bold; padding: 8px;");

    // ── fixed colours per role ────────────────────────────────────────────
    QMap<QString, QColor> roleColors;
    roleColors["Administrateur"] = QColor("#2E5265");
    roleColors["Manager"]        = QColor("#4A90D9");
    roleColors["Technicien"]     = QColor("#50C878");
    roleColors["Operateur"]      = QColor("#F4A460");

    // ══ CHART 1: Pie — répartition par rôle ══════════════════════════════
    QPieSeries* pieSeries = new QPieSeries();
    for (auto it = roleCount.constBegin(); it != roleCount.constEnd(); ++it) {
        double pct = 100.0 * it.value() / total;
        QPieSlice* slice = pieSeries->append(
            QString("%1\n%2 (%3%)").arg(it.key()).arg(it.value())
                .arg(QString::number(pct, 'f', 1)),
            it.value());
        if (roleColors.contains(it.key()))
            slice->setColor(roleColors[it.key()]);
        slice->setLabelVisible(true);
        slice->setLabelColor(Qt::black);
    }
    QChart* pieChart = new QChart();
    pieChart->addSeries(pieSeries);
    pieChart->setTitle("Répartition par rôle");
    pieChart->setTitleFont(QFont("Segoe UI", 11, QFont::Bold));
    pieChart->legend()->setAlignment(Qt::AlignRight);
    pieChart->setAnimationOptions(QChart::AllAnimations);
    pieChart->setBackgroundVisible(false);
    QChartView* pieView = new QChartView(pieChart);
    pieView->setRenderHint(QPainter::Antialiasing);
    pieView->setMinimumHeight(280);
    pieView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

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

    QChart* barChart = new QChart();
    barChart->addSeries(barSeries);
    barChart->setTitle("Nombre d'employés par rôle");
    barChart->setTitleFont(QFont("Segoe UI", 11, QFont::Bold));
    barChart->addAxis(axisX, Qt::AlignBottom);
    barChart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisX);
    barSeries->attachAxis(axisY);
    barChart->legend()->setVisible(false);
    barChart->setAnimationOptions(QChart::SeriesAnimations);
    barChart->setBackgroundVisible(false);

    QChartView* barView = new QChartView(barChart);
    barView->setRenderHint(QPainter::Antialiasing);
    barView->setMinimumHeight(280);
    barView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // ── assemble layout ───────────────────────────────────────────────────
    QVBoxLayout* mainLay = new QVBoxLayout(container);
    mainLay->setContentsMargins(10, 10, 10, 10);
    mainLay->setSpacing(16);
    mainLay->addWidget(title);

    QHBoxLayout* chartsRow = new QHBoxLayout();
    chartsRow->setSpacing(12);
    chartsRow->addWidget(pieView, 1);
    chartsRow->addWidget(barView, 1);
    mainLay->addLayout(chartsRow, 1);
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
        // Keep charts comfortably below any top controls without large gaps
        grid2->setContentsMargins(0, 12, 0, 0);
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
        // Leave minimal room for any top controls without introducing large top padding
        grid4->setContentsMargins(0, 12, 0, 0);
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
        table->setColumnWidth(dataCols, 72);
    }
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    if (table->verticalHeader())
        table->verticalHeader()->setDefaultSectionSize(30);
}

void MainWindow::setupPersonnelTable()
{
    // Called once at startup – just prepare layout; data loaded on demand
    if (!ui->tableEmp) return;
    ui->tableEmp->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableEmp->setSelectionMode(QAbstractItemView::SingleSelection);
    if (ui->tableEmp->verticalHeader())
        ui->tableEmp->verticalHeader()->setDefaultSectionSize(30);
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
    h->setContentsMargins(2, 1, 2, 1);
    h->setSpacing(4);
    h->setAlignment(Qt::AlignCenter);
    container->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    QPushButton* btnModify = new QPushButton(QString(), container);
    btnModify->setProperty("type", "warning");
    btnModify->setObjectName("modifyBtn");
    btnModify->setFocusPolicy(Qt::NoFocus);
    btnModify->setToolTip(tr("Modifier"));
    btnModify->setIcon(QIcon(QStringLiteral(":/img/edit.svg")));
    btnModify->setIconSize(QSize(16, 16));
    btnModify->setFixedHeight(24);
    btnModify->setCursor(Qt::PointingHandCursor);

    QPushButton* btnDelete = new QPushButton(QString(), container);
    btnDelete->setProperty("type", "danger");
    btnDelete->setObjectName("deleteBtn");
    btnDelete->setFocusPolicy(Qt::NoFocus);
    btnDelete->setToolTip(tr("Supprimer"));
    btnDelete->setIcon(QIcon(QStringLiteral(":/img/delete.svg")));
    btnDelete->setIconSize(QSize(16, 16));
    btnDelete->setFixedHeight(24);
    btnDelete->setCursor(Qt::PointingHandCursor);

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
        table->setColumnWidth(last, 68);
    }
}

void MainWindow::on_faceBtn_clicked()
{
    if (!m_faceDetector || !m_faceRecognizer) {
        QMessageBox::warning(this, tr("Indisponible"),
            tr("Les modèles de reconnaissance faciale n'ont pas pu être chargés.\n"
               "Vérifiez que les fichiers .onnx sont présents à côté de l'exécutable."));
        return;
    }

    // Reload embeddings fresh from DB every time
    loadFaceEmbeddings();
    if (m_faceEmbeddings.isEmpty()) {
        QMessageBox::information(this, tr("Aucun visage enregistré"),
            tr("Aucun employé n'a de visage enregistré.\n"
               "Ajoutez une photo lors de la création d'un employé."));
        return;
    }

    // ── Build dialog ────────────────────────────────────────────────────────
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

    auto* statusLbl = new QLabel(tr("Recherche d'un visage…"), &dlg);
    statusLbl->setAlignment(Qt::AlignCenter);
    root->addWidget(statusLbl);

    auto* btnCancel = new QPushButton(tr("Annuler"), &dlg);
    root->addWidget(btnCancel);
    QObject::connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);

    // ── Open webcam ─────────────────────────────────────────────────────────
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        QMessageBox::critical(this, tr("Erreur"),
            tr("Impossible d'ouvrir la webcam (index 0)."));
        return;
    }

    int matchedId = -1;

    auto* frameTimer = new QTimer(&dlg);
    QObject::connect(frameTimer, &QTimer::timeout, &dlg,
        [&, camLbl, statusLbl]() {
            cv::Mat frame;
            cap >> frame;
            if (frame.empty()) return;

            // ── Detect ──────────────────────────────────────────────────────
            m_faceDetector->setInputSize(frame.size());
            cv::Mat faces;
            m_faceDetector->detect(frame, faces);

            for (int i = 0; i < faces.rows; ++i) {
                // ── Align + embed ────────────────────────────────────────────
                cv::Mat aligned, embedding;
                m_faceRecognizer->alignCrop(frame, faces.row(i), aligned);
                m_faceRecognizer->feature(aligned, embedding);

                int id = matchFaceEmbedding(embedding);
                if (id > 0) {
                    // Draw green rect + name
                    int fx = static_cast<int>(faces.at<float>(i, 0));
                    int fy = static_cast<int>(faces.at<float>(i, 1));
                    int fw = static_cast<int>(faces.at<float>(i, 2));
                    int fh = static_cast<int>(faces.at<float>(i, 3));
                    cv::rectangle(frame, cv::Rect(fx, fy, fw, fh),
                                  cv::Scalar(0, 220, 0), 3);
                    matchedId = id;
                    frameTimer->stop();
                    cap.release();
                    dlg.accept();
                    return;
                }

                // Draw yellow rect for unrecognised face
                int fx = static_cast<int>(faces.at<float>(i, 0));
                int fy = static_cast<int>(faces.at<float>(i, 1));
                int fw = static_cast<int>(faces.at<float>(i, 2));
                int fh = static_cast<int>(faces.at<float>(i, 3));
                cv::rectangle(frame, cv::Rect(fx, fy, fw, fh),
                              cv::Scalar(0, 200, 220), 2);
            }

            // ── Display in QLabel ────────────────────────────────────────────
            cv::Mat rgb;
            cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
            QImage qimg(rgb.data, rgb.cols, rgb.rows,
                        static_cast<int>(rgb.step),
                        QImage::Format_RGB888);
            camLbl->setPixmap(
                QPixmap::fromImage(qimg.copy())
                    .scaled(camLbl->size(), Qt::KeepAspectRatio,
                            Qt::SmoothTransformation));
        }
    );
    frameTimer->start(30); // ~33 fps

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
    if (!q.exec("SELECT id_emp, modele_faciale FROM EMPLOYE "
                "WHERE modele_faciale IS NOT NULL")) {
        qWarning() << "[FaceRecog] loadFaceEmbeddings query failed:"
                   << q.lastError().text();
        return;
    }

    constexpr int expectedBytes = 128 * static_cast<int>(sizeof(float));
    while (q.next()) {
        int id           = q.value(0).toInt();
        QByteArray blob  = q.value(1).toByteArray();
        if (blob.size() != expectedBytes) {
            qWarning() << "[FaceRecog] Skipping id" << id
                       << "— blob size" << blob.size()
                       << "(expected" << expectedBytes << ")";
            continue;
        }
        cv::Mat embedding(1, 128, CV_32F);
        std::memcpy(embedding.data, blob.constData(), blob.size());
        m_faceEmbeddings.insert(id, embedding);
    }
    qDebug() << "[FaceRecog] Loaded" << m_faceEmbeddings.size()
             << "face embedding(s) from DB";
}

int MainWindow::matchFaceEmbedding(const cv::Mat& embedding)
{
    if (!m_faceRecognizer || m_faceEmbeddings.isEmpty()) return -1;

    for (auto it = m_faceEmbeddings.cbegin(); it != m_faceEmbeddings.cend(); ++it) {
        double score = m_faceRecognizer->match(
            embedding, it.value(), cv::FaceRecognizerSF::FR_COSINE);
        qDebug() << "[FaceRecog] id" << it.key() << "cosine score:" << score;
        if (score > 0.363) // official SFace cosine threshold
            return it.key();
    }
    return -1;
}

void MainWindow::on_exportEmpBtn_clicked()
{
    qDebug() << "[exportEmpBtn] slot fired";

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

void MainWindow::on_toolButton_clicked()
{
    // Reserved for future use (toolButton widget in UI)
}
