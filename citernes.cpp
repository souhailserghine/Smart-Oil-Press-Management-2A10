#include "citernes.h"
#include "ui_citernes.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>
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
#include <cmath>
#include <algorithm>
#include <QTimer>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QThread>
#include <QProgressBar>

// Qt Charts
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
    statusBar()->showMessage("Prêt");

    connect(ui->Trier, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ rafraichirListe(); });
    connect(ui->SaiRecherche, &QLineEdit::textChanged,
            this, &Citernes::on_Recherche_clicked);
    connect(ui->exporterListeCiterne, &QPushButton::clicked,
            this, &Citernes::on_exporterListeCiterne_clicked);
    connect(ui->StatistiqueCiterne, &QToolButton::clicked,
            this, &Citernes::on_StatistiqueCiterne_clicked);
    ui->X->setEnabled(true);
    ui->X->setVisible(true);
    connect(ui->X, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Citernes::on_X_currentIndexChanged);

    // Connexion des nouveaux boutons
    if (ui->mesure) {
        connect(ui->mesure, &QCommandLinkButton::clicked, this, &Citernes::on_mesure_clicked);
    }
    if (ui->remplissageCi) {
        connect(ui->remplissageCi, &QCommandLinkButton::clicked, this, &Citernes::on_remplissageCi_clicked);
    }

    // ── Validator chiffres + point décimal ────────────────────────────────
    QRegularExpression reDecimal("^\\d{0,10}(\\.\\d{0,4})?$");
    auto* validatorDecimal = new QRegularExpressionValidator(reDecimal, this);
    ui->SaiCapa->setValidator(validatorDecimal);
    ui->SaiNiv->setValidator(validatorDecimal);
    ui->SaiCapa->setPlaceholderText("Ex : 5000.00");
    ui->SaiNiv->setPlaceholderText("Ex : 2500.00");

    // ── Labels d'erreur ───────────────────────────────────────────────────
    m_errCapa = new QLabel("", ui->SaiCapa->parentWidget());
    m_errNiv  = new QLabel("", ui->SaiNiv->parentWidget());
    m_errEtat = new QLabel("", ui->SaiEtat->parentWidget());
    m_errCapa->setVisible(false);
    m_errNiv->setVisible(false);
    m_errEtat->setVisible(false);

    QFormLayout* fl = qobject_cast<QFormLayout*>(
        ui->SaiCapa->parentWidget()->layout());
    if (fl) {
        fl->insertRow(1, "", m_errCapa);
        fl->insertRow(3, "", m_errNiv);
        fl->addRow("", m_errEtat);
    }

    // ── Validation temps réel — Capacité ─────────────────────────────────
    connect(ui->SaiCapa, &QLineEdit::textChanged, this, [this](const QString& txt) {
        if (txt.isEmpty()) { setNeutre(ui->SaiCapa, m_errCapa); return; }
        bool ok=false; double val=txt.toDouble(&ok);
        if (!ok||val<=0)
            setErreur(ui->SaiCapa, m_errCapa, "La capacité doit être un nombre positif");
        else
            setOk(ui->SaiCapa, m_errCapa);
        if (!ui->SaiNiv->text().isEmpty())
            emit ui->SaiNiv->textChanged(ui->SaiNiv->text());
    });

    // ── Validation temps réel — Niveau ───────────────────────────────────
    connect(ui->SaiNiv, &QLineEdit::textChanged, this, [this](const QString& txt) {
        if (txt.isEmpty()) { setNeutre(ui->SaiNiv, m_errNiv); return; }
        bool okN=false, okC=false;
        double niv=txt.toDouble(&okN);
        double cap=ui->SaiCapa->text().toDouble(&okC);
        if (!okN||niv<0)
            setErreur(ui->SaiNiv, m_errNiv, "Le niveau doit être un nombre positif ou nul");
        else if (okC&&cap>0&&niv>cap)
            setErreur(ui->SaiNiv, m_errNiv,
                      QString("Le niveau ne peut pas dépasser la capacité (%1 L)")
                          .arg(QString::number(cap,'f',0)));
        else
            setOk(ui->SaiNiv, m_errNiv);
    });

    // ── Suggestion automatique état ───────────────────────────────────────
    connect(ui->SaiNiv, &QLineEdit::textChanged, this, [this](const QString& txt) {
        bool okN=false, okC=false;
        double niv=txt.toDouble(&okN);
        double cap=ui->SaiCapa->text().toDouble(&okC);
        if (!okN||!okC||cap<=0) return;
        double pct=niv/cap*100.0;
        if      (pct>=100.0) ui->SaiEtat->setCurrentText("Pleine");
        else if (pct==0.0)   ui->SaiEtat->setCurrentText("Vide");
        else if (pct<50.0)   ui->SaiEtat->setCurrentText("En remplissage");
        else                  ui->SaiEtat->setCurrentText("En utilisation");
    });

    // ── Validation temps réel — État ─────────────────────────────────────
    auto validerEtat = [this]() {
        bool okN=false, okC=false;
        double niv=ui->SaiNiv->text().toDouble(&okN);
        double cap=ui->SaiCapa->text().toDouble(&okC);
        if (!okN||!okC||cap<=0||
            ui->SaiNiv->text().isEmpty()||ui->SaiCapa->text().isEmpty()) {
            setNeutreCombo(ui->SaiEtat, m_errEtat); return;
        }
        double pct=niv/cap*100.0;
        QString etat=ui->SaiEtat->currentText();
        QString erreur;
        if (etat=="Pleine"&&pct<100.0)
            erreur=QString("\"Pleine\" nécessite niveau = capacité (actuellement %1%)")
                         .arg(QString::number(pct,'f',1));
        else if (etat=="Vide"&&pct>0.0)
            erreur=QString("\"Vide\" nécessite niveau = 0 (actuellement %1%)")
                         .arg(QString::number(pct,'f',1));
        if (!erreur.isEmpty())
            setErreurCombo(ui->SaiEtat, m_errEtat, erreur);
        else
            setOkCombo(ui->SaiEtat, m_errEtat);
    };
    connect(ui->SaiEtat, &QComboBox::currentTextChanged,
            this, [validerEtat](const QString&){ validerEtat(); });
    connect(ui->SaiNiv,  &QLineEdit::textChanged,
            this, [validerEtat](const QString&){ validerEtat(); });
    connect(ui->SaiCapa, &QLineEdit::textChanged,
            this, [validerEtat](const QString&){ validerEtat(); });

    chargerListeCiternes();

    // ── Tri par double-clic sur l'en-tête ─────────────────────────────────
    ui->ListeCiterne->horizontalHeader()->setSectionsClickable(true);
    connect(ui->ListeCiterne->horizontalHeader(), &QHeaderView::sectionDoubleClicked,
            this, [this](int col) {
                const QStringList sqlCol = {
                    "ID_CITERNE","CAPACITEMAX","NIVEAUACTUEL",
                    "TYPEHUILE","TEMPERATURE_CITERNE","ETAT_CITERNE",""
                };
                if (col<0||col>=sqlCol.size()||sqlCol[col].isEmpty()) return;
                QSqlQuery query;
                if (!query.exec(QString(
                                    "SELECT ID_CITERNE, CAPACITEMAX, NIVEAUACTUEL, TYPEHUILE,"
                                    "TEMPERATURE_CITERNE, ETAT_CITERNE FROM CITERNE ORDER BY %1 ASC")
                                    .arg(sqlCol[col]))) {
                    QMessageBox::critical(this,"Erreur SQL",query.lastError().text()); return;
                }
                ui->ListeCiterne->clearContents();
                ui->ListeCiterne->setRowCount(0);
                int row=0;
                while (query.next()) {
                    ui->ListeCiterne->insertRow(row);
                    for (int c=0;c<6;++c) {
                        QTableWidgetItem* item=new QTableWidgetItem(query.value(c).toString());
                        item->setFlags(item->flags()&~Qt::ItemIsEditable);
                        ui->ListeCiterne->setItem(row,c,item);
                    }
                    QWidget* cw=new QWidget(ui->ListeCiterne);
                    QHBoxLayout* lay=new QHBoxLayout(cw);
                    lay->setContentsMargins(2,2,2,2); lay->setSpacing(6);
                    QPushButton* bm=new QPushButton("Modifier",cw);
                    bm->setStyleSheet("background:#2196F3;color:white;border-radius:4px;padding:5px;");
                    bm->setFixedSize(70,30);
                    QPushButton* bs=new QPushButton("Supprimer",cw);
                    bs->setStyleSheet("background:#F44336;color:white;border-radius:4px;padding:5px;");
                    bs->setFixedSize(70,30);
                    lay->addWidget(bm); lay->addWidget(bs); lay->setAlignment(Qt::AlignCenter);
                    ui->ListeCiterne->setCellWidget(row,6,cw);
                    connect(bm,&QPushButton::clicked,this,[this,row]{onModifier(row);});
                    connect(bs,&QPushButton::clicked,this,[this,row]{onSupprimer(row);});
                    ui->ListeCiterne->setRowHeight(row,50); ++row;
                }
                for (int c=0;c<ui->ListeCiterne->columnCount();++c) {
                    QTableWidgetItem* h=ui->ListeCiterne->horizontalHeaderItem(c);
                    if (h) {
                        h->setBackground(c==col?QColor("#1565C0"):QColor());
                        h->setForeground(c==col?QColor("white"):QColor());
                    }
                }
                QString colName=ui->ListeCiterne->horizontalHeaderItem(col)
                                      ?ui->ListeCiterne->horizontalHeaderItem(col)->text():"";
                statusBar()->showMessage(QString("Trié par : %1").arg(colName),3000);
            });

    // Initialisation du timer pour les mesures périodiques
    m_timerMesures = new QTimer(this);
    connect(m_timerMesures, &QTimer::timeout, this, &Citernes::enregistrementPeriodiqueMesures);
    m_timerMesures->start(60000); // Toutes les minutes

    // Créer la table mesure si elle n'existe pas
    creerTableMesure();
}

Citernes::~Citernes()
{
    if (m_timerMesures) {
        m_timerMesures->stop();
        delete m_timerMesures;
    }
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS Qt Charts
// ─────────────────────────────────────────────────────────────────────────────

QList<CiterneData> Citernes::lireDonneesCiternes()
{
    QList<CiterneData> rows;
    QSqlQuery q;
    q.exec("SELECT ID_CITERNE, CAPACITEMAX, NIVEAUACTUEL, TYPEHUILE,"
           "TEMPERATURE_CITERNE, ETAT_CITERNE FROM CITERNE ORDER BY ID_CITERNE");
    while (q.next()) {
        CiterneData d;
        d.id=q.value(0).toInt(); d.capaciteMax=q.value(1).toDouble();
        d.niveauActuel=q.value(2).toDouble(); d.typeHuile=q.value(3).toString();
        d.temperature=q.value(4).toDouble(); d.etat=q.value(5).toString();
        rows.append(d);
    }
    return rows;
}

QChart* Citernes::makeChart(const QString &title)
{
    QChart* c = new QChart();
    c->setTitle(title);
    QFont titleFont("Segoe UI", 10, QFont::Bold);
    c->setTitleFont(titleFont);
    c->setTitleBrush(QBrush(Qt::black));
    c->setAnimationOptions(QChart::SeriesAnimations);
    c->setBackgroundVisible(false);
    QFont legendFont("Segoe UI", 8);
    c->legend()->setFont(legendFont);
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
    if (m_currentChart){m_currentChart->hide();m_currentChart->deleteLater();m_currentChart=nullptr;}
    if (!view) return;
    QRect r=ui->chartStatusContainer->rect();
    view->setGeometry(0,36,r.width(),r.height()-36);
    view->show(); view->raise();
    m_currentChart=view;
}

// ─────────────────────────────────────────────────────────────────────────────
// CHARGER / RAFRAÎCHIR
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::chargerListeCiternes()
{
    QString orderBy="ID_CITERNE";
    QString t=ui->Trier->currentText();
    if      (t=="Niveau Actuel")     orderBy="NIVEAUACTUEL";
    else if (t=="Type Huile")        orderBy="TYPEHUILE";
    else if (t=="Capacite Maximale") orderBy="CAPACITEMAX";
    else if (t=="Température")       orderBy="TEMPERATURE_CITERNE";
    else if (t=="Etat")              orderBy="ETAT_CITERNE";

    QSqlQuery query;
    if (!query.exec(QString("SELECT ID_CITERNE, CAPACITEMAX, NIVEAUACTUEL, TYPEHUILE,"
                            "TEMPERATURE_CITERNE, ETAT_CITERNE FROM CITERNE ORDER BY %1 ASC")
                        .arg(orderBy))) {
        QMessageBox::critical(this,"Erreur SQL",query.lastError().text()); return;
    }
    ui->ListeCiterne->clearContents();
    ui->ListeCiterne->setRowCount(0);
    ui->ListeCiterne->setColumnCount(7);
    ui->ListeCiterne->setHorizontalHeaderLabels(
        {"ID","Capacité Max","Niveau Actuel","Type Huile","Température","État","Action"});
    ui->ListeCiterne->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->ListeCiterne->horizontalHeader()->setSectionResizeMode(6,QHeaderView::ResizeToContents);
    ui->ListeCiterne->setAlternatingRowColors(true);
    ui->ListeCiterne->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->ListeCiterne->verticalHeader()->setVisible(false);

    int row=0;
    while (query.next()) {
        ui->ListeCiterne->insertRow(row);
        for (int col=0;col<6;++col) {
            QTableWidgetItem* item=new QTableWidgetItem(query.value(col).toString());
            item->setFlags(item->flags()&~Qt::ItemIsEditable);
            ui->ListeCiterne->setItem(row,col,item);
        }
        QWidget* cw=new QWidget(ui->ListeCiterne);
        QHBoxLayout* lay=new QHBoxLayout(cw);
        lay->setContentsMargins(2,2,2,2); lay->setSpacing(6);
        QPushButton* bm=new QPushButton("Modifier",cw);
        bm->setStyleSheet("background:#2196F3;color:white;border-radius:4px;padding:5px;");
        bm->setFixedSize(70,30);
        QPushButton* bs=new QPushButton("Supprimer",cw);
        bs->setStyleSheet("background:#F44336;color:white;border-radius:4px;padding:5px;");
        bs->setFixedSize(70,30);
        lay->addWidget(bm); lay->addWidget(bs); lay->setAlignment(Qt::AlignCenter);
        ui->ListeCiterne->setCellWidget(row,6,cw);
        connect(bm,&QPushButton::clicked,this,[this,row]{onModifier(row);});
        connect(bs,&QPushButton::clicked,this,[this,row]{onSupprimer(row);});
        ui->ListeCiterne->setRowHeight(row,50); ++row;
    }
    statusBar()->showMessage(QString("%1 citernes chargées").arg(row),3000);
}

void Citernes::rafraichirListe(){ chargerListeCiternes(); }

// ─────────────────────────────────────────────────────────────────────────────
// MODIFIER / SUPPRIMER
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::onModifier(int row)
{
    if (row<0||row>=ui->ListeCiterne->rowCount()) return;
    QString id=ui->ListeCiterne->item(row,0)->text();
    double  ca=ui->ListeCiterne->item(row,1)->text().toDouble();
    double  na=ui->ListeCiterne->item(row,2)->text().toDouble();
    QString ty=ui->ListeCiterne->item(row,3)->text();
    double  ta=ui->ListeCiterne->item(row,4)->text().toDouble();
    QString et=ui->ListeCiterne->item(row,5)->text();

    QDialog dlg(this);
    dlg.setWindowTitle(QString("Modifier la citerne %1").arg(id));
    dlg.setMinimumWidth(420);
    QFormLayout* form=new QFormLayout(&dlg);
    QLineEdit* editCap=new QLineEdit(QString::number(ca),&dlg);
    QLineEdit* editNiv=new QLineEdit(QString::number(na),&dlg);
    auto* v=new QRegularExpressionValidator(
        QRegularExpression("^\\d{0,10}(\\.\\d{0,4})?$"),&dlg);
    editCap->setValidator(v); editNiv->setValidator(v);
    QComboBox* editType=new QComboBox(&dlg);
    editType->addItems({"Olive","Raffinée","Bio","Alimentaire","Industrielle"});
    QDoubleSpinBox* editTemp=new QDoubleSpinBox(&dlg);
    editTemp->setRange(-50,150); editTemp->setDecimals(2);
    editTemp->setValue(ta); editTemp->setSuffix(" °C");
    QComboBox* editEtat=new QComboBox(&dlg);
    editEtat->addItems({"Vide","Pleine","En utilisation","En remplissage","En maintenance","Bloquee"});
    int ix=editType->findText(ty,Qt::MatchFixedString); if(ix>=0) editType->setCurrentIndex(ix);
    int ie=editEtat->findText(et,Qt::MatchFixedString); if(ie>=0) editEtat->setCurrentIndex(ie);
    form->addRow("Capacité maximale (L):",editCap);
    form->addRow("Niveau actuel (L):",editNiv);
    form->addRow("Type d'huile:",editType);
    form->addRow("Température:",editTemp);
    form->addRow("État:",editEtat);
    QDialogButtonBox* btns=new QDialogButtonBox(
        QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dlg);
    form->addRow(btns);
    connect(btns,&QDialogButtonBox::accepted,&dlg,&QDialog::accept);
    connect(btns,&QDialogButtonBox::rejected,&dlg,&QDialog::reject);
    if (dlg.exec()!=QDialog::Accepted) return;
    double newCap=editCap->text().toDouble();
    double newNiv=editNiv->text().toDouble();
    if (newCap<=0){QMessageBox::warning(this,"Erreur","La capacité doit être positive"); return;}
    if (newNiv<0||newNiv>newCap){QMessageBox::warning(this,"Erreur","Niveau invalide"); return;}
    QSqlQuery q;
    q.prepare("UPDATE CITERNE SET CAPACITEMAX=:cap, NIVEAUACTUEL=:niv, TYPEHUILE=:type,"
              "TEMPERATURE_CITERNE=:temp, ETAT_CITERNE=:etat WHERE ID_CITERNE=:id");
    q.bindValue(":cap",newCap); q.bindValue(":niv",newNiv);
    q.bindValue(":type",editType->currentText()); q.bindValue(":temp",editTemp->value());
    q.bindValue(":etat",editEtat->currentText()); q.bindValue(":id",id);
    if (!q.exec()) QMessageBox::critical(this,"Erreur SQL",q.lastError().text());
    else { QMessageBox::information(this,"Succès","Citerne modifiée avec succès"); rafraichirListe(); }
}

void Citernes::onSupprimer(int row)
{
    if (row < 0 || row >= ui->ListeCiterne->rowCount()) return;

    QString id = ui->ListeCiterne->item(row, 0)->text();

    // Compter les mesures liées pour informer l'utilisateur
    QSqlQuery qCount;
    qCount.prepare("SELECT COUNT(*) FROM MESURE WHERE IDCITERNE = :id");
    qCount.bindValue(":id", id);
    int nbMesures = 0;
    if (qCount.exec() && qCount.next()) {
        nbMesures = qCount.value(0).toInt();
    }

    // Message de confirmation adapté
    QString msg = (nbMesures > 0)
                      ? QString("Voulez-vous supprimer la citerne %1 ?\n\n"
                                "⚠ Attention : %2 mesure(s) associée(s) seront également supprimées.")
                            .arg(id).arg(nbMesures)
                      : QString("Voulez-vous supprimer la citerne %1 ?").arg(id);

    if (QMessageBox::question(this, "Confirmation", msg,
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    // Démarrer une transaction pour garantir la cohérence
    QSqlDatabase::database().transaction();

    // Étape 1 : Supprimer d'abord les mesures liées (table enfant)
    QSqlQuery qMesures;
    qMesures.prepare("DELETE FROM MESURE WHERE IDCITERNE = :id");
    qMesures.bindValue(":id", id);

    if (!qMesures.exec()) {
        QSqlDatabase::database().rollback();
        QMessageBox::critical(this, "Erreur SQL",
                              "Impossible de supprimer les mesures associées :\n"
                                  + qMesures.lastError().text());
        return;
    }

    // Étape 2 : Supprimer la citerne (table parent)
    QSqlQuery qCiterne;
    qCiterne.prepare("DELETE FROM CITERNE WHERE ID_CITERNE = :id");
    qCiterne.bindValue(":id", id);

    if (qCiterne.exec()) {
        QSqlDatabase::database().commit();
        ui->ListeCiterne->removeRow(row);
        QMessageBox::information(this, "Succès",
                                 QString("Citerne %1 supprimée avec succès.").arg(id));
        statusBar()->showMessage(QString("Citerne %1 supprimée").arg(id), 3000);
    } else {
        QSqlDatabase::database().rollback();
        QMessageBox::critical(this, "Erreur SQL", qCiterne.lastError().text());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AJOUTER
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::on_ConAjout_clicked()
{
    QString capStr=ui->SaiCapa->text().trimmed();
    QString nivStr=ui->SaiNiv->text().trimmed();
    QString type=ui->SaiTyp->currentText();
    double  temp=ui->SaiTem->value();
    QString etat=ui->SaiEtat->currentText();

    bool hasError=false;
    QStringList messages;

    // Contrôle 1 : Capacité
    bool okCap=false;
    double cap=capStr.toDouble(&okCap);
    if (capStr.isEmpty()) {
        setErreur(ui->SaiCapa,m_errCapa,"La capacité est obligatoire");
        messages<<"• La capacité maximale est obligatoire."; hasError=true;
    } else if (!okCap||cap<=0) {
        setErreur(ui->SaiCapa,m_errCapa,"La capacité doit être un nombre positif");
        messages<<"• La capacité maximale doit être un nombre positif."; hasError=true;
    } else { setOk(ui->SaiCapa,m_errCapa); }

    // Contrôle 2 : Niveau
    bool okNiv=false;
    double niv=nivStr.toDouble(&okNiv);
    if (nivStr.isEmpty()) {
        setErreur(ui->SaiNiv,m_errNiv,"Le niveau est obligatoire");
        messages<<"• Le niveau actuel est obligatoire."; hasError=true;
    } else if (!okNiv||niv<0) {
        setErreur(ui->SaiNiv,m_errNiv,"Le niveau doit être un nombre positif ou nul");
        messages<<"• Le niveau actuel doit être un nombre positif ou nul."; hasError=true;
    } else if (okCap&&cap>0&&niv>cap) {
        setErreur(ui->SaiNiv,m_errNiv,
                  QString("Le niveau (%1 L) dépasse la capacité (%2 L)")
                      .arg(QString::number(niv,'f',0)).arg(QString::number(cap,'f',0)));
        messages<<QString("• Le niveau (%1 L) ne peut pas dépasser la capacité (%2 L).").arg(niv).arg(cap);
        hasError=true;
    } else { setOk(ui->SaiNiv,m_errNiv); }

    if (hasError) {
        QMessageBox::warning(this,"Erreurs de saisie",
                             "Veuillez corriger les erreurs suivantes :\n\n"+messages.join("\n"));
        return;
    }

    // Contrôle 3 : Température
    Seuils s=getSeuilsTemp(type);
    if (temp>=s.danger) {
        QMessageBox::critical(this,"Température dangereuse",
                              QString("<b>⛔ DANGER : température %1°C !</b><br><br>"
                                      "Type d'huile : <b>%2</b><br>"
                                      "Plage optimale : <b>%3°C – %4°C</b><br>"
                                      "Seuil de danger : <b style='color:#F44336;'>%5°C</b><br><br>"
                                      "L'ajout est <b>bloqué</b>.")
                                  .arg(temp).arg(type).arg(s.minOk).arg(s.maxOk).arg(s.danger));
        return;
    } else if (temp>s.maxOk) {
        auto rep=QMessageBox::warning(this,"Température élevée",
                                        QString("<b>⚠ Température %1°C — au-dessus de la plage optimale</b><br><br>"
                                                "Type d'huile : <b>%2</b><br>"
                                                "Plage optimale : <b>%3°C – %4°C</b><br>"
                                                "Seuil de danger : <b style='color:#F44336;'>%5°C</b><br><br>"
                                                "Voulez-vous quand même enregistrer ?")
                                            .arg(temp).arg(type).arg(s.minOk).arg(s.maxOk).arg(s.danger),
                                        QMessageBox::Yes|QMessageBox::No);
        if (rep==QMessageBox::No) return;
    } else if (temp<s.minOk) {
        auto rep=QMessageBox::warning(this,"Température basse",
                                        QString("<b>❄ Température %1°C — en dessous de la plage optimale</b><br><br>"
                                                "Type d'huile : <b>%2</b><br>"
                                                "Plage optimale : <b>%3°C – %4°C</b><br>"
                                                "Seuil de danger : <b style='color:#F44336;'>%5°C</b><br><br>"
                                                "Voulez-vous quand même enregistrer ?")
                                            .arg(temp).arg(type).arg(s.minOk).arg(s.maxOk).arg(s.danger),
                                        QMessageBox::Yes|QMessageBox::No);
        if (rep==QMessageBox::No) return;
    }

    // Contrôle 4 : Cohérence État / Niveau
    double pct=niv/cap*100.0;
    bool etatIncoherent=false;
    QString suggestionEtat;
    if (etat=="Pleine"&&pct<100.0) {
        etatIncoherent=true;
        suggestionEtat=(pct==0.0)?"Vide":(pct<50.0)?"En remplissage":"En utilisation";
    } else if (etat=="Vide"&&pct>0.0) {
        etatIncoherent=true;
        suggestionEtat=(pct>=100.0)?"Pleine":(pct>=50.0)?"En utilisation":"En remplissage";
    }
    if (etatIncoherent) {
        auto rep=QMessageBox::question(this,"État incohérent",
                                         QString("L'état <b>\"%1\"</b> semble incohérent "
                                                 "avec le niveau <b>%2%</b> (%3 L / %4 L).<br><br>"
                                                 "État suggéré : <b>\"%5\"</b><br><br>"
                                                 "Voulez-vous utiliser l'état suggéré ?")
                                             .arg(etat).arg(QString::number(pct,'f',1))
                                             .arg(QString::number(niv,'f',0)).arg(QString::number(cap,'f',0))
                                             .arg(suggestionEtat),
                                         QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
        if (rep==QMessageBox::Cancel) return;
        if (rep==QMessageBox::Yes) {
            etat=suggestionEtat;
            ui->SaiEtat->setCurrentText(etat);
            setOkCombo(ui->SaiEtat,m_errEtat);
        }
    }

    // INSERTION POUR ORACLE - L'ID est géré par la séquence et le trigger
    QSqlQuery q;
    q.prepare(
        "INSERT INTO CITERNE(CAPACITEMAX, NIVEAUACTUEL, TYPEHUILE, "
        "                    TEMPERATURE_CITERNE, ETAT_CITERNE) "
        "VALUES(:cap, :niv, :type, :temp, :etat)"
        );
    q.bindValue(":cap",  cap);
    q.bindValue(":niv",  niv);
    q.bindValue(":type", type);
    q.bindValue(":temp", temp);
    q.bindValue(":etat", etat);

    if (q.exec()) {
        QMessageBox::information(this,"Succès","✅ Citerne ajoutée avec succès !");
        ui->SaiCapa->clear(); ui->SaiNiv->clear();
        ui->SaiTyp->setCurrentIndex(0); ui->SaiTem->setValue(20.0); ui->SaiEtat->setCurrentIndex(0);
        setNeutre(ui->SaiCapa,m_errCapa); setNeutre(ui->SaiNiv,m_errNiv);
        setNeutreCombo(ui->SaiEtat,m_errEtat);
        rafraichirListe();
        ui->metiersCiternes->setCurrentWidget(ui->consulterciterne);
        statusBar()->showMessage("Citerne ajoutée avec succès",3000);
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
    ui->SaiTyp->setCurrentIndex(0); ui->SaiTem->setValue(20.0); ui->SaiEtat->setCurrentIndex(0);
    setNeutre(ui->SaiCapa,m_errCapa); setNeutre(ui->SaiNiv,m_errNiv);
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
    QString r=ui->SaiRecherche->text().trimmed();
    if (r.isEmpty()){rafraichirListe();return;}
    bool isNum=false; r.toInt(&isNum);
    QString cond=isNum
                       ?QString("TO_CHAR(ID_CITERNE) LIKE '%%%1%'").arg(r)
                       :QString("UPPER(TYPEHUILE) LIKE UPPER('%%%1%')").arg(r);
    QSqlQuery query;
    if (!query.exec(QString("SELECT ID_CITERNE, CAPACITEMAX, NIVEAUACTUEL, TYPEHUILE,"
                            "TEMPERATURE_CITERNE, ETAT_CITERNE FROM CITERNE WHERE %1 "
                            "ORDER BY ID_CITERNE").arg(cond))) {
        QMessageBox::critical(this,"Erreur SQL",query.lastError().text()); return;
    }
    ui->ListeCiterne->clearContents(); ui->ListeCiterne->setRowCount(0);
    int row=0;
    while (query.next()) {
        ui->ListeCiterne->insertRow(row);
        for (int col=0;col<6;++col) {
            QTableWidgetItem* item=new QTableWidgetItem(query.value(col).toString());
            item->setFlags(item->flags()&~Qt::ItemIsEditable);
            if (isNum&&col==0){item->setBackground(QBrush(QColor("#FFF9C4")));item->setForeground(QBrush(QColor("#E65100")));}
            else if (!isNum&&col==3){item->setBackground(QBrush(QColor("#E8F5E9")));item->setForeground(QBrush(QColor("#2E7D32")));}
            ui->ListeCiterne->setItem(row,col,item);
        }
        QWidget* cw=new QWidget(ui->ListeCiterne);
        QHBoxLayout* lay=new QHBoxLayout(cw);
        lay->setContentsMargins(2,2,2,2); lay->setSpacing(6);
        QPushButton* bm=new QPushButton("Modifier",cw);
        bm->setStyleSheet("background:#2196F3;color:white;border-radius:4px;padding:5px;");
        bm->setFixedSize(70,30);
        QPushButton* bs=new QPushButton("Supprimer",cw);
        bs->setStyleSheet("background:#F44336;color:white;border-radius:4px;padding:5px;");
        bs->setFixedSize(70,30);
        lay->addWidget(bm); lay->addWidget(bs); lay->setAlignment(Qt::AlignCenter);
        ui->ListeCiterne->setCellWidget(row,6,cw);
        connect(bm,&QPushButton::clicked,this,[this,row]{onModifier(row);});
        connect(bs,&QPushButton::clicked,this,[this,row]{onSupprimer(row);});
        ui->ListeCiterne->setRowHeight(row,50); ++row;
    }
    statusBar()->showMessage(row==0
                                 ?QString("Aucune citerne pour : \"%1\"").arg(r)
                                 :QString("%1 citerne(s) pour \"%2\"").arg(row).arg(r),4000);
}

// ─────────────────────────────────────────────────────────────────────────────
// EXPORT PDF
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::on_exporterListeCiterne_clicked()
{
    QString fileName=QFileDialog::getSaveFileName(this,"Exporter en PDF",
                                                    QString("SmartOil_Citernes_%1.pdf")
                                                        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
                                                    "Fichiers PDF (*.pdf)");
    if (fileName.isEmpty()) return;
    QSqlQuery qs;
    int tot=0; double capT=0,nivT=0,tmpM=0;
    qs.exec("SELECT COUNT(*),SUM(CAPACITEMAX),SUM(NIVEAUACTUEL),AVG(TEMPERATURE_CITERNE) FROM CITERNE");
    if (qs.next()){tot=qs.value(0).toInt();capT=qs.value(1).toDouble();
        nivT=qs.value(2).toDouble();tmpM=qs.value(3).toDouble();}
    double tauxO=(capT>0)?(nivT/capT*100.0):0.0;
    QMap<QString,int> cEtat,cType;
    qs.exec("SELECT ETAT_CITERNE,COUNT(*) FROM CITERNE GROUP BY ETAT_CITERNE");
    while(qs.next()) cEtat[qs.value(0).toString()]=qs.value(1).toInt();
    qs.exec("SELECT TYPEHUILE,COUNT(*) FROM CITERNE GROUP BY TYPEHUILE");
    while(qs.next()) cType[qs.value(0).toString()]=qs.value(1).toInt();

    QString html="<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                   "<style>body{font-family:'Segoe UI',Arial,sans-serif;margin:30px;color:#333;}"
                   ".hdr{background:linear-gradient(135deg,#1565C0,#42A5F5);color:white;padding:24px;"
                   "border-radius:10px;margin-bottom:20px;}.hdr h1{margin:0;font-size:24px;}"
                   ".kpi{display:flex;gap:14px;margin-bottom:20px;}"
                   ".k{flex:1;background:#f5f9ff;border-left:5px solid #2196F3;padding:12px;border-radius:8px;}"
                   ".k .v{font-size:20px;font-weight:bold;color:#1565C0;}.k .l{font-size:10px;color:#777;}"
                   "h2{color:#1565C0;border-bottom:2px solid #e3f2fd;padding-bottom:4px;font-size:14px;}"
                   "table{width:100%;border-collapse:collapse;font-size:12px;}"
                   "thead tr{background:#1565C0;color:white;}th{padding:8px;text-align:left;}"
                   "tbody tr:nth-child(even){background:#f5f9ff;}td{padding:7px;border-bottom:1px solid #e0e0e0;}"
                   ".bar{height:12px;background:#e0e0e0;border-radius:6px;overflow:hidden;}"
                   ".bf{height:100%;background:#2196F3;border-radius:6px;}"
                   ".footer{margin-top:30px;font-size:10px;color:#aaa;text-align:center;}"
                   "</style></head><body>";
    html+="<div class='hdr'><h1>Rapport — Gestion des Citernes</h1><p>SmartOil · Généré le "
            +QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm")+"</p></div>";
    html+="<div class='kpi'>"
            +QString("<div class='k'><div class='v'>%1</div><div class='l'>Citernes</div></div>").arg(tot)
            +QString("<div class='k'><div class='v'>%1 L</div><div class='l'>Capacité</div></div>").arg(QString::number(capT,'f',0))
            +QString("<div class='k'><div class='v'>%1 L</div><div class='l'>Volume actuel</div></div>").arg(QString::number(nivT,'f',0))
            +QString("<div class='k'><div class='v'>%1%</div><div class='l'>Taux</div></div>").arg(QString::number(tauxO,'f',1))
            +QString("<div class='k'><div class='v'>%1°C</div><div class='l'>Temp. moy.</div></div>").arg(QString::number(tmpM,'f',1))
            +"</div>";
    html+="<h2>Répartition par état</h2><tr><thead>汽<th>État</th><th>Nb</th><th>Distribution</th></tr></thead><tbody>";
    for (auto it=cEtat.constBegin();it!=cEtat.constEnd();++it){int p=(tot>0)?(it.value()*100/tot):0;
        html+="<tr><td>"+it.key()+"</td><td>"+QString::number(it.value())+"</td>"
                                                                                    "<td><div class='bar'><div class='bf' style='width:"+QString::number(p)+"%'></div></div>"+QString::number(p)+"%</td></tr>";}
    html+="</tbody></table>";
    html+="<h2>Répartition par type d'huile</h2><tr><thead>汽<th>Type</th><th>Nb</th><th>Distribution</th></tr></thead><tbody>";
    for (auto it=cType.constBegin();it!=cType.constEnd();++it){int p=(tot>0)?(it.value()*100/tot):0;
        html+="<tr><td>"+it.key()+"</td><td>"+QString::number(it.value())+"</td>"
                                                                                    "<td><div class='bar'><div class='bf' style='width:"+QString::number(p)+"%;background:#4CAF50;'></div></div>"+QString::number(p)+"%</td></tr>";}
    html+="</tbody></table>";
    html+="<h2>Liste détaillée</h2><table><thead>汽<th>ID</th><th>Cap. max</th><th>Niveau</th>"
            "<th>Occup.%</th><th>Type</th><th>Temp.</th><th>État</th></tr></thead><tbody>";
    QSqlQuery qd("SELECT ID_CITERNE, CAPACITEMAX, NIVEAUACTUEL, TYPEHUILE,"
                 "TEMPERATURE_CITERNE, ETAT_CITERNE FROM CITERNE ORDER BY ID_CITERNE");
    while(qd.next()){double c=qd.value(1).toDouble(),n=qd.value(2).toDouble();
        double o=(c>0)?(n/c*100.0):0.0;
        QString co=(o>90)?"color:#E53935;font-weight:bold;":(o>70)?"color:#FB8C00;":"color:#43A047;";
        html+="<tr>"
                "<td>"+qd.value(0).toString()+"</td>"
                                           "<td>"+QString::number(c,'f',0)+"</td>"
                                               "<td>"+QString::number(n,'f',0)+"</td>"
                                               "<td style='"+co+"'>"+QString::number(o,'f',1)+"%</td>"
                                                           "<td>"+qd.value(3).toString()+"</td>"
                                           "<td>"+qd.value(4).toString()+"</td>"
                                           "<td>"+qd.value(5).toString()+"</td>"
                                           "</tr>";
    }
    html+="</tbody></table><div class='footer'>SmartOil — Document généré automatiquement.</div></body></html>";
    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat); printer.setOutputFileName(fileName);
    printer.setPageSize(QPageSize(QPageSize::A4)); printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(15,15,15,15),QPageLayout::Millimeter);
    QTextDocument doc; doc.setHtml(html); doc.print(&printer);
    QMessageBox::information(this,"Export réussi",QString("PDF exporté !\n%1").arg(fileName));
    statusBar()->showMessage("PDF exporté : "+fileName,5000);
}

// ═════════════════════════════════════════════════════════════════════════════
//  STATISTIQUES
// ═════════════════════════════════════════════════════════════════════════════

void Citernes::afficherStatistiques()
{
    QList<CiterneData> rows=lireDonneesCiternes();
    if (rows.isEmpty()){afficherGraphique(nullptr);return;}

    const QList<QColor> pal={QColor("#2196F3"),QColor("#4CAF50"),QColor("#FF9800"),
                               QColor("#F44336"),QColor("#9C27B0"),QColor("#00BCD4"),QColor("#795548"),QColor("#607D8B")};

    QString choix=ui->X->currentText();
    QChartView* view=nullptr;

    if (choix.contains("part")||choix.contains("huile")) {
        QMap<QString,int> tc;
        for (const auto& d:rows) tc[d.typeHuile]++;
        QPieSeries* series=new QPieSeries(); int ci=0;
        for (auto it=tc.constBegin();it!=tc.constEnd();++it,++ci) {
            double pct=100.0*it.value()/rows.size();
            QPieSlice* sl=series->append(
                QString("%1\n%2 (%3%)").arg(it.key()).arg(it.value())
                    .arg(QString::number(pct,'f',1)), it.value());
            sl->setColor(pal[ci%pal.size()]);
            sl->setLabelVisible(true);
            sl->setLabelFont(QFont("Segoe UI",8));
            sl->setLabelColor(Qt::black);
        }
        series->setHoleSize(0.38);
        QChart* chart=makeChart("Répartition des types d'huile");
        chart->addSeries(series);
        chart->legend()->setAlignment(Qt::AlignRight);
        view=makeChartView(chart);
    }
    else if (choix.contains("emp")) {
        double tMin=999,tMax=-999,tSum=0;
        for (const auto& d:rows){
            tMin=qMin(tMin,d.temperature); tMax=qMax(tMax,d.temperature); tSum+=d.temperature;
        }
        double tMoy=tSum/rows.size();

        QBarSet* setTemp=new QBarSet("Température (°C)");
        setTemp->setColor(QColor("#2196F3"));
        setTemp->setLabelColor(Qt::black);
        QStringList cats;
        for (const auto& d:rows){ *setTemp<<d.temperature; cats<<QString("#%1").arg(d.id); }

        QBarSeries* series=new QBarSeries(); series->append(setTemp);
        series->setLabelsVisible(true);
        series->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);

        QLineSeries* lineMoy=new QLineSeries();
        lineMoy->setName(QString("Moy. %1 °C").arg(QString::number(tMoy,'f',1)));
        QPen pen(QColor("#F44336")); pen.setStyle(Qt::DashLine); pen.setWidth(2);
        lineMoy->setPen(pen);
        for (int i=0;i<rows.size();++i) lineMoy->append(i,tMoy);

        QBarCategoryAxis* axX=new QBarCategoryAxis();
        axX->append(cats); axX->setTitleText("Citernes");
        axX->setLabelsColor(Qt::black);
        axX->setTitleBrush(QBrush(Qt::black));

        QValueAxis* axY=new QValueAxis();
        axY->setTitleText("Température (°C)"); axY->setLabelFormat("%.1f");
        axY->setRange(qMax(0.0,tMin-5),tMax+5);
        axY->setLabelsColor(Qt::black);
        axY->setTitleBrush(QBrush(Qt::black));

        QValueAxis* axX2=new QValueAxis();
        axX2->setRange(-0.5,rows.size()-0.5); axX2->setVisible(false);

        QChart* chart=makeChart("Températures des citernes");
        chart->addSeries(series); chart->addSeries(lineMoy);
        chart->addAxis(axX,Qt::AlignBottom); chart->addAxis(axY,Qt::AlignLeft);
        chart->addAxis(axX2,Qt::AlignBottom);
        series->attachAxis(axX); series->attachAxis(axY);
        lineMoy->attachAxis(axX2); lineMoy->attachAxis(axY);
        chart->legend()->setAlignment(Qt::AlignBottom);
        view=makeChartView(chart);
    }
    else if (choix.contains("tat")) {
        QMap<QString,int> ec;
        for (const auto& d:rows) ec[d.etat]++;
        QMap<QString,QColor> coul={
            {"Vide",QColor("#90A4AE")},{"Pleine",QColor("#4CAF50")},
            {"En utilisation",QColor("#FF9800")},{"En remplissage",QColor("#2196F3")},
            {"En maintenance",QColor("#F44336")},{"Bloquee",QColor("#795548")}
        };
        QPieSeries* series=new QPieSeries();
        for (auto it=ec.constBegin();it!=ec.constEnd();++it) {
            double pct=100.0*it.value()/rows.size();
            QPieSlice* sl=series->append(
                QString("%1\n%2 (%3%)").arg(it.key()).arg(it.value())
                    .arg(QString::number(pct,'f',1)), it.value());
            sl->setColor(coul.value(it.key(),QColor("#9C27B0")));
            sl->setLabelVisible(true);
            sl->setLabelFont(QFont("Segoe UI",8));
            sl->setLabelColor(Qt::black);
        }
        series->setHoleSize(0.40);
        QChart* chart=makeChart("État des citernes");
        chart->addSeries(series);
        chart->legend()->setAlignment(Qt::AlignRight);
        view=makeChartView(chart);
    }
    else if (choix.contains("aux")) {
        double totCap=0,totNiv=0;
        for (const auto& d:rows){ totCap+=d.capaciteMax; totNiv+=d.niveauActuel; }
        double taux=(totCap>0)?(totNiv/totCap*100.0):0.0;
        QColor cTaux=(taux>85)?QColor("#F44336"):(taux>60)?QColor("#FF9800"):QColor("#4CAF50");

        QBarSet* setR=new QBarSet("Rempli"); *setR<<taux;
        setR->setColor(cTaux); setR->setLabelColor(Qt::black);
        QBarSet* setL=new QBarSet("Libre");  *setL<<100.0-taux;
        setL->setColor(QColor("#E0E0E0")); setL->setLabelColor(Qt::black);

        QHorizontalStackedBarSeries* series=new QHorizontalStackedBarSeries();
        series->append(setR); series->append(setL);
        series->setLabelsVisible(true);
        series->setLabelsPosition(QAbstractBarSeries::LabelsCenter);

        QBarCategoryAxis* axY=new QBarCategoryAxis();
        axY->append(QStringList()<<"Système global");
        axY->setLabelsColor(Qt::black);
        axY->setTitleBrush(QBrush(Qt::black));

        QValueAxis* axX=new QValueAxis();
        axX->setRange(0,100); axX->setLabelFormat("%.0f %%");
        axX->setTitleText("Taux de remplissage (%)"); axX->setTickCount(6);
        axX->setLabelsColor(Qt::black);
        axX->setTitleBrush(QBrush(Qt::black));

        QChart* chart=makeChart(
            QString("Taux de remplissage global : %1 %  —  %2 L / %3 L")
                .arg(QString::number(taux,'f',1))
                .arg(QString::number(totNiv,'f',0))
                .arg(QString::number(totCap,'f',0)));
        chart->addSeries(series);
        chart->addAxis(axY,Qt::AlignLeft); chart->addAxis(axX,Qt::AlignBottom);
        series->attachAxis(axY); series->attachAxis(axX);
        chart->legend()->setAlignment(Qt::AlignBottom);
        view=makeChartView(chart,200);
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
    if (ui->metiersCiternes->currentWidget()==ui->statCiterne) afficherStatistiques();
}

// ─────────────────────────────────────────────────────────────────────────────
// STUBS
// ─────────────────────────────────────────────────────────────────────────────

QLabel* Citernes::creerGraphiqueBarres(const QStringList&,const QList<double>&,const QString&,const QString&,double){return nullptr;}
QLabel* Citernes::creerGraphiqueCirculaire(const QStringList&,const QList<double>&,const QString&){return nullptr;}
QString Citernes::debutGraphique(const QString&,const QString&){return {};}
QString Citernes::finGraphique(){return {};}
QLabel* Citernes::creerLabelGraphique(const QString&){return nullptr;}
void    Citernes::remplacerContenuAvecAnimation(QLabel*){}
void    Citernes::afficherMessageErreur(const QString&){}
void    Citernes::afficherMessageInfo(const QString&){}
QLabel* Citernes::creerStatistiquesHuile(const QList<QString>&){return nullptr;}
QLabel* Citernes::creerStatistiquesTypeHuile(const QList<QString>&){return nullptr;}
QLabel* Citernes::creerStatistiquesEtat(const QList<QString>&){return nullptr;}
QLabel* Citernes::creerStatistiquesCapacite(const QList<QString>&){return nullptr;}
QLabel* Citernes::creerStatistiquesNiveau(const QList<QString>&){return nullptr;}
QLabel* Citernes::creerStatistiquesTemperature(const QList<QString>&){return nullptr;}
QLabel* Citernes::creerStatistiquesComparatives(const QList<QString>&){return nullptr;}

// ═════════════════════════════════════════════════════════════════════════════
//  MESURES ET ALERTES AVANCÉES
// ═════════════════════════════════════════════════════════════════════════════

void Citernes::creerTableMesure()
{
    QSqlQuery query;
    query.exec("SELECT table_name FROM user_tables WHERE table_name='MESURE'");

    if (!query.next()) {
        QString createTable =
            "CREATE TABLE MESURE ("
            "idMesure NUMBER PRIMARY KEY, "
            "idCiterne NUMBER NOT NULL, "
            "dateMesure DATE NOT NULL, "
            "niveauActuel NUMBER NOT NULL, "
            "temperature NUMBER NOT NULL, "
            "CONSTRAINT FK_MESURE_CITERNE FOREIGN KEY(idCiterne) REFERENCES CITERNE(ID_CITERNE)"
            ")";

        if (!query.exec(createTable)) {
            qDebug() << "Erreur création table mesure:" << query.lastError().text();
        } else {
            query.exec("CREATE SEQUENCE seq_mesure START WITH 1 INCREMENT BY 1");
        }
    }
}

void Citernes::enregistrementPeriodiqueMesures()
{
    QSqlQuery query;
    query.exec("SELECT ID_CITERNE, NIVEAUACTUEL, TEMPERATURE_CITERNE FROM CITERNE");

    while (query.next()) {
        int idCiterne = query.value(0).toInt();
        double niveau = query.value(1).toDouble();
        double temperature = query.value(2).toDouble();
        insererMesure(idCiterne, niveau, temperature);
    }
    statusBar()->showMessage("Mesures périodiques enregistrées", 3000);
}

void Citernes::insererMesure(int idCiterne, double niveau, double temperature)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO MESURE (idMesure, idCiterne, dateMesure, niveauActuel, temperature) "
        "VALUES (seq_mesure.NEXTVAL, :idCiterne, SYSDATE, :niveau, :temperature)"
        );

    query.bindValue(":idCiterne", idCiterne);
    query.bindValue(":niveau", niveau);
    query.bindValue(":temperature", temperature);

    if (query.exec()) {
        QSqlQuery typeQuery;
        typeQuery.prepare("SELECT TYPEHUILE FROM CITERNE WHERE ID_CITERNE = :id");
        typeQuery.bindValue(":id", idCiterne);
        QString typeHuile = "";
        if (typeQuery.exec() && typeQuery.next()) {
            typeHuile = typeQuery.value(0).toString();
        }
        verifierSeuilsEtAlerter(idCiterne, niveau, temperature, typeHuile);
    }
}

void Citernes::verifierSeuilsEtAlerter(int idCiterne, double niveau, double temperature, const QString& typeHuile)
{
    QSqlQuery query;
    query.prepare("SELECT CAPACITEMAX, TYPEHUILE FROM CITERNE WHERE ID_CITERNE = :id");
    query.bindValue(":id", idCiterne);

    if (!query.exec() || !query.next()) return;

    double capaciteMax = query.value(0).toDouble();
    QString typeHuileReel = typeHuile.isEmpty() ? query.value(1).toString() : typeHuile;

    verifierNiveauCritique(idCiterne, niveau, capaciteMax);
    verifierTemperatureAnormale(idCiterne, temperature, typeHuileReel);

    QDateTime dateActuelle = QDateTime::currentDateTime();
    verifierFuite(idCiterne, niveau, dateActuelle);

    MesurePrecedente mesure;
    mesure.idCiterne = idCiterne;
    mesure.niveau = niveau;
    mesure.temperature = temperature;
    mesure.date = dateActuelle;
    mesure.existe = true;
    m_dernieresMesures[idCiterne] = mesure;
}

// ─────────────────────────────────────────────────────────────────────────────
// ALERTE NIVEAU CRITIQUE
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::verifierNiveauCritique(int idCiterne, double niveau, double capaciteMax)
{
    double pourcentageNiveau = (niveau / capaciteMax) * 100.0;

    if (pourcentageNiveau > 99.5) {
        QString message = QString(
                              "<h3 style='color:#F44336;'>🚨 ALERTE DEBORDEMENT - Citerne %1</h3>"
                              "<br>"
                              "<table style='width:100%;'>"
                              "<tr><td><b>Niveau actuel :</b></td><td style='color:#F44336;font-weight:bold;'>%2 L</b></td></tr>"
                              "<tr><td><b>Capacité max :</b></td><td>%3 L</b></td></tr>"
                              "<tr><td><b>Taux de remplissage :</b></td><td style='color:#F44336;font-weight:bold;'>%4%</b></td></tr>"
                              "<tr><td><b>Seuil critique :</b></td><td>&gt; 99.5%</b></td></tr>"
                              "</table>"
                              "<br>"
                              "<b>⚠️ Action requise :</b> Arrêter immédiatement le remplissage et vérifier la citerne !")
                              .arg(idCiterne)
                              .arg(niveau, 0, 'f', 2)
                              .arg(capaciteMax, 0, 'f', 2)
                              .arg(pourcentageNiveau, 0, 'f', 2);

        QMessageBox::critical(this, "Alerte Débordement", message);
        statusBar()->showMessage(QString("⚠️ DEBORDEMENT - Citerne %1 à %2%").arg(idCiterne).arg(pourcentageNiveau, 0, 'f', 1), 10000);
        QApplication::beep();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ALERTE TEMPÉRATURE ANORMALE
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::verifierTemperatureAnormale(int idCiterne, double temperature, const QString& typeHuile)
{
    Seuils s = getSeuilsTemp(typeHuile);

    if (temperature > s.maxOk) {
        QString niveauAlerte = (temperature >= s.danger) ? "CRITIQUE" : "ÉLEVÉE";
        QColor couleur = (temperature >= s.danger) ? "#F44336" : "#FF9800";
        QString icone = (temperature >= s.danger) ? "⛔🔥" : "⚠️🔥";

        QString message = QString(
                              "<h3 style='color:%2;'>%1 ALERTE SURCHAUFFE - Citerne %3</h3>"
                              "<br>"
                              "<table style='width:100%;'>"
                              "<tr><td><b>Température actuelle :</b></td><td style='color:%2;font-weight:bold;'>%4 °C</b></td></tr>"
                              "<tr><td><b>Type d'huile :</b></td><td>%5</b></td></tr>"
                              "<tr><td><b>Plage optimale :</b></td><td>%6 °C - %7 °C</b></td></tr>"
                              "<tr><td><b>Seuil de danger :</b></td><td style='color:#F44336;'>%8 °C</b></td></tr>"
                              "<tr><td><b>Niveau d'alerte :</b></td><td style='color:%2;font-weight:bold;'>%9</b></td></tr>"
                              "</table>"
                              "<br>"
                              "<b>⚠️ Action requise :</b> %10")
                              .arg(icone)
                              .arg(couleur.name())
                              .arg(idCiterne)
                              .arg(temperature, 0, 'f', 1)
                              .arg(typeHuile)
                              .arg(s.minOk, 0, 'f', 1)
                              .arg(s.maxOk, 0, 'f', 1)
                              .arg(s.danger, 0, 'f', 1)
                              .arg(niveauAlerte)
                              .arg((temperature >= s.danger) ? "ARRÊT IMMÉDIAT ! Refroidir la citerne d'urgence." : "Surveiller la température. Activer le système de refroidissement.");

        if (temperature >= s.danger) {
            QMessageBox::critical(this, "Alerte Surchauffe Critique", message);
        } else {
            QMessageBox::warning(this, "Alerte Surchauffe", message);
        }

        statusBar()->showMessage(QString("%1 SURCHAUFFE - Citerne %2 à %3°C").arg(icone).arg(idCiterne).arg(temperature, 0, 'f', 1), 8000);
        QApplication::beep();
    }
    else if (temperature < s.minOk) {
        QString message = QString(
                              "<h3 style='color:#2196F3;'>❄️ ALERTE BASSE TEMPERATURE - Citerne %1</h3>"
                              "<br>"
                              "<table style='width:100%;'>"
                              "<tr><td><b>Température actuelle :</b></td><td style='color:#2196F3;font-weight:bold;'>%2 °C</b></td></tr>"
                              "<tr><td><b>Type d'huile :</b></td><td>%3</b></td></tr>"
                              "<tr><td><b>Plage optimale :</b></td><td>%4 °C - %5 °C</b></td></tr>"
                              "<tr><td><b>Température minimale :</b></td><td style='color:#2196F3;font-weight:bold;'>%6 °C</b></td></tr>"
                              "</table>"
                              "<br>"
                              "<b>⚠️ Action requise :</b> Activer le système de chauffage pour ramener l'huile à température optimale.")
                              .arg(idCiterne)
                              .arg(temperature, 0, 'f', 1)
                              .arg(typeHuile)
                              .arg(s.minOk, 0, 'f', 1)
                              .arg(s.maxOk, 0, 'f', 1)
                              .arg(s.minOk, 0, 'f', 1);

        QMessageBox::warning(this, "Alerte Basse Température", message);
        statusBar()->showMessage(QString("❄️ BASSE TEMPÉRATURE - Citerne %1 à %2°C").arg(idCiterne).arg(temperature, 0, 'f', 1), 8000);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ALERTE FUITE
// ─────────────────────────────────────────────────────────────────────────────

void Citernes::verifierFuite(int idCiterne, double niveauActuel, const QDateTime& dateActuelle)
{
    if (!m_dernieresMesures.contains(idCiterne) || !m_dernieresMesures[idCiterne].existe) {
        return;
    }

    MesurePrecedente& derniere = m_dernieresMesures[idCiterne];
    double differenceNiveau = derniere.niveau - niveauActuel;

    if (differenceNiveau > 0) {
        qint64 minutesEcoulees = derniere.date.secsTo(dateActuelle) / 60;

        if (minutesEcoulees > 0) {
            double tauxBaisse = differenceNiveau / minutesEcoulees;

            bool fuiteProbable = (tauxBaisse > 10.0) || (differenceNiveau > 100.0 && minutesEcoulees <= 5);

            if (fuiteProbable) {
                QSqlQuery checkOperation;
                checkOperation.prepare(
                    "SELECT COUNT(*) FROM OPERATIONS "
                    "WHERE ID_CITERNE = :id AND DATE_OPERATION > SYSDATE - 1/24 "
                    "AND TYPE_OPERATION = 'SORTIE'"
                    );
                checkOperation.bindValue(":id", idCiterne);

                bool operationSortie = false;
                if (checkOperation.exec() && checkOperation.next()) {
                    operationSortie = (checkOperation.value(0).toInt() > 0);
                }

                if (!operationSortie) {
                    QString message = QString(
                                          "<h3 style='color:#F44336;'>💧💧💧 ALERTE FUITE - Citerne %1</h3>"
                                          "<br>"
                                          "<table style='width:100%;'>"
                                          "tr Jans<b>Niveau précédent :</b></td><td>%2 L</b></td></tr>"
                                          "tr Jans<b>Niveau actuel :</b></td><td style='color:#F44336;font-weight:bold;'>%3 L</b></td></tr>"
                                          "tr Jans<b>Perte constatée :</b></td><td style='color:#F44336;font-weight:bold;'>%4 L</b></td></tr>"
                                          "tr Jans<b>Période :</b></td><td>%5 minutes</b></td></tr>"
                                          "tr Jans<b>Taux de baisse :</b></td><td>%6 L/min</b></td></tr>"
                                          "tr Jans<b>Dernière mesure :</b></td><td>%7</b></td></tr>"
                                          "</table>"
                                          "<br>"
                                          "<b>🚨 ACTION URGENTE REQUISE :</b><br>"
                                          "• Couper immédiatement les arrivées<br>"
                                          "• Vérifier les vannes et joints<br>"
                                          "• Contenir la fuite<br>"
                                          "• Contacter la maintenance d'urgence")
                                          .arg(idCiterne)
                                          .arg(derniere.niveau, 0, 'f', 2)
                                          .arg(niveauActuel, 0, 'f', 2)
                                          .arg(differenceNiveau, 0, 'f', 2)
                                          .arg(minutesEcoulees)
                                          .arg(tauxBaisse, 0, 'f', 2)
                                          .arg(derniere.date.toString("HH:mm:ss"));

                    QMessageBox::critical(this, "⚠️ ALERTE FUITE DÉTECTÉE ⚠️", message);
                    statusBar()->showMessage(QString("💧 FUITE DÉTECTÉE - Citerne %1 - Perte de %2 L").arg(idCiterne).arg(differenceNiveau, 0, 'f', 1), 15000);

                    for (int i = 0; i < 3; i++) {
                        QApplication::beep();
                        QThread::msleep(200);
                    }
                }
            }
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  POPUP SUIVI DES MESURES
// ═════════════════════════════════════════════════════════════════════════════

void Citernes::on_mesure_clicked()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Suivi des mesures avec alertes");
    dialog.setMinimumSize(1100, 750);

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    QLabel* infoLabel = new QLabel(
        "<h3 style='color:#2196F3;'>📊 Suivi périodique des mesures avec Alertes Avancées</h3>"
        "<p>Les mesures sont enregistrées automatiquement toutes les minutes.</p>"
        "<br>"
        "<h4 style='color:#F44336;'>🚨 Types d'alertes :</h4>"
        "<table style='width:100%;'>"
        "<tr style='background:#FFEBEE;'><td><b>💧 Fuite</b></td><td>Baisse anormale du niveau sans opération de sortie</b></td><td style='color:#F44336;'>&gt; 10L/min ou &gt;100L/5min</b></td></tr>"
        "<tr style='background:#FFF3E0;'><td><b>🔥 Surchauffe</b></b></td><td>Température &gt; plage optimale</b></td><td style='color:#FF9800;'>Critique si &gt; seuil danger</b></td></tr>"
        "<tr style='background:#E3F2FD;'><td><b>❄️ Basse température</b></b></td><td>Température &lt; plage optimale</b></td><td style='color:#2196F3;'>Risque de solidification</b></td></tr>"
        "<tr style='background:#FFEBEE;'><td><b>⚠️ Débordement</b></b></td><td>Niveau &gt; 99.5%</b></td><td style='color:#F44336;'>Risque de débordement immédiat</b></td></tr>"
        "<tr style='background:#FFF9C4;'><td><b>📉 Niveau critique bas</b></b></td><td>Niveau &lt; 20%</b></td><td style='color:#FF9800;'>Risque de vide</b></td></tr>"
        "</table>"
        );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("background:#E8EAF6; padding:10px; border-radius:5px;");
    mainLayout->addWidget(infoLabel);

    QLabel* tableTitle = new QLabel("<h4>📋 Historique des mesures (30 derniers jours)</h4>");
    mainLayout->addWidget(tableTitle);

    QTableWidget* tableMesures = new QTableWidget();
    tableMesures->setColumnCount(7);
    tableMesures->setHorizontalHeaderLabels({"ID Mesure", "ID Citerne", "Date", "Niveau (L)", "Température (°C)", "État Niveau", "État Température"});
    tableMesures->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QSqlQuery query;
    query.exec("SELECT m.idMesure, m.idCiterne, TO_CHAR(m.dateMesure, 'DD/MM/YYYY HH24:MI:SS'), "
               "m.niveauActuel, m.temperature, c.CAPACITEMAX, c.TYPEHUILE FROM MESURE m "
               "JOIN CITERNE c ON m.idCiterne = c.ID_CITERNE "
               "WHERE m.dateMesure >= SYSDATE - 30 "
               "ORDER BY m.dateMesure DESC "
               "FETCH FIRST 100 ROWS ONLY");

    int row = 0;
    while (query.next()) {
        tableMesures->insertRow(row);
        tableMesures->setItem(row, 0, new QTableWidgetItem(query.value(0).toString()));
        tableMesures->setItem(row, 1, new QTableWidgetItem(query.value(1).toString()));
        tableMesures->setItem(row, 2, new QTableWidgetItem(query.value(2).toString()));

        double niveau = query.value(3).toDouble();
        double temperature = query.value(4).toDouble();
        double capacite = query.value(5).toDouble();
        QString typeHuile = query.value(6).toString();

        tableMesures->setItem(row, 3, new QTableWidgetItem(QString::number(niveau, 'f', 2)));
        tableMesures->setItem(row, 4, new QTableWidgetItem(QString::number(temperature, 'f', 2)));

        double pct = (niveau / capacite) * 100;
        QString etatNiveau;
        QColor couleurNiveau;
        if (pct > 99.5) {
            etatNiveau = "⚠️ DÉBORDEMENT!";
            couleurNiveau = QColor("#F44336");
        } else if (pct < 20) {
            etatNiveau = "📉 CRITIQUE BAS";
            couleurNiveau = QColor("#FF9800");
        } else {
            etatNiveau = "✓ Normal";
            couleurNiveau = QColor("#4CAF50");
        }

        QTableWidgetItem* niveauItem = new QTableWidgetItem(etatNiveau);
        niveauItem->setForeground(QBrush(couleurNiveau));
        tableMesures->setItem(row, 5, niveauItem);

        Seuils s = getSeuilsTemp(typeHuile);
        QString etatTemp;
        QColor couleurTemp;
        if (temperature >= s.danger) {
            etatTemp = "⛔ SURCHAUFFE CRITIQUE!";
            couleurTemp = QColor("#F44336");
        } else if (temperature > s.maxOk) {
            etatTemp = "🔥 SURCHAUFFE";
            couleurTemp = QColor("#FF9800");
        } else if (temperature < s.minOk) {
            etatTemp = "❄️ BASSE TEMP";
            couleurTemp = QColor("#2196F3");
        } else {
            etatTemp = "✓ Normal";
            couleurTemp = QColor("#4CAF50");
        }

        QTableWidgetItem* tempItem = new QTableWidgetItem(etatTemp);
        tempItem->setForeground(QBrush(couleurTemp));
        tableMesures->setItem(row, 6, tempItem);

        if (pct > 99.5 || pct < 20 || temperature >= s.danger || temperature > s.maxOk || temperature < s.minOk) {
            for (int col = 0; col < 7; col++) {
                if (tableMesures->item(row, col)) {
                    tableMesures->item(row, col)->setBackground(QBrush(QColor("#FFEBEE")));
                }
            }
        }

        row++;
    }

    mainLayout->addWidget(tableMesures);

    QPushButton* forceMeasureBtn = new QPushButton("Forcer une mesure immédiate");
    forceMeasureBtn->setStyleSheet("background:#4CAF50; color:white; padding:8px;");
    connect(forceMeasureBtn, &QPushButton::clicked, this, &Citernes::enregistrementPeriodiqueMesures);
    mainLayout->addWidget(forceMeasureBtn);

    QPushButton* closeBtn = new QPushButton("Fermer");
    closeBtn->setStyleSheet("background:#2196F3; color:white; padding:8px;");
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    mainLayout->addWidget(closeBtn);

    dialog.exec();
}

// ═════════════════════════════════════════════════════════════════════════════
//  REMPLISSAGE DES CITERNES VIDES (MODIFIÉ)
// ═════════════════════════════════════════════════════════════════════════════

QList<Citernes::ResultatRemplissage> Citernes::repartirQuantiteDansCiternesVides(const QString& typeHuile, double quantiteDemandee)
{
    QList<ResultatRemplissage> resultats;
    double quantiteRestante = quantiteDemandee;

    // Récupérer les citernes vides (niveauActuel = 0 ET etat_citerne = 'Vide') avec le bon type d'huile
    QSqlQuery query;
    query.prepare("SELECT ID_CITERNE, CAPACITEMAX FROM CITERNE "
                  "WHERE NIVEAUACTUEL = 0 AND UPPER(ETAT_CITERNE) = 'VIDE' AND UPPER(TYPEHUILE) = UPPER(:typeHuile) "
                  "ORDER BY CAPACITEMAX DESC");
    query.bindValue(":typeHuile", typeHuile);

    if (!query.exec()) {
        QMessageBox::critical(nullptr, "Erreur", "Erreur lors de la récupération des citernes: " + query.lastError().text());
        return resultats;
    }

    double capaciteTotale = 0;
    QList<QPair<int, double>> citernesDisponibles;

    while (query.next()) {
        int id = query.value(0).toInt();
        double capacite = query.value(1).toDouble();
        citernesDisponibles.append(qMakePair(id, capacite));
        capaciteTotale += capacite;
    }

    if (citernesDisponibles.isEmpty()) {
        QMessageBox::warning(nullptr, "Impossible",
                             QString("Aucune citerne vide disponible pour le type d'huile: %1").arg(typeHuile));
        return resultats;
    }

    if (capaciteTotale < quantiteDemandee) {
        QMessageBox::warning(nullptr, "Impossible",
                             QString("Capacité totale insuffisante!\n"
                                     "Quantité demandée: %1 L\n"
                                     "Capacité disponible: %2 L\n"
                                     "Manque: %3 L\n\n"
                                     "Citernes disponibles pour %4 : %5")
                                 .arg(quantiteDemandee)
                                 .arg(capaciteTotale)
                                 .arg(quantiteDemandee - capaciteTotale)
                                 .arg(typeHuile)
                                 .arg(citernesDisponibles.size()));
        return resultats;
    }

    // Répartir la quantité
    for (const auto& citerne : citernesDisponibles) {
        if (quantiteRestante <= 0) break;

        ResultatRemplissage result;
        result.idCiterne = citerne.first;
        result.capaciteMax = citerne.second;

        if (quantiteRestante >= citerne.second) {
            result.quantiteAttribuee = citerne.second;
            quantiteRestante -= citerne.second;
        } else {
            result.quantiteAttribuee = quantiteRestante;
            quantiteRestante = 0;
        }
        resultats.append(result);
    }
    return resultats;
}

void Citernes::on_remplissageCi_clicked()
{
    QString typeHuile = ui->saiTyphuile->currentText().trimmed();
    QString quantiteStr = ui->saiquanhuile->text().trimmed();

    // Validation des entrées
    if (typeHuile.isEmpty()) {
        QMessageBox::warning(this, "Champ requis", "Veuillez sélectionner un type d'huile.");
        return;
    }

    if (quantiteStr.isEmpty()) {
        QMessageBox::warning(this, "Champ requis", "Veuillez saisir la quantité d'huile.");
        return;
    }

    bool ok;
    double quantiteDemandee = quantiteStr.toDouble(&ok);
    if (!ok || quantiteDemandee <= 0) {
        QMessageBox::warning(this, "Erreur", "La quantité doit être un nombre positif valide.");
        return;
    }

    // Vérifier qu'il existe au moins une citerne avec ce type d'huile
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM CITERNE WHERE UPPER(TYPEHUILE) = UPPER(:typeHuile)");
    checkQuery.bindValue(":typeHuile", typeHuile);

    if (!checkQuery.exec() || !checkQuery.next()) {
        QMessageBox::warning(this, "Erreur", QString("Erreur lors de la vérification du type d'huile: %1").arg(typeHuile));
        return;
    }

    int nbCiternes = checkQuery.value(0).toInt();
    if (nbCiternes == 0) {
        QMessageBox::warning(this, "Erreur", QString("Aucune citerne trouvée avec le type d'huile: %1").arg(typeHuile));
        return;
    }

    // Effectuer la répartition
    QList<ResultatRemplissage> resultats = repartirQuantiteDansCiternesVides(typeHuile, quantiteDemandee);

    if (resultats.isEmpty()) {
        return; // Erreur déjà affichée dans la fonction
    }

    // Afficher les résultats dans une popup
    QDialog dialog(this);
    dialog.setWindowTitle("Résultat du remplissage");
    dialog.setMinimumSize(750, 550);

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    // En-tête avec résumé
    QLabel* headerLabel = new QLabel(
        QString("<h3 style='color:#4CAF50;'>✅ Remplissage planifié</h3>"
                "<table style='width:100%;'>"
                "tr Jans<b>Type d'huile :</b></b></td><td style='color:#2196F3;font-weight:bold;'>%1</b></td></tr>"
                "tr Jans<b>Quantité totale à stocker :</b></b></td><td>%2 L</b></td></tr>"
                "tr Jans<b>Citernes vides disponibles :</b></b></td><td>%3</b></td></tr>"
                "tr Jans<b>Citernes utilisées :</b></b></td><td style='color:#4CAF50;font-weight:bold;'>%4</b></td></tr>"
                "</table>")
            .arg(typeHuile)
            .arg(quantiteDemandee)
            .arg(resultats.size())
            .arg(resultats.size())
        );
    headerLabel->setWordWrap(true);
    headerLabel->setStyleSheet("background:#E8F5E9; padding:10px; border-radius:5px;");
    mainLayout->addWidget(headerLabel);

    // Tableau des résultats
    QLabel* tableTitle = new QLabel("<h4>📋 Détail du remplissage par citerne</h4>");
    mainLayout->addWidget(tableTitle);

    QTableWidget* tableResultats = new QTableWidget();
    tableResultats->setColumnCount(5);
    tableResultats->setHorizontalHeaderLabels({"ID Citerne", "Type Huile", "Capacité Max (L)", "Quantité Attribuée (L)", "État Final"});
    tableResultats->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    double totalAttribue = 0;
    int row = 0;
    for (const auto& result : resultats) {
        tableResultats->insertRow(row);
        tableResultats->setItem(row, 0, new QTableWidgetItem(QString::number(result.idCiterne)));
        tableResultats->setItem(row, 1, new QTableWidgetItem(typeHuile));
        tableResultats->setItem(row, 2, new QTableWidgetItem(QString::number(result.capaciteMax, 'f', 0)));
        tableResultats->setItem(row, 3, new QTableWidgetItem(QString::number(result.quantiteAttribuee, 'f', 0)));

        QString etatFinal;
        QColor couleurEtat;
        if (result.quantiteAttribuee >= result.capaciteMax) {
            etatFinal = "Pleine";
            couleurEtat = QColor("#C8E6C9");
        } else {
            etatFinal = "Partiellement remplie";
            couleurEtat = QColor("#FFF9C4");
        }

        QTableWidgetItem* etatItem = new QTableWidgetItem(etatFinal);
        etatItem->setBackground(QBrush(couleurEtat));
        tableResultats->setItem(row, 4, etatItem);

        totalAttribue += result.quantiteAttribuee;
        row++;
    }

    mainLayout->addWidget(tableResultats);

    // Barre de progression visuelle du remplissage
    double pourcentageUtilise = (totalAttribue / quantiteDemandee) * 100;

    QLabel* progressLabel = new QLabel(
        QString("<hr>"
                "<table style='width:100%;'>"
                "tr Jans<b>Total attribué :</b></b></td><td style='color:#4CAF50;font-weight:bold;'>%1 L</b></b>"
                "<td style='width:20px;'></td>"
                "<td><b>Pourcentage utilisé :</b></td><td style='color:#2196F3;font-weight:bold;'>%2%</b></td>"
                "</table>")
            .arg(totalAttribue)
            .arg(pourcentageUtilise, 0, 'f', 1)
        );
    mainLayout->addWidget(progressLabel);

    // Barre de progression
    QProgressBar* progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(static_cast<int>(pourcentageUtilise));
    progressBar->setFormat(QString("Remplissage : %1%").arg(pourcentageUtilise, 0, 'f', 1));
    progressBar->setStyleSheet(
        "QProgressBar {"
        "    border: 1px solid #BDBDBD;"
        "    border-radius: 5px;"
        "    text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #4CAF50;"
        "    border-radius: 5px;"
        "}"
        );
    mainLayout->addWidget(progressBar);

    // Boutons d'action
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    QPushButton* appliquerBtn = new QPushButton("✅ Appliquer le remplissage");
    appliquerBtn->setStyleSheet("background:#4CAF50; color:white; padding:10px; font-weight:bold; border-radius:5px;");

    QPushButton* annulerBtn = new QPushButton("❌ Annuler");
    annulerBtn->setStyleSheet("background:#F44336; color:white; padding:10px; font-weight:bold; border-radius:5px;");

    buttonLayout->addWidget(appliquerBtn);
    buttonLayout->addWidget(annulerBtn);
    mainLayout->addLayout(buttonLayout);

    // Connecter les boutons
    connect(annulerBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(appliquerBtn, &QPushButton::clicked, [this, &dialog, resultats, typeHuile]() {
        QSqlDatabase::database().transaction();
        bool succes = true;

        for (const auto& result : resultats) {
            QSqlQuery updateQuery;
            updateQuery.prepare("UPDATE CITERNE SET "
                                "NIVEAUACTUEL = NIVEAUACTUEL + :quantite, "
                                "ETAT_CITERNE = CASE "
                                "    WHEN (NIVEAUACTUEL + :quantite) >= CAPACITEMAX THEN 'Pleine' "
                                "    ELSE 'En remplissage' "
                                "END "
                                "WHERE ID_CITERNE = :id");
            updateQuery.bindValue(":quantite", result.quantiteAttribuee);
            updateQuery.bindValue(":id", result.idCiterne);

            if (!updateQuery.exec()) {
                succes = false;
                qDebug() << "Erreur mise à jour citerne" << result.idCiterne << ":" << updateQuery.lastError().text();
                break;
            }
        }

        if (succes) {
            QSqlDatabase::database().commit();

            // Message récapitulatif
            QString recap = QString(
                                "<h3 style='color:#4CAF50;'>✅ Remplissage effectué avec succès !</h3>"
                                "<br>"
                                "<table style='width:100%;'>"
                                "tr Jans<b>Type d'huile :</b></b></td><td>%1</b></td></tr>"
                                "tr Jans<b>Quantité totale stockée :</b></b></td><td>%2 L</b></td></tr>"
                                "tr Jans<b>Citernes utilisées :</b></b></td><td>%3</b></td></tr>"
                                "</td>")
                                .arg(typeHuile)
                                .arg(resultats.size())
                                .arg(resultats.size());

            QMessageBox::information(this, "Succès", recap);
            rafraichirListe();
            dialog.accept();
        } else {
            QSqlDatabase::database().rollback();
            QMessageBox::critical(this, "Erreur", "Erreur lors de la mise à jour des citernes.\nL'opération a été annulée.");
        }
    });

    dialog.exec();
}
