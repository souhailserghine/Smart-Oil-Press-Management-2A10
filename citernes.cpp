#include "citernes.h"
#include "ui_citernes.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QTextDocument>
#include <QDateTime>
#include <QHeaderView>
#include <QApplication>
#include <QLabel>
#include <QFormLayout>
#include <QPrinter>
#include <QToolButton>
#include <QStatusBar>
#include <QPainter>
#include <QColor>
#include <QPalette>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTimer>
#include <cmath>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QProgressBar>
#include <QSerialPortInfo>

#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QHorizontalStackedBarSeries>

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS VALIDATION
// ─────────────────────────────────────────────────────────────────────────────

static void setErreur(QLineEdit* field, QLabel* label, const QString& msg)
{
    field->setStyleSheet(
        "border:2px solid #F44336; border-radius:4px; padding:2px; background:#fff8f8;");
    field->setToolTip("⚠ " + msg);
    if (label) {
        label->setText("⚠ " + msg);
        label->setStyleSheet("color:#F44336; font-size:10px; font-style:italic;");
        label->setVisible(true);
    }
}
static void setOk(QLineEdit* field, QLabel* label)
{
    field->setStyleSheet(
        "border:2px solid #4CAF50; border-radius:4px; padding:2px; background:#f8fff8;");
    field->setToolTip("");
    if (label) { label->setText(""); label->setVisible(false); }
}
static void setNeutre(QLineEdit* field, QLabel* label)
{
    field->setStyleSheet(""); field->setToolTip("");
    if (label) { label->setText(""); label->setVisible(false); }
}
static void setErreurCombo(QComboBox* combo, QLabel* label, const QString& msg)
{
    combo->setStyleSheet(
        "QComboBox { border:2px solid #F44336; border-radius:4px;"
        " padding:2px; background:#fff8f8; }");
    combo->setToolTip("⚠ " + msg);
    if (label) {
        label->setText("⚠ " + msg);
        label->setStyleSheet("color:#F44336; font-size:10px; font-style:italic;");
        label->setVisible(true);
    }
}
static void setOkCombo(QComboBox* combo, QLabel* label)
{
    combo->setStyleSheet(
        "QComboBox { border:2px solid #4CAF50; border-radius:4px; padding:2px; }");
    combo->setToolTip("");
    if (label) { label->setText(""); label->setVisible(false); }
}
static void setNeutreCombo(QComboBox* combo, QLabel* label)
{
    combo->setStyleSheet(""); combo->setToolTip("");
    if (label) { label->setText(""); label->setVisible(false); }
}

// ─────────────────────────────────────────────────────────────────────────────
// SEUILS TEMPÉRATURE
// ─────────────────────────────────────────────────────────────────────────────

struct Seuils { double minOk, maxOk, danger; };
static Seuils getSeuilsTemp(const QString& type)
{
    static const QMap<QString, Seuils> s = {
        {"Olive",        {15.0, 25.0, 40.0}},
        {"Raffinée",     {10.0, 30.0, 60.0}},
        {"Bio",          {12.0, 22.0, 35.0}},
        {"Alimentaire",  {10.0, 28.0, 50.0}},
        {"Industrielle", { 5.0, 50.0, 80.0}}
    };
    return s.value(type, {10.0, 30.0, 55.0});
}

// ─────────────────────────────────────────────────────────────────────────────
// CONSTRUCTEUR
// ─────────────────────────────────────────────────────────────────────────────

Citernes::Citernes(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Citernes)
{
    ui->setupUi(this);

    {
        QSqlQuery q;
        q.exec(R"(
            BEGIN
                EXECUTE IMMEDIATE 'CREATE TABLE BESOIN_ACHAT (
                    id NUMBER PRIMARY KEY,
                    typeHuile VARCHAR2(100),
                    quantiteManquante NUMBER,
                    capaciteSuggeree NUMBER,
                    nombreCiternesRequises NUMBER,
                    statut VARCHAR2(50),
                    dateAjout VARCHAR2(50)
                )';
            EXCEPTION
                WHEN OTHERS THEN
                    IF SQLCODE != -955 THEN RAISE; END IF;
            END;
        )");
        q.exec(R"(
            BEGIN
                EXECUTE IMMEDIATE 'CREATE SEQUENCE BESOIN_ACHAT_SEQ
                    START WITH 1 INCREMENT BY 1';
            EXCEPTION
                WHEN OTHERS THEN
                    IF SQLCODE != -955 THEN RAISE; END IF;
            END;
        )");
        creerSequenceMesure();
    }

    chargerBesoinsAchat();
    statusBar()->showMessage("Prêt");

    connect(ui->Trier, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ rafraichirListe(); });
    connect(ui->SaiRecherche, &QLineEdit::textChanged,
            this, &Citernes::on_Recherche_clicked);
    connect(ui->exporterListeCiterne, &QPushButton::clicked,
            this, &Citernes::on_exporterListeCiterne_clicked);
    connect(ui->StatistiqueCiterne, &QToolButton::clicked,
            this, &Citernes::on_StatistiqueCiterne_clicked);
    ui->X->setEnabled(true); ui->X->setVisible(true);
    connect(ui->X, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Citernes::on_X_currentIndexChanged);
    if (ui->acheter)
        connect(ui->acheter, &QCommandLinkButton::clicked,
                this, &Citernes::on_acheter_clicked);
    if (ui->remplissageCi)
        connect(ui->remplissageCi, &QCommandLinkButton::clicked,
                this, &Citernes::on_remplissageCi_clicked);

    // ── Bloc Arduino ──────────────────────────────────────────────────────
    {
        QWidget* arduinoWidget = new QWidget(this);
        arduinoWidget->setStyleSheet(
            "background:#E3F2FD; border-radius:8px; border:1px solid #90CAF9;");
        arduinoWidget->setMinimumHeight(200);

        QVBoxLayout* vlay = new QVBoxLayout(arduinoWidget);
        vlay->setContentsMargins(14, 12, 14, 12);
        vlay->setSpacing(10);

        QLabel* titre = new QLabel("🔌  Connexion Arduino — Anti-débordement", arduinoWidget);
        titre->setStyleSheet(
            "font-weight:bold; font-size:13px; color:#1565C0;"
            "background:transparent; border:none;");
        vlay->addWidget(titre);

        QHBoxLayout* hPorts = new QHBoxLayout();
        QLabel* lblPort = new QLabel("Port série :", arduinoWidget);
        lblPort->setStyleSheet("background:transparent; border:none; font-size:12px;");
        lblPort->setFixedWidth(80);

        m_comboPorts = new QComboBox(arduinoWidget);
        m_comboPorts->setMinimumWidth(120);
        m_comboPorts->setStyleSheet(
            "QComboBox { border:1px solid #90CAF9; border-radius:4px;"
            " padding:4px; background:white; font-size:12px; }"
            "QComboBox::drop-down { border:none; }");

        QPushButton* btnRefresh = new QPushButton("↺", arduinoWidget);
        btnRefresh->setToolTip("Actualiser la liste des ports");
        btnRefresh->setFixedSize(32, 32);
        btnRefresh->setStyleSheet(
            "background:#1565C0; color:white; border-radius:4px;"
            " font-size:14px; font-weight:bold;");
        connect(btnRefresh, &QPushButton::clicked,
                this, &Citernes::rafraichirListePorts);

        hPorts->addWidget(lblPort);
        hPorts->addWidget(m_comboPorts, 1);
        hPorts->addWidget(btnRefresh);
        vlay->addLayout(hPorts);

        m_labelStatut = new QLabel("⚪  Non connecté", arduinoWidget);
        m_labelStatut->setStyleSheet(
            "color:#607D8B; font-size:11px; background:transparent; border:none;");
        vlay->addWidget(m_labelStatut);

        m_btnArduino = new QPushButton("🔌  Connecter Arduino", arduinoWidget);
        m_btnArduino->setMinimumHeight(38);
        m_btnArduino->setStyleSheet(
            "background:#2196F3; color:white; border-radius:6px;"
            " padding:8px 14px; font-weight:bold; font-size:12px; border:none;");
        connect(m_btnArduino, &QPushButton::clicked,
                this, &Citernes::on_btnConnecterArduino_clicked);
        vlay->addWidget(m_btnArduino);

        QLabel* lblInfo = new QLabel(
            "ℹ️  Une mesure est enregistrée dans MESURE une seule fois par détection "
            "(front montant sec→eau). L'alerte débordement s'affiche une seule fois "
            "par événement ≥ 90%.",
            arduinoWidget);
        lblInfo->setWordWrap(true);
        lblInfo->setStyleSheet(
            "color:#1565C0; font-size:10px; font-style:italic;"
            " background:transparent; border:none;");
        vlay->addWidget(lblInfo);

        if (ui->AvCiterne) {
            QLayout* existing = ui->AvCiterne->layout();
            if (existing) {
                existing->addWidget(arduinoWidget);
            } else {
                QVBoxLayout* newLay = new QVBoxLayout(ui->AvCiterne);
                newLay->setAlignment(Qt::AlignTop);
                newLay->addWidget(arduinoWidget);
            }
        }
        rafraichirListePorts();
    }

    // ── Validation saisie ─────────────────────────────────────────────────
    QRegularExpression reDecimal("^\\d{0,10}(\\.\\d{0,4})?$");
    auto* validatorDecimal = new QRegularExpressionValidator(reDecimal, this);
    ui->SaiCapa->setValidator(validatorDecimal);
    ui->SaiNiv->setValidator(validatorDecimal);
    ui->SaiCapa->setPlaceholderText("Ex : 5000.00");
    ui->SaiNiv->setPlaceholderText("Ex : 2500.00");

    m_errCapa = new QLabel("", ui->SaiCapa->parentWidget());
    m_errNiv  = new QLabel("", ui->SaiNiv->parentWidget());
    m_errEtat = new QLabel("", ui->SaiEtat->parentWidget());
    m_errCapa->setVisible(false);
    m_errNiv->setVisible(false);
    m_errEtat->setVisible(false);

    QFormLayout* fl = qobject_cast<QFormLayout*>(ui->SaiCapa->parentWidget()->layout());
    if (fl) {
        fl->insertRow(1, "", m_errCapa);
        fl->insertRow(3, "", m_errNiv);
        fl->addRow("",   m_errEtat);
    }

    connect(ui->SaiCapa, &QLineEdit::textChanged, this, [this](const QString& txt) {
        if (txt.isEmpty()) { setNeutre(ui->SaiCapa, m_errCapa); return; }
        bool ok = false; double val = txt.toDouble(&ok);
        if (!ok || val <= 0)
            setErreur(ui->SaiCapa, m_errCapa, "La capacité doit être un nombre positif");
        else setOk(ui->SaiCapa, m_errCapa);
        if (!ui->SaiNiv->text().isEmpty())
            emit ui->SaiNiv->textChanged(ui->SaiNiv->text());
    });

    connect(ui->SaiNiv, &QLineEdit::textChanged, this, [this](const QString& txt) {
        if (txt.isEmpty()) { setNeutre(ui->SaiNiv, m_errNiv); return; }
        bool okN = false, okC = false;
        double niv = txt.toDouble(&okN), cap = ui->SaiCapa->text().toDouble(&okC);
        if (!okN || niv < 0)
            setErreur(ui->SaiNiv, m_errNiv, "Le niveau doit être un nombre positif ou nul");
        else if (okC && cap > 0 && niv > cap)
            setErreur(ui->SaiNiv, m_errNiv,
                      QString("Le niveau ne peut pas dépasser la capacité (%1 L)")
                          .arg(QString::number(cap,'f',0)));
        else setOk(ui->SaiNiv, m_errNiv);
    });

    connect(ui->SaiNiv, &QLineEdit::textChanged, this, [this](const QString& txt) {
        bool okN = false, okC = false;
        double niv = txt.toDouble(&okN), cap = ui->SaiCapa->text().toDouble(&okC);
        if (!okN || !okC || cap <= 0) return;
        double pct = niv / cap * 100.0;
        if      (pct >= 100.0) ui->SaiEtat->setCurrentText("Pleine");
        else if (pct == 0.0)   ui->SaiEtat->setCurrentText("Vide");
        else if (pct < 50.0)   ui->SaiEtat->setCurrentText("En remplissage");
        else                    ui->SaiEtat->setCurrentText("En utilisation");
    });

    auto validerEtat = [this]() {
        bool okN = false, okC = false;
        double niv = ui->SaiNiv->text().toDouble(&okN);
        double cap = ui->SaiCapa->text().toDouble(&okC);
        if (!okN || !okC || cap <= 0 ||
            ui->SaiNiv->text().isEmpty() || ui->SaiCapa->text().isEmpty()) {
            setNeutreCombo(ui->SaiEtat, m_errEtat); return;
        }
        double pct = niv / cap * 100.0;
        QString etat = ui->SaiEtat->currentText(), erreur;
        if (etat == "Pleine" && pct < 100.0)
            erreur = QString("\"Pleine\" nécessite niveau = capacité (actuellement %1%)")
                         .arg(QString::number(pct,'f',1));
        else if (etat == "Vide" && pct > 0.0)
            erreur = QString("\"Vide\" nécessite niveau = 0 (actuellement %1%)")
                         .arg(QString::number(pct,'f',1));
        if (!erreur.isEmpty()) setErreurCombo(ui->SaiEtat, m_errEtat, erreur);
        else                   setOkCombo(ui->SaiEtat, m_errEtat);
    };

    connect(ui->SaiEtat, &QComboBox::currentTextChanged,
            this, [validerEtat](const QString&){ validerEtat(); });
    connect(ui->SaiNiv,  &QLineEdit::textChanged,
            this, [validerEtat](const QString&){ validerEtat(); });
    connect(ui->SaiCapa, &QLineEdit::textChanged,
            this, [validerEtat](const QString&){ validerEtat(); });

    chargerListeCiternes();

    ui->ListeCiterne->horizontalHeader()->setSectionsClickable(true);
    connect(ui->ListeCiterne->horizontalHeader(), &QHeaderView::sectionDoubleClicked,
            this, [this](int col) {
                const QStringList sqlCol = {
                    "ID_CITERNE","CAPACITEMAX","NIVEAUACTUEL",
                    "TYPEHUILE","TEMPERATURE_CITERNE","ETAT_CITERNE",""
                };
                if (col < 0 || col >= sqlCol.size() || sqlCol[col].isEmpty()) return;
                QSqlQuery query;
                if (!query.exec(
                        QString("SELECT ID_CITERNE,CAPACITEMAX,NIVEAUACTUEL,TYPEHUILE,"
                                "TEMPERATURE_CITERNE,ETAT_CITERNE FROM CITERNE ORDER BY %1 ASC")
                            .arg(sqlCol[col]))) {
                    QMessageBox::critical(this,"Erreur SQL",query.lastError().text()); return;
                }
                ui->ListeCiterne->clearContents();
                ui->ListeCiterne->setRowCount(0);
                int row = 0;
                while (query.next()) {
                    ui->ListeCiterne->insertRow(row);
                    for (int c = 0; c < 6; ++c) {
                        auto* item = new QTableWidgetItem(query.value(c).toString());
                        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                        ui->ListeCiterne->setItem(row, c, item);
                    }
                    QWidget* cw  = new QWidget(ui->ListeCiterne);
                    auto*    lay = new QHBoxLayout(cw);
                    lay->setContentsMargins(2,2,2,2); lay->setSpacing(6);
                    auto* bm = new QPushButton("Modifier",  cw);
                    bm->setStyleSheet("background:#2196F3;color:white;border-radius:4px;padding:5px;");
                    bm->setFixedSize(70,30);
                    auto* bs = new QPushButton("Supprimer", cw);
                    bs->setStyleSheet("background:#F44336;color:white;border-radius:4px;padding:5px;");
                    bs->setFixedSize(70,30);
                    lay->addWidget(bm); lay->addWidget(bs);
                    lay->setAlignment(Qt::AlignCenter);
                    ui->ListeCiterne->setCellWidget(row, 6, cw);
                    connect(bm, &QPushButton::clicked, this, [this,row]{ onModifier(row); });
                    connect(bs, &QPushButton::clicked, this, [this,row]{ onSupprimer(row); });
                    ui->ListeCiterne->setRowHeight(row, 50);
                    ++row;
                }
            });

    m_timerNiveaux = new QTimer(this);
}

Citernes::~Citernes()
{
    if (m_timerNiveaux) m_timerNiveaux->stop();
    if (m_arduino) { m_arduino->close_arduino(); delete m_arduino; }
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
// BASE DE DONNÉES — TABLE MESURE
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::creerSequenceMesure()
{
    // SEQ_MESURE existe déjà dans le schéma Oracle — rien à créer
    QSqlQuery q;
    q.exec("SELECT SEQ_MESURE.NEXTVAL FROM DUAL");
    if (q.lastError().isValid())
        qDebug() << "[MESURE] SEQ_MESURE inaccessible :" << q.lastError().text();
    else
        qDebug() << "[MESURE] SEQ_MESURE OK";
}

int Citernes::getIdCiterneArduino()
{
    if (m_idCiterneArduino > 0) return m_idCiterneArduino;
    QSqlQuery q;
    q.exec("SELECT MIN(ID_CITERNE) FROM CITERNE");
    if (q.next() && !q.value(0).isNull()) {
        m_idCiterneArduino = q.value(0).toInt();
        qDebug() << "[MESURE] ID citerne Arduino résolu :" << m_idCiterneArduino;
    }
    return m_idCiterneArduino;
}

void Citernes::sauvegarderMesure(int idCiterne, double niveauActuel, double temperature)
{
    if (idCiterne <= 0) {
        qDebug() << "[MESURE] ID citerne invalide, mesure ignorée.";
        return;
    }
    QSqlQuery chk;
    chk.prepare("SELECT COUNT(*) FROM CITERNE WHERE ID_CITERNE = :id");
    chk.bindValue(":id", idCiterne);
    if (!chk.exec() || !chk.next() || chk.value(0).toInt() == 0) {
        qDebug() << "[MESURE] Citerne introuvable, mesure ignorée.";
        return;
    }
    QSqlQuery q;
    q.prepare(
        "INSERT INTO MESURE (IDMESURE, IDCITERNE, DATEMESURE, NIVEAUACTUEL, TEMPERATURE) "
        "VALUES (SEQ_MESURE.NEXTVAL, :idCiterne, SYSDATE, :niveau, :temp)");
    q.bindValue(":idCiterne", idCiterne);
    q.bindValue(":niveau",    niveauActuel);
    q.bindValue(":temp",      temperature);
    if (!q.exec())
        qDebug() << "[MESURE] Erreur INSERT :" << q.lastError().text();
    else
        qDebug() << "[MESURE] ✅ Mesure insérée — citerne" << idCiterne
                 << "niveau" << niveauActuel << "% temp" << temperature << "°C";
}

// ─────────────────────────────────────────────────────────────────────────────
// RAFRAÎCHIR LISTE PORTS
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::rafraichirListePorts()
{
    if (!m_comboPorts) return;
    m_comboPorts->clear();

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    if (ports.isEmpty()) {
        m_comboPorts->addItem("Aucun port disponible");
        m_comboPorts->setEnabled(false);
        statusBar()->showMessage("⚠ Aucun port série détecté.", 4000);
        return;
    }

    m_comboPorts->setEnabled(true);
    int indexCOM5 = -1;
    for (int i = 0; i < ports.size(); ++i) {
        const auto& p = ports[i];
        QString label = p.portName();
        if (!p.description().isEmpty()) label += "  —  " + p.description();
        m_comboPorts->addItem(label, p.portName());
        if (p.portName().compare("COM5", Qt::CaseInsensitive) == 0) indexCOM5 = i;
    }
    if (indexCOM5 >= 0) m_comboPorts->setCurrentIndex(indexCOM5);
    statusBar()->showMessage(QString("%1 port(s) série disponible(s)").arg(ports.size()), 3000);
}

// ─────────────────────────────────────────────────────────────────────────────
// ARDUINO — CONNEXION / DÉCONNEXION
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::on_btnConnecterArduino_clicked()
{
    if (m_arduinoConnecte) {
        if (m_arduino) {
            disconnect(m_arduino->getserial(), &QSerialPort::readyRead,
                       this, &Citernes::onDonneesArduinoRecues);
            m_arduino->close_arduino();
            delete m_arduino;
            m_arduino = nullptr;
        }
        m_arduinoConnecte          = false;
        m_idCiterneArduino         = 0;
        m_capteurActifPrecedent    = false;
        m_alerteDebordementActive  = false;

        if (m_btnArduino) {
            m_btnArduino->setText("🔌  Connecter Arduino");
            m_btnArduino->setStyleSheet(
                "background:#2196F3; color:white; border-radius:6px;"
                " padding:8px 14px; font-weight:bold; font-size:12px; border:none;");
        }
        if (m_labelStatut) m_labelStatut->setText("⚪  Non connecté");
        if (m_comboPorts)  m_comboPorts->setEnabled(true);
        statusBar()->showMessage("Arduino déconnecté.", 3000);
        return;
    }

    QString portChoisi = "COM5";
    if (m_comboPorts && m_comboPorts->isEnabled() && m_comboPorts->count() > 0) {
        QVariant ud = m_comboPorts->currentData();
        portChoisi  = ud.isValid() ? ud.toString()
                                  : m_comboPorts->currentText()
                                        .split("  —  ").first().trimmed();
    }

    if (m_labelStatut)
        m_labelStatut->setText(QString("🔄  Connexion sur %1...").arg(portChoisi));
    if (m_btnArduino) m_btnArduino->setEnabled(false);
    QApplication::processEvents();

    m_arduino = new Arduino();
    // Connexion directe sur le port choisi à 9600 baud (capteur niveau d'eau)
    int result = m_arduino->connect_arduino_port(portChoisi);
    // Pas de fallback connect_arduino() — il utilise 115200 pour le fingerprint

    if (result != 0) {
        delete m_arduino; m_arduino = nullptr;
        if (m_btnArduino) m_btnArduino->setEnabled(true);
        if (m_labelStatut) m_labelStatut->setText("🔴  Connexion échouée");

        QString msg;
        if (result == 1) {
            msg = QString("Port <b>%1</b> détecté mais impossible de l'ouvrir.<br><br>"
                          "✔ Vérifiez qu'aucun autre programme n'utilise ce port<br>"
                          "✔ Fermez l'IDE Arduino si ouvert<br>"
                          "✔ Débranchez et rebranchez le câble USB").arg(portChoisi);
        } else {
            QString portsList;
            foreach (const QSerialPortInfo &p, QSerialPortInfo::availablePorts())
                portsList += QString("<br>  • %1 — %2").arg(p.portName()).arg(p.description());
            if (portsList.isEmpty()) portsList = "<br>  <i>Aucun port détecté</i>";
            msg = QString("Arduino non trouvé sur <b>%1</b>.<br><br>"
                          "<b>Ports disponibles :</b>%2<br><br>"
                          "✔ Vérifiez que l'Arduino est branché en USB<br>"
                          "✔ Installez le pilote CH340 si nécessaire")
                      .arg(portChoisi).arg(portsList);
        }
        QMessageBox::warning(this, "Connexion Arduino échouée", msg);
        statusBar()->showMessage(QString("❌ Connexion échouée sur %1").arg(portChoisi), 5000);
        rafraichirListePorts();
        return;
    }

    if (m_labelStatut)
        m_labelStatut->setText(
            QString("🔄  Port ouvert sur %1 — attente démarrage Arduino...")
                .arg(m_arduino->getarduino_port_name()));
    statusBar()->showMessage("Arduino détecté, démarrage en cours...", 2500);
    QTimer::singleShot(2000, this, &Citernes::finishArduinoConnect);
}

void Citernes::finishArduinoConnect()
{
    if (!m_arduino || !m_arduino->getserial() || !m_arduino->getserial()->isOpen()) {
        if (m_btnArduino) m_btnArduino->setEnabled(true);
        if (m_labelStatut) m_labelStatut->setText("🔴  Connexion échouée");
        return;
    }

    // Réinitialiser tous les flags à chaque nouvelle connexion
    m_capteurActifPrecedent   = false;
    m_alerteDebordementActive = false;

    connect(m_arduino->getserial(), &QSerialPort::readyRead,
            this, &Citernes::onDonneesArduinoRecues);

    m_arduinoConnecte  = true;
    m_idCiterneArduino = 0;

    if (m_btnArduino) {
        m_btnArduino->setEnabled(true);
        m_btnArduino->setText("🔴  Déconnecter Arduino");
        m_btnArduino->setStyleSheet(
            "background:#F44336; color:white; border-radius:6px;"
            " padding:8px 14px; font-weight:bold; font-size:12px; border:none;");
    }
    if (m_labelStatut)
        m_labelStatut->setText(
            QString("🟢  Connecté sur %1").arg(m_arduino->getarduino_port_name()));
    if (m_comboPorts) m_comboPorts->setEnabled(false);

    statusBar()->showMessage(
        QString("✅ Arduino connecté sur %1").arg(m_arduino->getarduino_port_name()), 5000);

    QMessageBox::information(this, "Arduino connecté",
                             QString("✅ Arduino connecté sur <b>%1</b>.<br><br>"
                                     "• La LED s'allume si le niveau ≥ 90 %.<br>"
                                     "• Une alerte s'affiche une seule fois par débordement.<br>"
                                     "• Une mesure est insérée dans <b>MESURE</b> une seule fois "
                                     "par détection (passage sec → eau).")
                                 .arg(m_arduino->getarduino_port_name()));
}

// ─────────────────────────────────────────────────────────────────────────────
// ARDUINO — RÉCEPTION DONNÉES
//
// Règles :
//   1. Insertion MESURE  → front montant uniquement (sec → eau, pourcentage > 0)
//   2. Alerte popup      → première fois que le niveau atteint ≥ 90%
//                          (m_alerteDebordementActive empêche le spam)
//   3. m_alerteDebordementActive se remet à false dès que le niveau repasse < 90%
//      afin qu'un nouvel événement de débordement déclenche à nouveau l'alerte
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::onDonneesArduinoRecues()
{
    if (!m_arduino || !m_arduino->getserial()) return;

    static QByteArray buffer;
    buffer += m_arduino->getserial()->readAll();

    while (buffer.contains('\n')) {
        int     idx   = buffer.indexOf('\n');
        QString ligne = QString::fromUtf8(buffer.left(idx)).trimmed();
        buffer        = buffer.mid(idx + 1);

        qDebug() << "[Arduino →Qt]" << ligne;

        if (!ligne.startsWith("PERCENT:")) continue;

        bool ok;
        int pourcentage = ligne.mid(8).toInt(&ok);
        if (!ok) continue;

        // ─── 1. Calcul des flags ──────────────────────────────────────────
        bool niveauDebordement = (pourcentage >= 90);
        bool capteurSec        = (pourcentage == 0);

        // Remettre les flags à false quand le capteur repasse à sec
        // → prêt pour un nouvel événement
        if (capteurSec) {
            m_capteurActifPrecedent   = false;
            m_alerteDebordementActive = false;
        }

        // ─── 2. Résoudre ID citerne et température ────────────────────────
        int    idCit = getIdCiterneArduino();
        double temp  = m_temperatureArduino;

        if (idCit > 0) {
            QSqlQuery qTemp;
            qTemp.prepare("SELECT TEMPERATURE_CITERNE FROM CITERNE WHERE ID_CITERNE = :id");
            qTemp.bindValue(":id", idCit);
            if (qTemp.exec() && qTemp.next() && !qTemp.value(0).isNull()) {
                temp = qTemp.value(0).toDouble();
                m_temperatureArduino = temp;
            }
        }

        // ─── 3. Insertion BDD — une seule fois quand niveau ≥ 90% ────────
        // m_capteurActifPrecedent sert ici de flag "mesure déjà insérée pour cet événement"
        if (niveauDebordement && !m_capteurActifPrecedent) {
            m_capteurActifPrecedent = true; // verrouiller pour cet événement
            sauvegarderMesure(idCit, pourcentage, temp);
            qDebug() << "[Arduino] ✅ Niveau ≥ 90% → mesure insérée une seule fois. Niveau :"
                     << pourcentage << "%";
        } else if (niveauDebordement) {
            qDebug() << "[Arduino] Niveau ≥ 90% (" << pourcentage << "%) — mesure déjà insérée.";
        } else {
            qDebug() << "[Arduino] Niveau normal :" << pourcentage << "%";
        }

        // ─── 4. Mise à jour label statut ─────────────────────────────────
        if (m_labelStatut && m_arduinoConnecte) {
            QString couleur = niveauDebordement ? "#F44336"
                              : (pourcentage >= 70) ? "#FF9800" : "#4CAF50";
            m_labelStatut->setText(
                QString("🟢  Connecté sur %1  |  Citerne <b>#%2</b>  |  Niveau : "
                        "<b style='color:%3;'>%4 %</b>  |  T° : %5 °C")
                    .arg(m_arduino->getarduino_port_name())
                    .arg(idCit).arg(couleur)
                    .arg(pourcentage)
                    .arg(QString::number(temp, 'f', 1)));
        }

        // ─── 5. Gestion débordement ───────────────────────────────────────
        if (niveauDebordement) {

            // LED toujours allumée tant que le niveau est ≥ 90%
            envoyerCommandeLED(true);

            // Alerte popup : seulement si ce débordement n'a pas encore été signalé
            if (!m_alerteDebordementActive) {
                m_alerteDebordementActive = true;   // verrouiller pour ce débordement

                QString message = QString(
                                      "<b style='color:#F44336; font-size:14px;'>"
                                      "🚨 RISQUE DE DÉBORDEMENT DÉTECTÉ !</b><br><br>"
                                      "<table cellspacing='6'>"
                                      "<tr><td><b>Source :</b></td>"
                                      "<td style='color:#1565C0;'>Capteur Arduino (mesure physique)</td></tr>"
                                      "<tr><td><b>Citerne surveillée :</b></td>"
                                      "<td><b>#%1</b></td></tr>"
                                      "<tr><td><b>Niveau détecté :</b></td>"
                                      "<td style='color:#F44336; font-weight:bold; font-size:13px;'>%2 %</td></tr>"
                                      "<tr><td><b>Température :</b></td>"
                                      "<td>%3 °C</td></tr>"
                                      "<tr><td><b>Port Arduino :</b></td>"
                                      "<td>%4</td></tr>"
                                      "<tr><td><b>État :</b></td>"
                                      "<td style='color:#F44336; font-weight:bold;'>"
                                      "⛔ NIVEAU MAXIMUM ATTEINT</td></tr>"
                                      "<tr><td><b>LED Arduino :</b></td>"
                                      "<td style='color:#E65100;'>🔴 Allumée automatiquement</td></tr>"
                                      "<tr><td><b>Mesure :</b></td>"
                                      "<td style='color:#2E7D32;'>✅ Enregistrée dans la table MESURE</td></tr>"
                                      "</table><br>"
                                      "<b style='color:#B71C1C;'>⚡ Action requise : "
                                      "Arrêtez immédiatement le remplissage !</b>")
                                      .arg(idCit)
                                      .arg(pourcentage)
                                      .arg(QString::number(temp, 'f', 1))
                                      .arg(m_arduino ? m_arduino->getarduino_port_name() : "N/A");

                QMessageBox alert(this);
                alert.setWindowTitle("⚠ ALERTE — Débordement détecté !");
                alert.setIcon(QMessageBox::Critical);
                alert.setText(message);
                alert.setStandardButtons(QMessageBox::Ok);
                QPushButton* btnEteindre =
                    alert.addButton("🔴 Éteindre la LED Arduino", QMessageBox::ActionRole);
                alert.exec();

                if (alert.clickedButton() == btnEteindre) {
                    envoyerCommandeLED(false);
                    statusBar()->showMessage("LED éteinte.", 3000);
                }
            }

            statusBar()->showMessage(
                QString("🔴 ALERTE DÉBORDEMENT — Niveau Arduino : %1 %").arg(pourcentage),
                10000);

        } else {
            // Niveau normal : éteindre la LED
            envoyerCommandeLED(false);
            statusBar()->showMessage(
                QString("📊 Niveau Arduino : %1 %  —  Normal  (citerne #%2, T° %3 °C)")
                    .arg(pourcentage).arg(idCit)
                    .arg(QString::number(temp, 'f', 1)),
                2000);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ARDUINO — COMMANDE LED
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::envoyerCommandeLED(bool allumer)
{
    if (!m_arduinoConnecte || !m_arduino || !m_arduino->getserial()) return;
    if (!m_arduino->getserial()->isOpen()) return;
    m_arduino->write_to_arduino(allumer ? "1" : "0");
}

// ═════════════════════════════════════════════════════════════════════════════
// HELPERS Qt Charts
// ═════════════════════════════════════════════════════════════════════════════

QList<CiterneData> Citernes::lireDonneesCiternes()
{
    QList<CiterneData> rows;
    QSqlQuery q;
    q.exec("SELECT ID_CITERNE,CAPACITEMAX,NIVEAUACTUEL,TYPEHUILE,"
           "TEMPERATURE_CITERNE,ETAT_CITERNE FROM CITERNE ORDER BY ID_CITERNE");
    while (q.next()) {
        CiterneData d;
        d.id           = q.value(0).toInt();
        d.capaciteMax  = q.value(1).toDouble();
        d.niveauActuel = q.value(2).toDouble();
        d.typeHuile    = q.value(3).toString();
        d.temperature  = q.value(4).toDouble();
        d.etat         = q.value(5).toString();
        rows.append(d);
    }
    return rows;
}

QChart* Citernes::makeChart(const QString &title)
{
    QChart* c = new QChart();
    c->setTitle(title);
    c->setTitleFont(QFont("Segoe UI", 10, QFont::Bold));
    c->setTitleBrush(QBrush(Qt::black));
    c->setAnimationOptions(QChart::SeriesAnimations);
    c->setBackgroundVisible(false);
    c->legend()->setFont(QFont("Segoe UI", 8));
    c->legend()->setLabelColor(Qt::black);
    return c;
}

QChartView* Citernes::makeChartView(QChart *chart, int minHeight)
{
    QChartView* v = new QChartView(chart, ui->chartStatusContainer);
    v->setRenderHint(QPainter::Antialiasing);
    v->setMinimumHeight(minHeight);
    return v;
}

void Citernes::afficherGraphique(QChartView* view)
{
    if (m_currentChart) {
        m_currentChart->hide();
        m_currentChart->deleteLater();
        m_currentChart = nullptr;
    }
    if (!view) return;
    QRect r = ui->chartStatusContainer->rect();
    view->setGeometry(0, 36, r.width(), r.height() - 36);
    view->show(); view->raise();
    m_currentChart = view;
}

// ─────────────────────────────────────────────────────────────────────────────
// CHARGER / RAFRAÎCHIR LISTE
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::chargerListeCiternes()
{
    QString orderBy = "ID_CITERNE";
    QString t = ui->Trier->currentText();
    if      (t == "Niveau Actuel")     orderBy = "NIVEAUACTUEL";
    else if (t == "Type Huile")        orderBy = "TYPEHUILE";
    else if (t == "Capacite Maximale") orderBy = "CAPACITEMAX";
    else if (t == "Température")       orderBy = "TEMPERATURE_CITERNE";
    else if (t == "Etat")              orderBy = "ETAT_CITERNE";

    QSqlQuery query;
    if (!query.exec(
            QString("SELECT ID_CITERNE,CAPACITEMAX,NIVEAUACTUEL,TYPEHUILE,"
                    "TEMPERATURE_CITERNE,ETAT_CITERNE FROM CITERNE ORDER BY %1 ASC")
                .arg(orderBy))) {
        QMessageBox::critical(this,"Erreur SQL",query.lastError().text()); return;
    }

    ui->ListeCiterne->clearContents();
    ui->ListeCiterne->setRowCount(0);
    ui->ListeCiterne->setColumnCount(7);
    ui->ListeCiterne->setHorizontalHeaderLabels(
        {"ID","Capacité Max","Niveau Actuel","Type Huile","Température","État","Action"});
    ui->ListeCiterne->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->ListeCiterne->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    ui->ListeCiterne->setAlternatingRowColors(true);
    ui->ListeCiterne->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->ListeCiterne->verticalHeader()->setVisible(false);

    int row = 0;
    while (query.next()) {
        ui->ListeCiterne->insertRow(row);
        double cap = query.value(1).toDouble(), niv = query.value(2).toDouble();
        bool   pleine = (cap > 0 && niv >= cap);
        for (int col = 0; col < 6; ++col) {
            auto* item = new QTableWidgetItem(query.value(col).toString());
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            if (pleine) {
                item->setBackground(QBrush(QColor("#FFEBEE")));
                item->setForeground(QBrush(QColor("#C62828")));
            }
            ui->ListeCiterne->setItem(row, col, item);
        }
        QWidget* cw  = new QWidget(ui->ListeCiterne);
        auto*    lay = new QHBoxLayout(cw);
        lay->setContentsMargins(2,2,2,2); lay->setSpacing(6);
        auto* bm = new QPushButton("Modifier",  cw);
        bm->setStyleSheet("background:#2196F3;color:white;border-radius:4px;padding:5px;");
        bm->setFixedSize(70,30);
        auto* bs = new QPushButton("Supprimer", cw);
        bs->setStyleSheet("background:#F44336;color:white;border-radius:4px;padding:5px;");
        bs->setFixedSize(70,30);
        lay->addWidget(bm); lay->addWidget(bs);
        lay->setAlignment(Qt::AlignCenter);
        ui->ListeCiterne->setCellWidget(row, 6, cw);
        connect(bm, &QPushButton::clicked, this, [this,row]{ onModifier(row); });
        connect(bs, &QPushButton::clicked, this, [this,row]{ onSupprimer(row); });
        ui->ListeCiterne->setRowHeight(row, 50);
        ++row;
    }
    statusBar()->showMessage(QString("%1 citernes chargées").arg(row), 3000);
}

void Citernes::rafraichirListe() { chargerListeCiternes(); }

// ─────────────────────────────────────────────────────────────────────────────
// MODIFIER / SUPPRIMER
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::onModifier(int row)
{
    if (row < 0 || row >= ui->ListeCiterne->rowCount()) return;
    QString id = ui->ListeCiterne->item(row,0)->text();
    double  ca = ui->ListeCiterne->item(row,1)->text().toDouble();
    double  na = ui->ListeCiterne->item(row,2)->text().toDouble();
    QString ty = ui->ListeCiterne->item(row,3)->text();
    double  ta = ui->ListeCiterne->item(row,4)->text().toDouble();
    QString et = ui->ListeCiterne->item(row,5)->text();

    QDialog dlg(this);
    dlg.setWindowTitle(QString("Modifier citerne %1").arg(id));
    dlg.setMinimumWidth(420);
    QFormLayout* form = new QFormLayout(&dlg);

    auto* editCap  = new QLineEdit(QString::number(ca), &dlg);
    auto* editNiv  = new QLineEdit(QString::number(na), &dlg);
    auto* v = new QRegularExpressionValidator(
        QRegularExpression("^\\d{0,10}(\\.\\d{0,4})?$"), &dlg);
    editCap->setValidator(v); editNiv->setValidator(v);

    auto* editType = new QComboBox(&dlg);
    editType->addItems({"Olive","Raffinée","Bio","Alimentaire","Industrielle"});

    auto* editTemp = new QDoubleSpinBox(&dlg);
    editTemp->setRange(-50,150); editTemp->setDecimals(2);
    editTemp->setValue(ta); editTemp->setSuffix(" °C");

    auto* editEtat = new QComboBox(&dlg);
    editEtat->addItems({"Vide","Pleine","En utilisation","En remplissage","En maintenance","Bloquee"});

    int ix = editType->findText(ty,Qt::MatchFixedString); if(ix>=0) editType->setCurrentIndex(ix);
    int ie = editEtat->findText(et,Qt::MatchFixedString); if(ie>=0) editEtat->setCurrentIndex(ie);

    form->addRow("Capacité max (L):", editCap);
    form->addRow("Niveau actuel (L):", editNiv);
    form->addRow("Type d'huile:", editType);
    form->addRow("Température:", editTemp);
    form->addRow("État:", editEtat);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(btns);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    double newCap = editCap->text().toDouble(), newNiv = editNiv->text().toDouble();
    if (newCap <= 0) { QMessageBox::warning(this,"Erreur","Capacité doit être positive"); return; }
    if (newNiv < 0 || newNiv > newCap) { QMessageBox::warning(this,"Erreur","Niveau invalide"); return; }

    QSqlQuery q;
    q.prepare("UPDATE CITERNE SET CAPACITEMAX=:cap,NIVEAUACTUEL=:niv,TYPEHUILE=:type,"
              "TEMPERATURE_CITERNE=:temp,ETAT_CITERNE=:etat WHERE ID_CITERNE=:id");
    q.bindValue(":cap",  newCap);
    q.bindValue(":niv",  newNiv);
    q.bindValue(":type", editType->currentText());
    q.bindValue(":temp", editTemp->value());
    q.bindValue(":etat", editEtat->currentText());
    q.bindValue(":id",   id);

    if (!q.exec()) QMessageBox::critical(this,"Erreur SQL",q.lastError().text());
    else { QMessageBox::information(this,"Succès","Citerne modifiée"); rafraichirListe(); }
}

void Citernes::onSupprimer(int row)
{
    if (row < 0 || row >= ui->ListeCiterne->rowCount()) return;
    QString id = ui->ListeCiterne->item(row,0)->text();

    if (QMessageBox::question(this,"Confirmation",
                              QString("Supprimer la citerne %1 ?").arg(id),
                              QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes) return;

    QSqlQuery qMes;
    qMes.prepare("DELETE FROM MESURE WHERE IDCITERNE = :id");
    qMes.bindValue(":id", id);
    qMes.exec();

    QSqlQuery q;
    q.prepare("DELETE FROM CITERNE WHERE ID_CITERNE=:id");
    q.bindValue(":id", id);
    if (q.exec()) {
        ui->ListeCiterne->removeRow(row);
        QMessageBox::information(this,"Succès",QString("Citerne %1 supprimée.").arg(id));
        statusBar()->showMessage(QString("Citerne %1 supprimée").arg(id), 3000);
        if (m_idCiterneArduino == id.toInt()) {
            m_idCiterneArduino        = 0;
            m_capteurActifPrecedent   = false;
            m_alerteDebordementActive = false;
        }
    } else {
        QMessageBox::critical(this,"Erreur SQL",q.lastError().text());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AJOUTER
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::on_ConAjout_clicked()
{
    QString capStr = ui->SaiCapa->text().trimmed();
    QString nivStr = ui->SaiNiv->text().trimmed();
    QString type   = ui->SaiTyp->currentText();
    double  temp   = ui->SaiTem->value();
    QString etat   = ui->SaiEtat->currentText();

    bool hasError = false;
    QStringList messages;

    bool okCap = false; double cap = capStr.toDouble(&okCap);
    if (capStr.isEmpty()) {
        setErreur(ui->SaiCapa,m_errCapa,"La capacité est obligatoire");
        messages << "• Capacité obligatoire."; hasError = true;
    } else if (!okCap || cap <= 0) {
        setErreur(ui->SaiCapa,m_errCapa,"La capacité doit être un nombre positif");
        messages << "• Capacité doit être positive."; hasError = true;
    } else setOk(ui->SaiCapa,m_errCapa);

    bool okNiv = false; double niv = nivStr.toDouble(&okNiv);
    if (nivStr.isEmpty()) {
        setErreur(ui->SaiNiv,m_errNiv,"Le niveau est obligatoire");
        messages << "• Niveau obligatoire."; hasError = true;
    } else if (!okNiv || niv < 0) {
        setErreur(ui->SaiNiv,m_errNiv,"Le niveau doit être positif ou nul");
        messages << "• Niveau doit être positif ou nul."; hasError = true;
    } else if (okCap && cap > 0 && niv > cap) {
        setErreur(ui->SaiNiv,m_errNiv,
                  QString("Niveau (%1 L) > Capacité (%2 L)")
                      .arg(QString::number(niv,'f',0)).arg(QString::number(cap,'f',0)));
        messages << "• Niveau ne peut pas dépasser la capacité."; hasError = true;
    } else setOk(ui->SaiNiv,m_errNiv);

    if (hasError) {
        QMessageBox::warning(this,"Erreurs de saisie",
                             "Veuillez corriger :\n\n" + messages.join("\n")); return;
    }

    Seuils s = getSeuilsTemp(type);
    if (temp >= s.danger) {
        QMessageBox::critical(this,"Température dangereuse",
                              QString("<b>⛔ DANGER : %1°C !</b><br>Plage : %2–%3°C<br>"
                                      "Seuil danger : <b style='color:#F44336;'>%4°C</b><br><br>"
                                      "Ajout <b>bloqué</b>.")
                                  .arg(temp).arg(s.minOk).arg(s.maxOk).arg(s.danger));
        return;
    } else if (temp > s.maxOk) {
        if (QMessageBox::warning(this,"Température élevée",
                                 QString("<b>⚠ %1°C — au-dessus de la plage optimale</b><br>"
                                         "Plage : %2–%3°C<br>Continuer quand même ?")
                                     .arg(temp).arg(s.minOk).arg(s.maxOk),
                                 QMessageBox::Yes|QMessageBox::No) == QMessageBox::No) return;
    } else if (temp < s.minOk) {
        if (QMessageBox::warning(this,"Température basse",
                                 QString("<b>❄ %1°C — en dessous de la plage optimale</b><br>"
                                         "Plage : %2–%3°C<br>Continuer quand même ?")
                                     .arg(temp).arg(s.minOk).arg(s.maxOk),
                                 QMessageBox::Yes|QMessageBox::No) == QMessageBox::No) return;
    }

    double pct = niv / cap * 100.0;
    bool   etatIncoherent = false;
    QString suggestionEtat;
    if (etat == "Pleine" && pct < 100.0) {
        etatIncoherent = true;
        suggestionEtat = (pct==0.0) ? "Vide" : (pct<50.0) ? "En remplissage" : "En utilisation";
    } else if (etat == "Vide" && pct > 0.0) {
        etatIncoherent = true;
        suggestionEtat = (pct>=100.0) ? "Pleine" : (pct>=50.0) ? "En utilisation" : "En remplissage";
    }
    if (etatIncoherent) {
        auto rep = QMessageBox::question(this,"État incohérent",
                                         QString("État <b>\"%1\"</b> incohérent avec niveau <b>%2%</b>.<br>"
                                                 "Suggestion : <b>\"%3\"</b><br>Utiliser la suggestion ?")
                                             .arg(etat).arg(QString::number(pct,'f',1)).arg(suggestionEtat),
                                         QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
        if (rep == QMessageBox::Cancel) return;
        if (rep == QMessageBox::Yes) {
            etat = suggestionEtat;
            ui->SaiEtat->setCurrentText(etat);
            setOkCombo(ui->SaiEtat, m_errEtat);
        }
    }

    QSqlQuery q;
    q.prepare("INSERT INTO CITERNE(CAPACITEMAX,NIVEAUACTUEL,TYPEHUILE,"
              "TEMPERATURE_CITERNE,ETAT_CITERNE) VALUES(:cap,:niv,:type,:temp,:etat)");
    q.bindValue(":cap",  cap);
    q.bindValue(":niv",  niv);
    q.bindValue(":type", type);
    q.bindValue(":temp", temp);
    q.bindValue(":etat", etat);

    if (q.exec()) {
        QMessageBox::information(this,"Succès","✅ Citerne ajoutée !");
        ui->SaiCapa->clear(); ui->SaiNiv->clear();
        ui->SaiTyp->setCurrentIndex(0); ui->SaiTem->setValue(20.0);
        ui->SaiEtat->setCurrentIndex(0);
        setNeutre(ui->SaiCapa,m_errCapa);
        setNeutre(ui->SaiNiv,m_errNiv);
        setNeutreCombo(ui->SaiEtat,m_errEtat);
        rafraichirListe();
        ui->metiersCiternes->setCurrentWidget(ui->consulterciterne);
        statusBar()->showMessage("Citerne ajoutée", 3000);
    } else {
        QMessageBox::critical(this,"Erreur SQL",q.lastError().text());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// NAVIGATION
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::on_AjoutCiterne_clicked()
{
    ui->SaiCapa->clear(); ui->SaiNiv->clear();
    ui->SaiTyp->setCurrentIndex(0); ui->SaiTem->setValue(20.0);
    ui->SaiEtat->setCurrentIndex(0);
    setNeutre(ui->SaiCapa,m_errCapa);
    setNeutre(ui->SaiNiv,m_errNiv);
    setNeutreCombo(ui->SaiEtat,m_errEtat);
    ui->metiersCiternes->setCurrentWidget(ui->ajoutCiternes);
}

void Citernes::on_ConsulterCiterne_clicked()
{ ui->metiersCiternes->setCurrentWidget(ui->consulterciterne); rafraichirListe(); }

void Citernes::on_MetierAvanceCiterne_clicked()
{ ui->metiersCiternes->setCurrentWidget(ui->AvCiterne); }

// ─────────────────────────────────────────────────────────────────────────────
// RECHERCHE
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::on_Recherche_clicked()
{
    QString r = ui->SaiRecherche->text().trimmed();
    if (r.isEmpty()) { rafraichirListe(); return; }

    bool    isNum = false;
    r.toInt(&isNum);
    QString cond = isNum
                       ? QString("TO_CHAR(ID_CITERNE) LIKE '%%%1%'").arg(r)
                       : QString("UPPER(TYPEHUILE) LIKE UPPER('%%%1%')").arg(r);

    QSqlQuery query;
    if (!query.exec(
            QString("SELECT ID_CITERNE,CAPACITEMAX,NIVEAUACTUEL,TYPEHUILE,"
                    "TEMPERATURE_CITERNE,ETAT_CITERNE FROM CITERNE WHERE %1 "
                    "ORDER BY ID_CITERNE").arg(cond))) {
        QMessageBox::critical(this,"Erreur SQL",query.lastError().text()); return;
    }

    ui->ListeCiterne->clearContents();
    ui->ListeCiterne->setRowCount(0);
    int row = 0;
    while (query.next()) {
        ui->ListeCiterne->insertRow(row);
        for (int col = 0; col < 6; ++col) {
            auto* item = new QTableWidgetItem(query.value(col).toString());
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            if (isNum && col==0) {
                item->setBackground(QBrush(QColor("#FFF9C4")));
                item->setForeground(QBrush(QColor("#E65100")));
            } else if (!isNum && col==3) {
                item->setBackground(QBrush(QColor("#E8F5E9")));
                item->setForeground(QBrush(QColor("#2E7D32")));
            }
            ui->ListeCiterne->setItem(row,col,item);
        }
        QWidget* cw  = new QWidget(ui->ListeCiterne);
        auto*    lay = new QHBoxLayout(cw);
        lay->setContentsMargins(2,2,2,2); lay->setSpacing(6);
        auto* bm = new QPushButton("Modifier",  cw);
        bm->setStyleSheet("background:#2196F3;color:white;border-radius:4px;padding:5px;");
        bm->setFixedSize(70,30);
        auto* bs = new QPushButton("Supprimer", cw);
        bs->setStyleSheet("background:#F44336;color:white;border-radius:4px;padding:5px;");
        bs->setFixedSize(70,30);
        lay->addWidget(bm); lay->addWidget(bs);
        lay->setAlignment(Qt::AlignCenter);
        ui->ListeCiterne->setCellWidget(row,6,cw);
        connect(bm, &QPushButton::clicked, this, [this,row]{ onModifier(row); });
        connect(bs, &QPushButton::clicked, this, [this,row]{ onSupprimer(row); });
        ui->ListeCiterne->setRowHeight(row, 50);
        ++row;
    }
    statusBar()->showMessage(row==0
                                 ? QString("Aucune citerne pour \"%1\"").arg(r)
                                 : QString("%1 citerne(s) pour \"%2\"").arg(row).arg(r), 4000);
}

// ─────────────────────────────────────────────────────────────────────────────
// EXPORT PDF — LISTE CITERNES
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::on_exporterListeCiterne_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(
        this, "Exporter en PDF",
        QString("SmartOil_Citernes_%1.pdf")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "Fichiers PDF (*.pdf)");
    if (fileName.isEmpty()) return;

    QSqlQuery qs;
    int tot=0; double capT=0,nivT=0,tmpM=0;
    qs.exec("SELECT COUNT(*),SUM(CAPACITEMAX),SUM(NIVEAUACTUEL),AVG(TEMPERATURE_CITERNE) FROM CITERNE");
    if (qs.next()) {
        tot=qs.value(0).toInt(); capT=qs.value(1).toDouble();
        nivT=qs.value(2).toDouble(); tmpM=qs.value(3).toDouble();
    }
    Q_UNUSED(capT); Q_UNUSED(nivT); Q_UNUSED(tmpM);

    QString html =
        "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
        "<style>body{font-family:'Segoe UI',Arial,sans-serif;margin:30px;color:#333;}"
        ".hdr{background:linear-gradient(135deg,#1565C0,#42A5F5);color:white;padding:24px;"
        "border-radius:10px;margin-bottom:20px;}.hdr h1{margin:0;font-size:24px;}"
        "h2{color:#1565C0;border-bottom:2px solid #e3f2fd;padding-bottom:4px;font-size:14px;}"
        "table{width:100%;border-collapse:collapse;font-size:12px;}"
        "thead tr{background:#1565C0;color:white;}th{padding:8px;text-align:left;}"
        "tbody tr:nth-child(even){background:#f5f9ff;}td{padding:7px;border-bottom:1px solid #e0e0e0;}"
        ".footer{margin-top:30px;font-size:10px;color:#aaa;text-align:center;}"
        "</style></head><body>"
        "<div class='hdr'><h1>Rapport — Gestion des Citernes</h1><p>SmartOil · Généré le "
        + QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm") + "</p></div>";

    html += QString("<p><b>Total citernes :</b> %1</p>").arg(tot);
    html += "<h2>Liste détaillée</h2><table><thead><tr>"
            "<th>ID</th><th>Cap. max</th><th>Niveau</th>"
            "<th>Occup.%</th><th>Type</th><th>Temp.</th><th>État</th>"
            "</tr></thead><tbody>";

    QSqlQuery qd("SELECT ID_CITERNE,CAPACITEMAX,NIVEAUACTUEL,TYPEHUILE,"
                 "TEMPERATURE_CITERNE,ETAT_CITERNE FROM CITERNE ORDER BY ID_CITERNE");
    while (qd.next()) {
        double c=qd.value(1).toDouble(), n=qd.value(2).toDouble();
        double o=(c>0)?(n/c*100.0):0.0;
        QString co=(o>90)?"color:#E53935;font-weight:bold;":(o>70)?"color:#FB8C00;":"color:#43A047;";
        html += "<tr><td>"+qd.value(0).toString()+"</td>"
                                                      "<td>"+QString::number(c,'f',0)+"</td>"
                                               "<td>"+QString::number(n,'f',0)+"</td>"
                                               "<td style='"+co+"'>"+QString::number(o,'f',1)+"%</td>"
                                                           "<td>"+qd.value(3).toString()+"</td>"
                                           "<td>"+qd.value(4).toString()+"</td>"
                                           "<td>"+qd.value(5).toString()+"</td></tr>";
    }
    html += "</tbody></table>"
            "<div class='footer'>SmartOil — Document généré automatiquement.</div>"
            "</body></html>";

    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(15,15,15,15), QPageLayout::Millimeter);
    QTextDocument doc; doc.setHtml(html); doc.print(&printer);

    QMessageBox::information(this,"Export réussi",QString("PDF exporté !\n%1").arg(fileName));
    statusBar()->showMessage("PDF exporté : "+fileName, 5000);
}

// ─────────────────────────────────────────────────────────────────────────────
// STATISTIQUES
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::afficherStatistiques()
{
    QList<CiterneData> rows = lireDonneesCiternes();
    if (rows.isEmpty()) { afficherGraphique(nullptr); return; }

    const QList<QColor> pal = {
        QColor("#2196F3"),QColor("#4CAF50"),QColor("#FF9800"),QColor("#F44336"),
        QColor("#9C27B0"),QColor("#00BCD4"),QColor("#795548"),QColor("#607D8B")
    };

    QString    choix = ui->X->currentText();
    QChartView* view = nullptr;

    if (choix.contains("Taux",Qt::CaseInsensitive)) {
        double totCap=0,totNiv=0;
        for(const auto& d:rows){totCap+=d.capaciteMax;totNiv+=d.niveauActuel;}
        double taux=(totCap>0)?(totNiv/totCap*100.0):0.0;
        QColor cTaux=(taux>85)?QColor("#F44336"):(taux>60)?QColor("#FF9800"):QColor("#4CAF50");

        auto* setR=new QBarSet("Rempli"); *setR<<taux; setR->setColor(cTaux);
        auto* setL=new QBarSet("Libre");  *setL<<100.0-taux; setL->setColor(QColor("#E0E0E0"));
        auto* series=new QHorizontalStackedBarSeries();
        series->append(setR); series->append(setL);
        series->setLabelsVisible(true);
        series->setLabelsPosition(QAbstractBarSeries::LabelsCenter);

        auto* axY=new QBarCategoryAxis();
        axY->append(QStringList()<<"Système global");
        auto* axX=new QValueAxis();
        axX->setRange(0,100); axX->setLabelFormat("%.0f %%");
        axX->setTitleText("Taux de remplissage (%)"); axX->setTickCount(6);

        QChart* chart=makeChart(QString("Taux global : %1%  —  %2 L / %3 L")
                                      .arg(QString::number(taux,'f',1))
                                      .arg(QString::number(totNiv,'f',0))
                                      .arg(QString::number(totCap,'f',0)));
        chart->addSeries(series);
        chart->addAxis(axY,Qt::AlignLeft); chart->addAxis(axX,Qt::AlignBottom);
        series->attachAxis(axY); series->attachAxis(axX);
        chart->legend()->setAlignment(Qt::AlignBottom);
        view=makeChartView(chart,200);
    }
    else if (choix.contains("Temp",Qt::CaseInsensitive)) {
        double tMin=999,tMax=-999,tSum=0;
        for(const auto& d:rows){
            tMin=qMin(tMin,d.temperature);tMax=qMax(tMax,d.temperature);tSum+=d.temperature;
        }
        double tMoy=tSum/rows.size();

        auto* setTemp=new QBarSet("Température (°C)"); setTemp->setColor(QColor("#2196F3"));
        QStringList cats;
        for(const auto& d:rows){*setTemp<<d.temperature;cats<<QString("#%1").arg(d.id);}

        auto* series=new QBarSeries(); series->append(setTemp);
        series->setLabelsVisible(true);
        series->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);

        auto* lineMoy=new QLineSeries();
        lineMoy->setName(QString("Moy. %1°C").arg(QString::number(tMoy,'f',1)));
        QPen pen(QColor("#F44336")); pen.setStyle(Qt::DashLine); pen.setWidth(2);
        lineMoy->setPen(pen);
        for(int i=0;i<rows.size();++i) lineMoy->append(i,tMoy);

        auto* axX=new QBarCategoryAxis(); axX->append(cats); axX->setTitleText("Citernes");
        auto* axY=new QValueAxis(); axY->setTitleText("Température (°C)");
        axY->setLabelFormat("%.1f"); axY->setRange(qMax(0.0,tMin-5),tMax+5);
        auto* axX2=new QValueAxis(); axX2->setRange(-0.5,rows.size()-0.5); axX2->setVisible(false);

        QChart* chart=makeChart("Températures des citernes");
        chart->addSeries(series); chart->addSeries(lineMoy);
        chart->addAxis(axX,Qt::AlignBottom); chart->addAxis(axY,Qt::AlignLeft);
        chart->addAxis(axX2,Qt::AlignBottom);
        series->attachAxis(axX); series->attachAxis(axY);
        lineMoy->attachAxis(axX2); lineMoy->attachAxis(axY);
        chart->legend()->setAlignment(Qt::AlignBottom);
        view=makeChartView(chart);
    }
    else if (choix.contains("Huile",Qt::CaseInsensitive)||choix.contains("part",Qt::CaseInsensitive)) {
        QMap<QString,int> tc; for(const auto& d:rows) tc[d.typeHuile]++;
        auto* series=new QPieSeries(); int ci=0;
        for(auto it=tc.constBegin();it!=tc.constEnd();++it,++ci){
            double pct=100.0*it.value()/rows.size();
            auto* sl=series->append(
                QString("%1\n%2 (%3%)").arg(it.key()).arg(it.value())
                    .arg(QString::number(pct,'f',1)), it.value());
            sl->setColor(pal[ci%pal.size()]);
            sl->setLabelVisible(true);
            sl->setLabelFont(QFont("Segoe UI",8));
            sl->setLabelColor(Qt::black);
        }
        series->setHoleSize(0.38);
        QChart* chart=makeChart("Répartition des types d'huile");
        chart->addSeries(series); chart->legend()->setAlignment(Qt::AlignRight);
        view=makeChartView(chart);
    }
    else if (choix.contains("tat",Qt::CaseInsensitive)) {
        QMap<QString,int> ec; for(const auto& d:rows) ec[d.etat]++;
        QMap<QString,QColor> coul={
            {"Vide",QColor("#90A4AE")},{"Pleine",QColor("#4CAF50")},
            {"En utilisation",QColor("#FF9800")},{"En remplissage",QColor("#2196F3")},
            {"En maintenance",QColor("#F44336")},{"Bloquee",QColor("#795548")}
        };
        auto* series=new QPieSeries();
        for(auto it=ec.constBegin();it!=ec.constEnd();++it){
            double pct=100.0*it.value()/rows.size();
            auto* sl=series->append(
                QString("%1\n%2 (%3%)").arg(it.key()).arg(it.value())
                    .arg(QString::number(pct,'f',1)), it.value());
            sl->setColor(coul.value(it.key(),QColor("#9C27B0")));
            sl->setLabelVisible(true);
            sl->setLabelFont(QFont("Segoe UI",8));
            sl->setLabelColor(Qt::black);
        }
        series->setHoleSize(0.40);
        QChart* chart=makeChart("État des citernes");
        chart->addSeries(series); chart->legend()->setAlignment(Qt::AlignRight);
        view=makeChartView(chart);
    }

    afficherGraphique(view);
}

void Citernes::on_StatistiqueCiterne_clicked()
{
    ui->metiersCiternes->setCurrentWidget(ui->statCiterne);
    ui->X->setEnabled(true); ui->X->setVisible(true); ui->X->setFocus();
    afficherStatistiques();
}

void Citernes::on_X_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if(ui->metiersCiternes->currentWidget()==ui->statCiterne) afficherStatistiques();
}

QLabel* Citernes::creerGraphiqueBarres(const QStringList&,const QList<double>&,
                                       const QString&,const QString&,double) { return nullptr; }
QLabel* Citernes::creerGraphiqueCirculaire(const QStringList&,const QList<double>&,
                                           const QString&) { return nullptr; }

// ─────────────────────────────────────────────────────────────────────────────
// BESOIN ACHAT
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::chargerBesoinsAchat()
{
    m_listeBesoins.clear();
    QSqlQuery q;
    q.exec("SELECT id,typeHuile,quantiteManquante,capaciteSuggeree,"
           "nombreCiternesRequises,statut,dateAjout FROM BESOIN_ACHAT ORDER BY id DESC");
    while(q.next()){
        BesoinAchat b;
        b.id=q.value(0).toInt(); b.typeHuile=q.value(1).toString();
        b.quantiteManquante=q.value(2).toDouble(); b.capaciteSuggeree=q.value(3).toDouble();
        b.nombreCiternesRequises=q.value(4).toInt(); b.statut=q.value(5).toString();
        b.dateAjout=QDateTime::fromString(q.value(6).toString(),Qt::ISODate);
        m_listeBesoins.append(b);
    }
}

void Citernes::sauvegarderBesoinAchat(int id,const QString& typeHuile,
                                      double qte,double capa,int nb,
                                      const QString& statut,const QDateTime& date)
{
    QSqlQuery q;
    q.prepare("INSERT INTO BESOIN_ACHAT(id,typeHuile,quantiteManquante,capaciteSuggeree,"
              "nombreCiternesRequises,statut,dateAjout) "
              "VALUES(:id,:type,:qte,:capa,:nb,:statut,:date)");
    q.bindValue(":id",id); q.bindValue(":type",typeHuile); q.bindValue(":qte",qte);
    q.bindValue(":capa",capa); q.bindValue(":nb",nb); q.bindValue(":statut",statut);
    q.bindValue(":date",date.toString(Qt::ISODate)); q.exec();
}

void Citernes::supprimerBesoinAchatBD(int id)
{
    QSqlQuery q;
    q.prepare("DELETE FROM BESOIN_ACHAT WHERE id=:id");
    q.bindValue(":id",id); q.exec();
}

void Citernes::mettreAJourStatutBesoinBD(int id,const QString& s)
{
    QSqlQuery q;
    q.prepare("UPDATE BESOIN_ACHAT SET statut=:s WHERE id=:id");
    q.bindValue(":s",s); q.bindValue(":id",id); q.exec();
}

void Citernes::ajouterBesoinAchat(const QString& typeHuile,double qte,double capa,int nb)
{
    for(const auto& b:m_listeBesoins)
        if(b.typeHuile==typeHuile&&std::abs(b.quantiteManquante-qte)<0.01&&b.statut=="En attente")
            return;

    QSqlQuery q; q.exec("SELECT BESOIN_ACHAT_SEQ.NEXTVAL FROM DUAL");
    int newId=1; if(q.next()) newId=q.value(0).toInt();
    QDateTime now=QDateTime::currentDateTime();
    sauvegarderBesoinAchat(newId,typeHuile,qte,capa,nb,"En attente",now);

    BesoinAchat b;
    b.id=newId; b.typeHuile=typeHuile; b.quantiteManquante=qte;
    b.capaciteSuggeree=capa; b.nombreCiternesRequises=nb;
    b.statut="En attente"; b.dateAjout=now;
    m_listeBesoins.prepend(b);
}

void Citernes::supprimerBesoinAchat(int id)
{
    supprimerBesoinAchatBD(id);
    for(int i=0;i<m_listeBesoins.size();++i)
        if(m_listeBesoins[i].id==id){m_listeBesoins.removeAt(i);break;}
}

void Citernes::mettreAJourStatutBesoin(int id,const QString& s)
{
    mettreAJourStatutBesoinBD(id,s);
    for(BesoinAchat& b:m_listeBesoins) if(b.id==id){b.statut=s;break;}
}

// ═════════════════════════════════════════════════════════════════════════════
// MÉTIER AVANCÉ 2 — REMPLISSAGE
// ═════════════════════════════════════════════════════════════════════════════

QList<Citernes::ResultatRemplissage>
Citernes::repartirQuantiteDansCiternesVides(const QString& typeHuile, double quantiteDemandee)
{
    QList<ResultatRemplissage> resultats;
    double qRest = quantiteDemandee;

    QSqlQuery query;
    query.prepare("SELECT ID_CITERNE,CAPACITEMAX FROM CITERNE "
                  "WHERE NIVEAUACTUEL=0 AND UPPER(ETAT_CITERNE)='VIDE' "
                  "AND UPPER(TYPEHUILE)=UPPER(:t) ORDER BY CAPACITEMAX DESC");
    query.bindValue(":t", typeHuile);
    if (!query.exec()) return resultats;

    double capTot=0;
    QList<QPair<int,double>> dispo;
    while(query.next()){
        dispo.append(qMakePair(query.value(0).toInt(),query.value(1).toDouble()));
        capTot+=query.value(1).toDouble();
    }

    if(dispo.isEmpty()||capTot<quantiteDemandee) return resultats;

    for(const auto& c:dispo){
        if(qRest<=0) break;
        ResultatRemplissage r;
        r.idCiterne=c.first; r.capaciteMax=c.second;
        if(qRest>=c.second){r.quantiteAttribuee=c.second;qRest-=c.second;}
        else               {r.quantiteAttribuee=qRest;   qRest=0;}
        resultats.append(r);
    }
    return resultats;
}

void Citernes::on_remplissageCi_clicked()
{
    QString typeHuile   = ui->saiTyphuile->currentText().trimmed();
    QString quantiteStr = ui->saiquanhuile->text().trimmed();

    if(typeHuile.isEmpty()){
        QMessageBox::warning(this,"Champ requis","Sélectionnez un type d'huile."); return;
    }
    if(quantiteStr.isEmpty()){
        QMessageBox::warning(this,"Champ requis","Saisissez la quantité."); return;
    }
    bool ok; double qDem=quantiteStr.toDouble(&ok);
    if(!ok||qDem<=0){QMessageBox::warning(this,"Erreur","Quantité invalide."); return;}

    QSqlQuery chk;
    chk.prepare("SELECT COUNT(*) FROM CITERNE WHERE UPPER(TYPEHUILE)=UPPER(:t)");
    chk.bindValue(":t",typeHuile);
    if(!chk.exec()||!chk.next()) return;
    if(chk.value(0).toInt()==0){
        QMessageBox::warning(this,"Erreur",QString("Aucune citerne %1").arg(typeHuile)); return;
    }

    QList<ResultatRemplissage> resultats = repartirQuantiteDansCiternesVides(typeHuile, qDem);

    if(resultats.isEmpty()){
        double capDispo=0;
        QSqlQuery qCap;
        qCap.prepare("SELECT NVL(SUM(CAPACITEMAX),0) FROM CITERNE WHERE NIVEAUACTUEL=0 "
                     "AND UPPER(ETAT_CITERNE)='VIDE' AND UPPER(TYPEHUILE)=UPPER(:t)");
        qCap.bindValue(":t",typeHuile);
        if(qCap.exec()&&qCap.next()) capDispo=qCap.value(0).toDouble();

        double qManq=qDem-capDispo; if(qManq<=0) qManq=qDem;
        double capSug=std::ceil(qManq/1000.0)*1000.0; if(capSug<500) capSug=500;
        int    nbReq=static_cast<int>(std::ceil(qManq/capSug)); if(nbReq<1) nbReq=1;
        ajouterBesoinAchat(typeHuile,qManq,capSug,nbReq);

        QMessageBox::warning(this,"Remplissage impossible",
                             QString("<b>⚠ Impossible : %1 L d'huile %2.</b><br>"
                                     "Capacité dispo : <b>%3 L</b><br>"
                                     "Manquante : <b style='color:#F44336;'>%4 L</b><br><br>"
                                     "✅ Besoin d'achat enregistré automatiquement.")
                                 .arg(qDem,0,'f',0).arg(typeHuile)
                                 .arg(capDispo,0,'f',0).arg(qManq,0,'f',0));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Résultat du remplissage");
    dialog.setMinimumSize(750,500); dialog.setModal(true);
    auto* mainLayout=new QVBoxLayout(&dialog);

    auto* hdr=new QLabel(QString(
                               "<h3 style='color:#4CAF50;'>✅ Remplissage planifié</h3>"
                               "<b>Type :</b> %1 | <b>Quantité :</b> %2 L | <b>Citernes :</b> %3")
                               .arg(typeHuile).arg(qDem).arg(resultats.size()));
    hdr->setWordWrap(true);
    hdr->setStyleSheet("background:#E8F5E9;padding:10px;border-radius:5px;");
    mainLayout->addWidget(hdr);

    auto* tbl=new QTableWidget();
    tbl->setColumnCount(5);
    tbl->setHorizontalHeaderLabels(
        {"ID Citerne","Type","Cap. Max (L)","Attribuée (L)","État Final"});
    tbl->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    double totAtt=0; int row=0;
    for(const auto& r:resultats){
        tbl->insertRow(row);
        tbl->setItem(row,0,new QTableWidgetItem(QString::number(r.idCiterne)));
        tbl->setItem(row,1,new QTableWidgetItem(typeHuile));
        tbl->setItem(row,2,new QTableWidgetItem(QString::number(r.capaciteMax,'f',0)));
        tbl->setItem(row,3,new QTableWidgetItem(QString::number(r.quantiteAttribuee,'f',0)));
        QString ef=(r.quantiteAttribuee>=r.capaciteMax)?"Pleine":"Partiellement remplie";
        QColor  ce=(r.quantiteAttribuee>=r.capaciteMax)?QColor("#C8E6C9"):QColor("#FFF9C4");
        auto*   ei=new QTableWidgetItem(ef); ei->setBackground(QBrush(ce));
        tbl->setItem(row,4,ei);
        totAtt+=r.quantiteAttribuee; row++;
    }
    mainLayout->addWidget(tbl);

    auto* pb=new QProgressBar();
    pb->setRange(0,100); pb->setValue(static_cast<int>(totAtt/qDem*100));
    pb->setFormat(QString("Remplissage : %1%").arg(totAtt/qDem*100,0,'f',1));
    pb->setStyleSheet(
        "QProgressBar{border:1px solid #BDBDBD;border-radius:5px;text-align:center;}"
        "QProgressBar::chunk{background:#4CAF50;border-radius:5px;}");
    mainLayout->addWidget(pb);

    auto* blay=new QHBoxLayout();
    auto* btnApp=new QPushButton("✅ Appliquer");
    btnApp->setStyleSheet("background:#4CAF50;color:white;padding:10px;font-weight:bold;border-radius:5px;");
    auto* btnAnn=new QPushButton("❌ Annuler");
    btnAnn->setStyleSheet("background:#F44336;color:white;padding:10px;font-weight:bold;border-radius:5px;");
    blay->addWidget(btnApp); blay->addWidget(btnAnn);
    mainLayout->addLayout(blay);

    connect(btnAnn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(btnApp, &QPushButton::clicked, this,
            [this,&dialog,resultats,typeHuile](){
                QSqlDatabase::database().transaction();
                bool ok=true;
                for(const auto& r:resultats){
                    QSqlQuery upd;
                    upd.prepare("UPDATE CITERNE SET NIVEAUACTUEL=NIVEAUACTUEL+:q,"
                                "ETAT_CITERNE=CASE WHEN(NIVEAUACTUEL+:q)>=CAPACITEMAX "
                                "THEN 'Pleine' ELSE 'En remplissage' END "
                                "WHERE ID_CITERNE=:id");
                    upd.bindValue(":q",  r.quantiteAttribuee);
                    upd.bindValue(":id", r.idCiterne);
                    if(!upd.exec()){ok=false;break;}
                }
                if(ok){
                    QSqlDatabase::database().commit();
                    QMessageBox::information(this,"Succès",
                                             QString("✅ Remplissage effectué !\nType : %1 — Citernes : %2")
                                                 .arg(typeHuile).arg(resultats.size()));
                    rafraichirListe(); dialog.accept();
                } else {
                    QSqlDatabase::database().rollback();
                    QMessageBox::critical(this,"Erreur","Échec mise à jour.");
                }
            });

    dialog.exec();
}

// ═════════════════════════════════════════════════════════════════════════════
// MÉTIER AVANCÉ 1 — LISTE DES ACHATS
// ═════════════════════════════════════════════════════════════════════════════

void Citernes::on_acheter_clicked() { afficherListeAchats(); }

void Citernes::afficherListeAchats()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Historique des citernes à acheter");
    dialog.setMinimumSize(950,600); dialog.setModal(true);
    auto* mainLayout=new QVBoxLayout(&dialog);

    auto* hdr=new QLabel("<h3 style='color:#1565C0;'>🛒 Historique des besoins d'achat</h3>");
    hdr->setStyleSheet(
        "background:#E3F2FD;padding:12px;border-radius:6px;border-left:4px solid #1565C0;");
    mainLayout->addWidget(hdr);

    auto* table=new QTableWidget();
    table->setColumnCount(8);
    table->setHorizontalHeaderLabels(
        {"ID","Type d'huile","Qté manquante (L)","Cap. suggérée (L)",
         "Nb citernes","Date","Statut","Actions"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setMinimumHeight(300);

    if(m_listeBesoins.isEmpty()){
        table->setRowCount(1); table->setRowHeight(0,60);
        auto* vide=new QTableWidgetItem("Aucun besoin enregistré.");
        vide->setTextAlignment(Qt::AlignCenter);
        vide->setForeground(QBrush(QColor("#9E9E9E")));
        table->setSpan(0,0,1,8); table->setItem(0,0,vide);
    } else {
        table->setRowCount(m_listeBesoins.size());
        for(int i=0;i<m_listeBesoins.size();++i){
            const auto& b=m_listeBesoins[i];
            table->setRowHeight(i,54);
            auto mk=[](const QString& t){
                auto* it=new QTableWidgetItem(t);
                it->setTextAlignment(Qt::AlignCenter|Qt::AlignVCenter);
                return it;
            };
            table->setItem(i,0,mk(QString::number(b.id)));
            table->setItem(i,1,mk(b.typeHuile));
            table->setItem(i,2,mk(QString::number(b.quantiteManquante,'f',0)+" L"));
            table->setItem(i,3,mk(QString::number(b.capaciteSuggeree,'f',0)+" L"));
            table->setItem(i,4,mk(QString::number(b.nombreCiternesRequises)));
            table->setItem(i,5,mk(b.dateAjout.toString("dd/MM/yyyy HH:mm")));

            auto* si=mk(b.statut);
            if(b.statut=="Commandé"){
                si->setForeground(QBrush(QColor("#2E7D32")));
                si->setBackground(QBrush(QColor("#E8F5E9")));
                QFont f=si->font(); f.setBold(true); si->setFont(f);
            } else {
                si->setForeground(QBrush(QColor("#E65100")));
                si->setBackground(QBrush(QColor("#FFF3E0")));
            }
            table->setItem(i,6,si);

            auto* cw=new QWidget(table);
            auto* lay=new QHBoxLayout(cw);
            lay->setContentsMargins(4,4,4,4); lay->setSpacing(6);

            auto* bc=new QPushButton("✔ Commandé");
            bc->setStyleSheet(
                "background:#4CAF50;color:white;border-radius:4px;padding:5px;font-size:11px;");
            bc->setFixedHeight(34); bc->setEnabled(b.statut!="Commandé");

            auto* bs2=new QPushButton("🗑 Supprimer");
            bs2->setStyleSheet(
                "background:#F44336;color:white;border-radius:4px;padding:5px;font-size:11px;");
            bs2->setFixedHeight(34);

            lay->addWidget(bc); lay->addWidget(bs2);
            lay->setAlignment(Qt::AlignCenter);
            table->setCellWidget(i,7,cw);

            int bid=b.id;
            connect(bc,  &QPushButton::clicked, this, [this,bid,&dialog](){
                mettreAJourStatutBesoin(bid,"Commandé");
                QMessageBox::information(this,"Commandé","✅ Commande marquée.");
                dialog.accept(); afficherListeAchats();
            });
            connect(bs2, &QPushButton::clicked, this, [this,bid,&dialog](){
                if(QMessageBox::question(this,"Confirmation","Supprimer ce besoin ?",
                                          QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes){
                    supprimerBesoinAchat(bid);
                    dialog.accept(); afficherListeAchats();
                }
            });
        }
    }
    mainLayout->addWidget(table);

    auto* bl=new QHBoxLayout();
    auto* btnExp=new QPushButton("📄 Exporter en PDF");
    btnExp->setStyleSheet(
        "background:#1565C0;color:white;padding:9px 18px;font-weight:bold;border-radius:5px;");
    btnExp->setEnabled(!m_listeBesoins.isEmpty());
    connect(btnExp, &QPushButton::clicked, this,
            [this](){ if(!m_listeBesoins.isEmpty()) exporterListeAchatsPDF(); });

    auto* btnF=new QPushButton("Fermer");
    btnF->setStyleSheet("background:#607D8B;color:white;padding:9px 18px;border-radius:5px;");
    connect(btnF, &QPushButton::clicked, &dialog, &QDialog::accept);

    bl->addWidget(btnExp); bl->addStretch(); bl->addWidget(btnF);
    mainLayout->addLayout(bl);
    dialog.exec();
}

void Citernes::exporterListeAchatsPDF()
{
    QString fileName=QFileDialog::getSaveFileName(
        this,"Exporter PDF",
        QString("SmartOil_Achats_%1.pdf")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "Fichiers PDF (*.pdf)");
    if(fileName.isEmpty()) return;

    QString html=
        "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
        "<style>body{font-family:'Segoe UI',Arial;margin:30px;color:#333;}"
        ".hdr{background:linear-gradient(135deg,#1565C0,#42A5F5);color:white;"
        "padding:24px;border-radius:10px;margin-bottom:24px;}"
        ".hdr h1{margin:0;font-size:22px;}"
        "table{width:100%;border-collapse:collapse;font-size:12px;}"
        "thead tr{background:#1565C0;color:white;}th{padding:9px;text-align:left;}"
        "tbody td{padding:8px;border-bottom:1px solid #BBDEFB;}"
        ".att{color:#E65100;font-weight:bold;}.cmd{color:#2E7D32;font-weight:bold;}"
        ".footer{margin-top:36px;font-size:10px;color:#aaa;text-align:center;}"
        "</style></head><body>"
        "<div class='hdr'><h1>🛒 Citernes à Acheter</h1><p>SmartOil · "
        + QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm")
        + "</p></div>"
          "<table><thead><tr><th>ID</th><th>Type</th><th>Qté manquante</th>"
          "<th>Cap. suggérée</th><th>Nb citernes</th><th>Date</th><th>Statut</th></tr></thead>"
          "<tbody>";

    for(const auto& b:m_listeBesoins)
        html += "<tr>"
                "<td>"+QString::number(b.id)+"</td>"
                                          "<td>"+b.typeHuile+"</td>"
                                "<td>"+QString::number(b.quantiteManquante,'f',0)+" L</td>"
                                                                 "<td>"+QString::number(b.capaciteSuggeree,'f',0)+" L</td>"
                                                                "<td>"+QString::number(b.nombreCiternesRequises)+"</td>"
                                                              "<td>"+b.dateAjout.toString("dd/MM/yyyy HH:mm")+"</td>"
                                                             "<td class='"+(b.statut=="Commandé"?"cmd":"att")+"'>"+b.statut+"</td>"
                                                                               "</tr>";

    html += "</tbody></table>"
            "<div class='footer'>SmartOil</div></body></html>";

    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(15,15,15,15), QPageLayout::Millimeter);

    QTextDocument doc; doc.setHtml(html); doc.print(&printer);
    QMessageBox::information(this,"Export réussi",QString("✅ PDF : %1").arg(fileName));
    statusBar()->showMessage("PDF : "+fileName, 5000);
}

