#include "agriculteurmodule.h"
#include "agriculteur.h"

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QAbstractItemView>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDebug>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QScrollArea>
#include <QFrame>
#include <QStringList>
#include <QSqlError>
#include <QSqlQuery>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <algorithm>

namespace {
QLabel* smallError(QWidget *parent)
{
    auto *label = new QLabel(parent);
    label->setStyleSheet(QStringLiteral("color:#c62828; font-size:10px; font-weight:600;"));
    label->setWordWrap(true);
    return label;
}

QLineEdit* line(const QString &placeholder, QWidget *parent)
{
    auto *edit = new QLineEdit(parent);
    edit->setPlaceholderText(placeholder);
    edit->setMinimumHeight(34);
    return edit;
}

QWidget* card(QWidget *parent)
{
    auto *w = new QWidget(parent);
    w->setObjectName(QStringLiteral("agriCard"));
    w->setStyleSheet(QStringLiteral(
        "QWidget#agriCard{background:#ffffff;border:1px solid #dfe8d4;border-radius:18px;}"));
    return w;
}

QLabel* title(const QString &text, QWidget *parent)
{
    auto *l = new QLabel(text, parent);
    l->setStyleSheet(QStringLiteral("font-size:20px;font-weight:900;color:#263414;"));
    return l;
}
}

AgriculteurModule::AgriculteurModule(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("embeddedAgriculteurModule"));
    buildUi();

    QRegularExpression rxNom(QStringLiteral("^[A-Za-zÀ-ÿ '\\-]+$"));
    auto *nameValidator = new QRegularExpressionValidator(rxNom, this);
    if (m_nom) m_nom->setValidator(nameValidator);
    if (m_prenom) m_prenom->setValidator(nameValidator);
    if (m_region) m_region->setValidator(nameValidator);
    if (m_mNom) m_mNom->setValidator(nameValidator);
    if (m_mPrenom) m_mPrenom->setValidator(nameValidator);
    if (m_mRegion) m_mRegion->setValidator(nameValidator);

    QRegularExpression rxNum(QStringLiteral("^[0-9]+$"));
    auto *numValidator = new QRegularExpressionValidator(rxNum, this);
    if (m_numero) m_numero->setValidator(numValidator);
    if (m_mNumero) m_mNumero->setValidator(numValidator);

    if (m_mNbArbre) m_mNbArbre->setReadOnly(true);
    if (m_mTypeOlive) m_mTypeOlive->setReadOnly(true);

    setupFloatingSubmitButton();
    ensureAgriculteurSupportTables();
    refreshAll();
    showConsultPage();
}


void AgriculteurModule::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateFloatingSubmitButton();
}

void AgriculteurModule::setupFloatingSubmitButton()
{
    if (m_floatingSubmitButton)
        return;

    m_floatingSubmitButton = new QPushButton(tr("Ajouter l'agriculteur"), this);
    m_floatingSubmitButton->setObjectName(QStringLiteral("agriFloatingSubmitButton"));
    m_floatingSubmitButton->setCursor(Qt::PointingHandCursor);
    m_floatingSubmitButton->setMinimumSize(240, 46);
    m_floatingSubmitButton->setStyleSheet(QStringLiteral(
        "QPushButton#agriFloatingSubmitButton {"
        " background:#667c3a; color:white; border:1px solid #566d2d;"
        " border-radius:16px; padding:10px 22px; font-weight:900; font-size:13px;"
        "}"
        "QPushButton#agriFloatingSubmitButton:hover { background:#566d2d; }"
        "QPushButton#agriFloatingSubmitButton:pressed { background:#455a23; }"
    ));
    connect(m_floatingSubmitButton, &QPushButton::clicked,
            this, &AgriculteurModule::addAgriculteur);

    connect(m_stack, &QStackedWidget::currentChanged, this, [this]() {
        updateFloatingSubmitButton();
    });

    updateFloatingSubmitButton();
}

void AgriculteurModule::updateFloatingSubmitButton()
{
    if (!m_floatingSubmitButton || !m_stack)
        return;

    const bool onAddPage = (m_stack->currentWidget() == m_addPage);
    m_floatingSubmitButton->setVisible(onAddPage);
    if (!onAddPage)
        return;

    const int w = qMax(240, m_floatingSubmitButton->sizeHint().width() + 22);
    const int h = 46;
    const int margin = 28;

    // Hard overlay: independent from the page/card layouts, so it cannot be
    // clipped by the Agriculteur form or by the embedded custom widget area.
    const int x = qMax(margin, width() - w - margin);
    const int y = qMax(margin, height() - h - margin);
    m_floatingSubmitButton->setGeometry(x, y, w, h);
    m_floatingSubmitButton->raise();
}

void AgriculteurModule::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_stack = new QStackedWidget(this);
    root->addWidget(m_stack, 1);

    buildAddPage();
    buildConsultPage();
    buildEditPage();
    buildHistoryPage();
    buildHistoryEditPage();
    buildStatsPage();
    buildDetectionPage();
    buildPredictionPage();
    buildAdvancedPage();
}

void AgriculteurModule::buildAddPage()
{
    auto *page = new QWidget(m_stack);
    page->setObjectName(QStringLiteral("agriAddPage"));
    m_addPage = page;

    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(24, 18, 24, 18);
    outer->setSpacing(12);

    auto *headerRow = new QHBoxLayout();
    headerRow->setContentsMargins(0, 0, 0, 0);
    headerRow->setSpacing(12);
    headerRow->addWidget(title(tr("Ajouter un agriculteur"), page), 1, Qt::AlignVCenter);

    auto *topSubmit = new QPushButton(tr("Ajouter l'agriculteur"), page);
    topSubmit->setObjectName(QStringLiteral("agriAddSubmitButton"));
    topSubmit->setProperty("type", "primary");
    topSubmit->setMinimumSize(210, 42);
    topSubmit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(topSubmit, &QPushButton::clicked, this, &AgriculteurModule::addAgriculteur);
    headerRow->addWidget(topSubmit, 0, Qt::AlignRight | Qt::AlignVCenter);
    outer->addLayout(headerRow);

    auto *content = card(page);
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    content->setMinimumHeight(390);

    auto *grid = new QGridLayout(content);
    grid->setContentsMargins(28, 22, 28, 22);
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(10);
    grid->setColumnStretch(0, 0);
    grid->setColumnStretch(1, 1);

    m_nom = line(tr("Nom"), content);
    m_prenom = line(tr("Prénom"), content);
    m_numero = line(tr("Numéro"), content);
    m_adresse = line(tr("Adresse"), content);
    m_mail = line(tr("Email"), content);
    m_region = line(tr("Région"), content);

    m_errorNom = smallError(content);
    m_errorPrenom = smallError(content);
    m_errorNumero = smallError(content);
    m_errorAdresse = smallError(content);
    m_errorMail = smallError(content);
    m_errorRegion = smallError(content);

    auto addRow = [grid](int row, const QString& labelText, QLineEdit* edit) {
        auto *label = new QLabel(labelText, edit->parentWidget());
        label->setMinimumWidth(70);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        grid->addWidget(label, row, 0);
        grid->addWidget(edit, row, 1);
    };

    addRow(0, tr("Nom"), m_nom);
    addRow(1, tr("Prénom"), m_prenom);
    addRow(2, tr("Numéro"), m_numero);
    addRow(3, tr("Adresse"), m_adresse);
    addRow(4, tr("Email"), m_mail);
    addRow(5, tr("Région"), m_region);

    auto *inlineSubmit = new QPushButton(tr("Ajouter l'agriculteur"), content);
    inlineSubmit->setObjectName(QStringLiteral("agriInlineSubmitButton"));
    inlineSubmit->setProperty("type", "primary");
    inlineSubmit->setMinimumSize(220, 42);
    inlineSubmit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(inlineSubmit, &QPushButton::clicked, this, &AgriculteurModule::addAgriculteur);
    grid->addWidget(inlineSubmit, 6, 1, Qt::AlignRight);

    outer->addWidget(content, 0);
    outer->addStretch(1);
    m_stack->addWidget(page);
}

void AgriculteurModule::buildConsultPage()
{
    auto *page = new QWidget(m_stack);
    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(16, 14, 16, 16);
    outer->setSpacing(12);

    auto *top = new QHBoxLayout();
    m_rech = line(tr("Rechercher..."), page);
    m_comboRech = new QComboBox(page);
    m_comboRech->addItems({tr("Nom"), tr("Numero"), tr("Région")});
    auto *btnRech = new QPushButton(tr("Rechercher"), page);
    btnRech->setProperty("type", "primary");
    m_comboTri = new QComboBox(page);
    m_comboTri->addItems({tr("Rendement moyen"), tr("Quantité olives"), tr("Région"), tr("Score")});
    m_comboOrdre = new QComboBox(page);
    m_comboOrdre->addItems({tr("Ascendant"), tr("Descendant")});
    auto *btnTrier = new QPushButton(tr("Trier"), page);
    top->addWidget(m_rech, 2);
    top->addWidget(m_comboRech);
    top->addWidget(btnRech);
    top->addSpacing(12);
    top->addWidget(m_comboTri);
    top->addWidget(m_comboOrdre);
    top->addWidget(btnTrier);
    outer->addLayout(top);

    m_tableau = new QTableWidget(page);
    setTableBasics(m_tableau);
    outer->addWidget(m_tableau, 1);

    connect(btnRech, &QPushButton::clicked, this, &AgriculteurModule::searchAgriculteurs);
    connect(m_rech, &QLineEdit::returnPressed, this, &AgriculteurModule::searchAgriculteurs);
    connect(btnTrier, &QPushButton::clicked, this, &AgriculteurModule::sortAgriculteurs);

    m_stack->addWidget(page);
}

void AgriculteurModule::buildEditPage()
{
    auto *page = new QWidget(m_stack);
    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(24, 20, 24, 24);
    outer->setSpacing(14);
    outer->addWidget(title(tr("Modifier un agriculteur"), page));

    auto *content = card(page);
    auto *form = new QFormLayout(content);
    form->setContentsMargins(28, 24, 28, 28);
    form->setSpacing(12);
    m_mNom = line(tr("Nom"), content);
    m_mPrenom = line(tr("Prénom"), content);
    m_mNumero = line(tr("Numéro"), content);
    m_mAdresse = line(tr("Adresse"), content);
    m_mNbArbre = line(tr("Nb arbres"), content);
    m_mTypeOlive = line(tr("Type olives"), content);
    m_mMail = line(tr("Email"), content);
    m_mRegion = line(tr("Région"), content);
    m_errorMNom = smallError(content); m_errorMPrenom = smallError(content); m_errorMNumero = smallError(content);
    m_errorMAdresse = smallError(content); m_errorMMail = smallError(content); m_errorMRegion = smallError(content);
    form->addRow(tr("Nom"), m_mNom); form->addRow(QString(), m_errorMNom);
    form->addRow(tr("Prénom"), m_mPrenom); form->addRow(QString(), m_errorMPrenom);
    form->addRow(tr("Numéro"), m_mNumero); form->addRow(QString(), m_errorMNumero);
    form->addRow(tr("Adresse"), m_mAdresse); form->addRow(QString(), m_errorMAdresse);
    form->addRow(tr("Nb arbres"), m_mNbArbre);
    form->addRow(tr("Type olives"), m_mTypeOlive);
    form->addRow(tr("Email"), m_mMail); form->addRow(QString(), m_errorMMail);
    form->addRow(tr("Région"), m_mRegion); form->addRow(QString(), m_errorMRegion);
    auto *row = new QHBoxLayout();
    auto *retour = new QPushButton(tr("Retour"), content);
    auto *save = new QPushButton(tr("Valider"), content);
    save->setProperty("type", "primary");
    row->addStretch(); row->addWidget(retour); row->addWidget(save);
    form->addRow(QString(), row);
    connect(retour, &QPushButton::clicked, this, &AgriculteurModule::backToConsult);
    connect(save, &QPushButton::clicked, this, &AgriculteurModule::saveEdit);
    outer->addWidget(content);
    outer->addStretch();
    m_stack->addWidget(page);
}

void AgriculteurModule::buildHistoryPage()
{
    auto *page = new QWidget(m_stack);
    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(16, 14, 16, 16);
    outer->setSpacing(10);
    auto *top = new QHBoxLayout();
    auto *back = new QPushButton(tr("Retour"), page);
    m_labelMoyenne = new QLabel(page);
    m_labelTotal = new QLabel(page);
    m_labelPerformance = new QLabel(page);
    top->addWidget(back); top->addStretch(); top->addWidget(m_labelMoyenne); top->addWidget(m_labelTotal); top->addWidget(m_labelPerformance);
    outer->addLayout(top);
    connect(back, &QPushButton::clicked, this, &AgriculteurModule::backToConsult);

    m_tableHistorique = new QTableWidget(page);
    setTableBasics(m_tableHistorique);
    outer->addWidget(m_tableHistorique, 1);

    auto *addBox = new QGroupBox(tr("Ajouter historique / récolte"), page);
    auto *grid = new QGridLayout(addBox);
    m_anneeH = line(tr("Année"), addBox);
    m_quantiteH = line(tr("Quantité"), addBox);
    m_nbArbreH = line(tr("Nb arbres"), addBox);
    m_typeH = line(tr("Type olives"), addBox);
    m_noteH = line(tr("Note 0-10"), addBox);
    m_dateRecolteH = new QDateEdit(QDate::currentDate(), addBox);
    m_dateRecolteH->setCalendarPopup(true);
    auto *add = new QPushButton(tr("Ajouter historique"), addBox);
    add->setProperty("type", "primary");
    grid->addWidget(new QLabel(tr("Année"), addBox), 0, 0); grid->addWidget(m_anneeH, 0, 1);
    grid->addWidget(new QLabel(tr("Quantité"), addBox), 0, 2); grid->addWidget(m_quantiteH, 0, 3);
    grid->addWidget(new QLabel(tr("Nb arbres"), addBox), 1, 0); grid->addWidget(m_nbArbreH, 1, 1);
    grid->addWidget(new QLabel(tr("Type"), addBox), 1, 2); grid->addWidget(m_typeH, 1, 3);
    grid->addWidget(new QLabel(tr("Note"), addBox), 2, 0); grid->addWidget(m_noteH, 2, 1);
    grid->addWidget(new QLabel(tr("Date récolte"), addBox), 2, 2); grid->addWidget(m_dateRecolteH, 2, 3);
    grid->addWidget(add, 3, 3);
    connect(add, &QPushButton::clicked, this, &AgriculteurModule::addHistorique);
    outer->addWidget(addBox);
    m_stack->addWidget(page);
}

void AgriculteurModule::buildHistoryEditPage()
{
    auto *page = new QWidget(m_stack);
    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(24, 20, 24, 24);
    outer->setSpacing(14);
    outer->addWidget(title(tr("Modifier historique"), page));
    auto *content = card(page);
    auto *form = new QFormLayout(content);
    form->setContentsMargins(28, 24, 28, 28);
    m_anneeH2 = line(tr("Année"), content);
    m_quantiteH2 = line(tr("Quantité"), content);
    m_nbArbreH2 = line(tr("Nb arbres"), content);
    m_typeH2 = line(tr("Type olives"), content);
    m_noteH2 = line(tr("Note"), content);
    form->addRow(tr("Année"), m_anneeH2);
    form->addRow(tr("Quantité"), m_quantiteH2);
    form->addRow(tr("Nb arbres"), m_nbArbreH2);
    form->addRow(tr("Type olives"), m_typeH2);
    form->addRow(tr("Note"), m_noteH2);
    auto *row = new QHBoxLayout();
    auto *back = new QPushButton(tr("Retour"), content);
    auto *save = new QPushButton(tr("Modifier"), content);
    save->setProperty("type", "primary");
    row->addStretch(); row->addWidget(back); row->addWidget(save);
    form->addRow(QString(), row);
    connect(back, &QPushButton::clicked, this, &AgriculteurModule::backToHistory);
    connect(save, &QPushButton::clicked, this, &AgriculteurModule::saveHistoriqueEdit);
    outer->addWidget(content);
    outer->addStretch();
    m_stack->addWidget(page);
}

void AgriculteurModule::buildStatsPage()
{
    auto *page = new QWidget(m_stack);
    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(16, 14, 16, 16);
    outer->setSpacing(12);
    auto *row = new QHBoxLayout();
    m_choixTri = new QComboBox(page);
    m_choixTri->addItems({tr("Quantité"), tr("Note"), tr("Arbres")});
    auto *ok = new QPushButton(tr("Afficher"), page);
    ok->setProperty("type", "primary");
    row->addWidget(new QLabel(tr("Faire les statistiques selon :"), page));
    row->addWidget(m_choixTri); row->addWidget(ok); row->addStretch();
    outer->addLayout(row);
    m_chartView = new QChartView(page);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    outer->addWidget(m_chartView, 1);
    connect(ok, &QPushButton::clicked, this, &AgriculteurModule::renderSelectedStat);
    m_stack->addWidget(page);
}

void AgriculteurModule::buildDetectionPage()
{
    auto *page = new QWidget(m_stack);
    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(16, 14, 16, 16);
    auto *top = new QHBoxLayout();
    auto *back = new QPushButton(tr("Retour"), page);
    auto *reload = new QPushButton(tr("Actualiser détection"), page);
    reload->setProperty("type", "primary");
    top->addWidget(back); top->addWidget(reload); top->addStretch();
    outer->addLayout(top);
    m_tableDetection = new QTableWidget(page);
    setTableBasics(m_tableDetection);
    outer->addWidget(m_tableDetection, 1);
    connect(back, &QPushButton::clicked, this, &AgriculteurModule::backToConsult);
    connect(reload, &QPushButton::clicked, this, &AgriculteurModule::showDetectionPage);
    m_stack->addWidget(page);
}

void AgriculteurModule::buildPredictionPage()
{
    auto *page = new QWidget(m_stack);
    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(16, 14, 16, 16);
    auto *back = new QPushButton(tr("Retour"), page);
    outer->addWidget(back, 0, Qt::AlignLeft);
    m_labelPrediction = new QLabel(page);
    m_labelPrediction->setStyleSheet(QStringLiteral("font-size:16px;font-weight:800;color:#263414;"));
    outer->addWidget(m_labelPrediction);
    m_chartPrediction = new QChartView(page);
    m_chartPrediction->setRenderHint(QPainter::Antialiasing);
    outer->addWidget(m_chartPrediction, 1);
    connect(back, &QPushButton::clicked, this, &AgriculteurModule::backToConsult);
    m_stack->addWidget(page);
}

void AgriculteurModule::buildAdvancedPage()
{
    auto *page = new QWidget(m_stack);
    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(24, 20, 24, 24);
    auto *panel = card(page);
    auto *lay = new QVBoxLayout(panel);
    lay->setContentsMargins(28, 28, 28, 28);
    lay->setSpacing(14);
    lay->addWidget(title(tr("Métiers avancés agriculteurs"), panel));
    auto *detect = new QPushButton(tr("Détection intelligente des agriculteurs"), panel);
    detect->setProperty("type", "primary");
    lay->addWidget(new QLabel(tr("Analyse les agriculteurs selon quantité, rendement, note qualité et historique."), panel));
    lay->addWidget(detect);
    lay->addStretch();
    outer->addWidget(panel, 1);
    connect(detect, &QPushButton::clicked, this, &AgriculteurModule::showDetectionPage);
    m_stack->addWidget(page);
}

void AgriculteurModule::setTableBasics(QTableWidget *table)
{
    if (!table) return;
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setShowGrid(false);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
}

QPushButton* AgriculteurModule::makeTableButton(const QString &text, const QString &type, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setProperty("role", "tableAction");
    button->setProperty("type", type);
    button->setMinimumWidth(84);
    button->setMaximumHeight(30);
    return button;
}

void AgriculteurModule::showAddPage() { m_stack->setCurrentWidget(m_addPage ? m_addPage : m_stack->widget(0)); updateFloatingSubmitButton(); }
void AgriculteurModule::showConsultPage() { afficherTableau(); m_stack->setCurrentIndex(1); updateFloatingSubmitButton(); }
void AgriculteurModule::showStatsPage() { m_stack->setCurrentIndex(5); updateFloatingSubmitButton(); }
void AgriculteurModule::showAdvancedPage() { m_stack->setCurrentIndex(8); updateFloatingSubmitButton(); }
void AgriculteurModule::refreshAll() { afficherTableau(); afficherTableauDetection(); }
void AgriculteurModule::backToConsult() { showConsultPage(); }
void AgriculteurModule::backToHistory() { if (m_idSelectionne > 0) afficherHistorique(m_idSelectionne); m_stack->setCurrentIndex(3); }

void AgriculteurModule::clearAddErrors()
{
    for (QLabel *label : {m_errorNom, m_errorPrenom, m_errorNumero, m_errorAdresse, m_errorMail, m_errorRegion})
        if (label) label->clear();
}

bool AgriculteurModule::validateAddForm()
{
    clearAddErrors();
    bool valid = true;
    if (m_nom->text().trimmed().isEmpty()) { m_errorNom->setText(tr("Nom obligatoire")); valid = false; }
    if (m_prenom->text().trimmed().isEmpty()) { m_errorPrenom->setText(tr("Prénom obligatoire")); valid = false; }
    if (m_numero->text().trimmed().isEmpty()) { m_errorNumero->setText(tr("Numéro obligatoire")); valid = false; }
    else if (m_numero->text().trimmed().length() < 8) { m_errorNumero->setText(tr("Numéro invalide")); valid = false; }
    if (m_adresse->text().trimmed().isEmpty()) { m_errorAdresse->setText(tr("Adresse obligatoire")); valid = false; }
    if (!m_mail->text().trimmed().contains('@')) { m_errorMail->setText(tr("Email invalide")); valid = false; }
    if (m_region->text().trimmed().isEmpty()) { m_errorRegion->setText(tr("Région obligatoire")); valid = false; }
    return valid;
}

bool AgriculteurModule::validateEditForm()
{
    for (QLabel *label : {m_errorMNom, m_errorMPrenom, m_errorMNumero, m_errorMAdresse, m_errorMMail, m_errorMRegion})
        if (label) label->clear();
    bool valid = true;
    if (m_mNom->text().trimmed().isEmpty()) { m_errorMNom->setText(tr("Nom obligatoire")); valid = false; }
    if (m_mPrenom->text().trimmed().isEmpty()) { m_errorMPrenom->setText(tr("Prénom obligatoire")); valid = false; }
    if (m_mNumero->text().trimmed().isEmpty()) { m_errorMNumero->setText(tr("Numéro obligatoire")); valid = false; }
    else if (m_mNumero->text().trimmed().length() < 8) { m_errorMNumero->setText(tr("Numéro invalide")); valid = false; }
    if (m_mAdresse->text().trimmed().isEmpty()) { m_errorMAdresse->setText(tr("Adresse obligatoire")); valid = false; }
    if (!m_mMail->text().trimmed().contains('@')) { m_errorMMail->setText(tr("Email invalide")); valid = false; }
    if (m_mRegion->text().trimmed().isEmpty()) { m_errorMRegion->setText(tr("Région obligatoire")); valid = false; }
    return valid;
}

void AgriculteurModule::addAgriculteur()
{
    if (!validateAddForm()) return;

    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO AGRICULTEUR "
        "(nom_agri, prenom_agri, adresse_agri, num_agri, mail_agri, region_agri) "
        "VALUES (:nom,:prenom,:adresse,:num,:mail,:region)"));
    query.bindValue(":nom", m_nom->text().trimmed());
    query.bindValue(":prenom", m_prenom->text().trimmed());
    query.bindValue(":adresse", m_adresse->text().trimmed());
    query.bindValue(":num", m_numero->text().trimmed());
    query.bindValue(":mail", m_mail->text().trimmed());
    query.bindValue(":region", m_region->text().trimmed());

    if (!query.exec()) {
        QMessageBox::critical(this, tr("Erreur"), tr("Erreur INSERT:\n%1").arg(query.lastError().text()));
        return;
    }

    QMessageBox::information(this, tr("Succès"), tr("Agriculteur ajouté !"));
    for (QLineEdit *edit : {m_nom, m_prenom, m_numero, m_adresse, m_mail, m_region}) edit->clear();
    showConsultPage();
}

void AgriculteurModule::saveEdit()
{
    if (m_idSelectionne <= 0) return;
    if (!validateEditForm()) return;

    QSqlQuery query;
    query.prepare(QStringLiteral(
        "UPDATE AGRICULTEUR SET nom_agri=:nom, prenom_agri=:prenom, num_agri=:num, "
        "adresse_agri=:adresse, mail_agri=:mail, region_agri=:region WHERE id_agri=:id"));
    query.bindValue(":nom", m_mNom->text().trimmed());
    query.bindValue(":prenom", m_mPrenom->text().trimmed());
    query.bindValue(":num", m_mNumero->text().trimmed());
    query.bindValue(":adresse", m_mAdresse->text().trimmed());
    query.bindValue(":mail", m_mMail->text().trimmed());
    query.bindValue(":region", m_mRegion->text().trimmed());
    query.bindValue(":id", m_idSelectionne);

    if (!query.exec()) {
        QMessageBox::critical(this, tr("Erreur"), query.lastError().text());
        return;
    }
    QMessageBox::information(this, tr("Succès"), tr("Modification réussie !"));
    showConsultPage();
}


bool AgriculteurModule::tableExists(const QString &tableName) const
{
    QSqlQuery q;
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM USER_TABLES WHERE TABLE_NAME = :table_name"));
    q.bindValue(QStringLiteral(":table_name"), tableName.trimmed().toUpper());
    return q.exec() && q.next() && q.value(0).toInt() > 0;
}

void AgriculteurModule::ensureAgriculteurSupportTables()
{
    // Your current exported schema has AGRICULTEUR/STOCK, but the integrated
    // Agriculteur module also reads HISTORIQUE_OLIVES. If the table is missing,
    // the Consulter query fails with ORA-00942 and the list remains empty.
    if (tableExists(QStringLiteral("HISTORIQUE_OLIVES")))
        return;

    QSqlQuery q;
    if (!q.exec(QStringLiteral(
            "CREATE TABLE HISTORIQUE_OLIVES ("
            "ID_HIST NUMBER, "
            "ID_AGRI NUMBER, "
            "ANNEE NUMBER, "
            "QUANTITE NUMBER, "
            "NB_ARBRES NUMBER, "
            "TYPE_OLIVES VARCHAR2(50), "
            "NOTE NUMBER(5,2), "
            "DATERECOLTE DATE)"))) {
        qDebug() << "[AgriculteurModule] Could not create HISTORIQUE_OLIVES:" << q.lastError().text();
        return;
    }

    QSqlQuery seq;
    if (!seq.exec(QStringLiteral("CREATE SEQUENCE SEQ_HISTORIQUE_OLIVES START WITH 1 INCREMENT BY 1 NOCACHE NOCYCLE"))) {
        qDebug() << "[AgriculteurModule] Could not create SEQ_HISTORIQUE_OLIVES:" << seq.lastError().text();
    }

    QSqlQuery trig;
    if (!trig.exec(QStringLiteral(
            "CREATE OR REPLACE TRIGGER TRG_HISTORIQUE_OLIVES_BI "
            "BEFORE INSERT ON HISTORIQUE_OLIVES "
            "FOR EACH ROW "
            "BEGIN "
            "IF :NEW.ID_HIST IS NULL THEN "
            "SELECT SEQ_HISTORIQUE_OLIVES.NEXTVAL INTO :NEW.ID_HIST FROM DUAL; "
            "END IF; "
            "END;"))) {
        qDebug() << "[AgriculteurModule] Could not create TRG_HISTORIQUE_OLIVES_BI:" << trig.lastError().text();
    }
}

QString AgriculteurModule::buildAgriculteurListQuery(const QString &whereClause, const QString &orderClause) const
{
    const bool hasHistory = tableExists(QStringLiteral("HISTORIQUE_OLIVES"));
    const bool hasStock = tableExists(QStringLiteral("STOCK"));

    const QString historyExpr = hasHistory
        ? QStringLiteral("(SELECT AVG(H.QUANTITE) FROM HISTORIQUE_OLIVES H WHERE H.ID_AGRI = A.ID_AGRI)")
        : QStringLiteral("CAST(NULL AS NUMBER)");

    QString sql = QStringLiteral(
        "SELECT A.ID_AGRI, A.NOM_AGRI, A.PRENOM_AGRI, A.NUM_AGRI, A.ADRESSE_AGRI, "
        "A.NB_ARBRES, A.TYPE_OLIVES, A.MAIL_AGRI, A.REGION_AGRI, "
        "A.QTOLIVES_ANNEEPREC, A.RENDE_MOY, A.NOTE_QUALTMOY, ");

    if (hasStock) {
        sql += QStringLiteral(
            "S.QT_STOCK, S.DATEMAJ_STOCK, %1 AS MOY_HIST "
            "FROM AGRICULTEUR A LEFT JOIN ("
            " SELECT ID_AGRI, QT_STOCK, DATEMAJ_STOCK FROM STOCK "
            " WHERE ID_AGRI IS NOT NULL AND (ID_AGRI, NVL(DATEMAJ_STOCK, DATE '1900-01-01')) IN ("
            "  SELECT ID_AGRI, MAX(NVL(DATEMAJ_STOCK, DATE '1900-01-01')) FROM STOCK WHERE ID_AGRI IS NOT NULL GROUP BY ID_AGRI"
            " )"
            ") S ON A.ID_AGRI = S.ID_AGRI ").arg(historyExpr);
    } else {
        sql += QStringLiteral(
            "CAST(NULL AS NUMBER) AS QT_STOCK, CAST(NULL AS DATE) AS DATEMAJ_STOCK, %1 AS MOY_HIST "
            "FROM AGRICULTEUR A ").arg(historyExpr);
    }

    if (!whereClause.trimmed().isEmpty())
        sql += whereClause + QLatin1Char(' ');
    if (!orderClause.trimmed().isEmpty())
        sql += orderClause;
    return sql;
}

void AgriculteurModule::afficherTableau()
{
    afficherTableauAvecQuery(buildAgriculteurListQuery(QString(), QStringLiteral("ORDER BY A.ID_AGRI DESC")));
}

void AgriculteurModule::afficherTableauAvecQuery(const QString &queryStr)
{
    m_tableau->setRowCount(0);
    m_tableau->setColumnCount(12);
    m_tableau->setHorizontalHeaderLabels({tr("ID"), tr("Nom"), tr("Prénom"), tr("Numéro"), tr("Adresse"), tr("Nb Arbres"), tr("Type"), tr("Mail"), tr("Région"), tr("Stock"), tr("Date"), tr("Action")});

    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qDebug() << "ERREUR SQL:" << query.lastError().text() << "REQUETE:" << queryStr;
        return;
    }

    int i = 0;
    while (query.next()) {
        m_tableau->insertRow(i);
        const int id = query.value("ID_AGRI").toInt();
        const float qt = query.value("QT_STOCK").toFloat();
        const float qtPrec = query.value("QTOLIVES_ANNEEPREC").toFloat();
        const float rendement = query.value("RENDE_MOY").toFloat();
        const float note = query.value("NOTE_QUALTMOY").toFloat();
        const int nb = query.value("NB_ARBRES").toInt();
        Q_UNUSED(qt); Q_UNUSED(qtPrec); Q_UNUSED(rendement); Q_UNUSED(note); Q_UNUSED(nb);

        const QString nbArbres = query.value("NB_ARBRES").isNull() ? QStringLiteral("0") : query.value("NB_ARBRES").toString();
        const QString typeOlives = query.value("TYPE_OLIVES").isNull() ? QStringLiteral("-") : query.value("TYPE_OLIVES").toString();
        const QString qtStock = query.value("QT_STOCK").isNull() ? QStringLiteral("0") : query.value("QT_STOCK").toString();
        const QString dateStock = query.value("DATEMAJ_STOCK").isNull() ? QStringLiteral("-") : query.value("DATEMAJ_STOCK").toDate().toString(QStringLiteral("dd/MM/yyyy"));

        const QStringList vals = {QString::number(id), query.value("NOM_AGRI").toString(), query.value("PRENOM_AGRI").toString(), query.value("NUM_AGRI").toString(), query.value("ADRESSE_AGRI").toString(), nbArbres, typeOlives, query.value("MAIL_AGRI").toString(), query.value("REGION_AGRI").toString(), qtStock, dateStock};
        for (int col = 0; col < vals.size(); ++col) {
            auto *item = new QTableWidgetItem(vals.at(col));
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            m_tableau->setItem(i, col, item);
        }

        auto *widget = new QWidget(m_tableau);
        auto *layout = new QHBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(6);
        auto *btnSupprimer = makeTableButton(tr("Supprimer"), QStringLiteral("danger"), widget);
        auto *btnModifier = makeTableButton(tr("Modifier"), QStringLiteral("warning"), widget);
        auto *btnHistorique = makeTableButton(tr("Historique"), QStringLiteral("primary"), widget);
        auto *btnPrediction = makeTableButton(tr("Prediction"), QStringLiteral("primary"), widget);
        layout->addWidget(btnModifier);
        layout->addWidget(btnHistorique);
        layout->addWidget(btnPrediction);
        layout->addWidget(btnSupprimer);
        m_tableau->setCellWidget(i, 11, widget);
        m_tableau->setRowHeight(i, 50);

        connect(btnPrediction, &QPushButton::clicked, this, [this, id]() {
            m_idSelectionne = id;
            afficherPrediction(id);
            m_stack->setCurrentIndex(7);
        });
        connect(btnHistorique, &QPushButton::clicked, this, [this, id]() {
            m_idSelectionne = id;
            m_anneeH->clear(); m_quantiteH->clear(); m_nbArbreH->clear(); m_typeH->clear(); m_noteH->clear();
            m_dateRecolteH->setDate(QDate::currentDate());
            afficherHistorique(id);
            m_stack->setCurrentIndex(3);
        });
        connect(btnSupprimer, &QPushButton::clicked, this, [this, id]() {
            if (QMessageBox::question(this, tr("Confirmation"), tr("Supprimer cet agriculteur et ses données liées ?")) != QMessageBox::Yes) return;
            QSqlQuery q;
            q.prepare(QStringLiteral("DELETE FROM HISTORIQUE_OLIVES WHERE ID_AGRI=:id")); q.bindValue(":id", id); q.exec();
            q.prepare(QStringLiteral("DELETE FROM STOCK WHERE ID_AGRI=:id")); q.bindValue(":id", id); q.exec();
            q.prepare(QStringLiteral("DELETE FROM AGRICULTEUR WHERE ID_AGRI=:id")); q.bindValue(":id", id); q.exec();
            afficherTableau();
        });
        connect(btnModifier, &QPushButton::clicked, this, [this, id, vals, nbArbres, typeOlives]() {
            m_idSelectionne = id;
            m_mNom->setText(vals.value(1));
            m_mPrenom->setText(vals.value(2));
            m_mNumero->setText(vals.value(3));
            m_mAdresse->setText(vals.value(4));
            m_mNbArbre->setText(nbArbres);
            m_mTypeOlive->setText(typeOlives);
            m_mMail->setText(vals.value(7));
            m_mRegion->setText(vals.value(8));
            m_stack->setCurrentIndex(2);
        });
        ++i;
    }
    m_tableau->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableau->horizontalHeader()->setSectionResizeMode(11, QHeaderView::ResizeToContents);
}

void AgriculteurModule::searchAgriculteurs()
{
    const QString texte = m_rech->text().trimmed();
    const QString choix = m_comboRech->currentText();
    if (texte.isEmpty()) { afficherTableau(); return; }

    QString where;
    const QString escaped = texte;
    if (choix == tr("Nom")) where = "WHERE LOWER(A.NOM_AGRI) LIKE LOWER('%" + escaped + "%')";
    else if (choix == tr("Numero")) where = "WHERE A.NUM_AGRI LIKE '%" + escaped + "%'";
    else where = "WHERE LOWER(A.REGION_AGRI) LIKE LOWER('%" + escaped + "%')";
    afficherTableauAvecQuery(buildAgriculteurListQuery(where, QStringLiteral("ORDER BY A.ID_AGRI DESC")));
}

void AgriculteurModule::sortAgriculteurs()
{
    const QString choix = m_comboTri->currentText();
    const QString ordre = m_comboOrdre->currentText();
    const QString orderSql = (ordre == tr("Ascendant")) ? QStringLiteral("ASC") : QStringLiteral("DESC");
    QString order;
    if (choix == tr("Rendement moyen")) order = "ORDER BY NVL(A.RENDE_MOY, 0) " + orderSql;
    else if (choix == tr("Quantité olives")) order = "ORDER BY NVL(QT_STOCK, 0) " + orderSql;
    else if (choix == tr("Région")) order = "ORDER BY A.REGION_AGRI " + orderSql;
    else order = "ORDER BY NVL(A.NOTE_QUALTMOY, 0) " + orderSql;
    afficherTableauAvecQuery(buildAgriculteurListQuery(QString(), order));
}

void AgriculteurModule::afficherHistorique(int idAgri)
{
    m_tableHistorique->setRowCount(0);
    QSqlQuery q;
    q.prepare(QStringLiteral("SELECT NB_ARBRES, TYPE_OLIVES FROM AGRICULTEUR WHERE ID_AGRI=:id"));
    q.bindValue(":id", idAgri);
    if (q.exec() && q.next()) {
        m_nbArbreH->setText(q.value(0).toString());
        m_typeH->setText(q.value(1).toString());
    }

    QSqlQuery stats;
    stats.prepare(QStringLiteral("SELECT AVG(QUANTITE), SUM(QUANTITE) FROM HISTORIQUE_OLIVES WHERE ID_AGRI=:id"));
    stats.bindValue(":id", idAgri);
    float moyenne = 0, total = 0;
    if (stats.exec() && stats.next()) { moyenne = stats.value(0).toFloat(); total = stats.value(1).toFloat(); }
    m_labelMoyenne->setText(tr("Rendement moyen : %1").arg(moyenne));
    m_labelTotal->setText(tr("Quantité totale : %1").arg(total));

    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT H.ID_HIST, H.ANNEE, H.QUANTITE, H.NB_ARBRES, H.TYPE_OLIVES, H.NOTE, "
        "(SELECT MAX(DATEMAJ_STOCK) FROM STOCK S WHERE S.ID_AGRI = H.ID_AGRI) AS DATE_STOCK "
        "FROM HISTORIQUE_OLIVES H WHERE H.ID_AGRI=:id ORDER BY H.ANNEE DESC"));
    query.bindValue(":id", idAgri);
    query.exec();
    m_tableHistorique->setColumnCount(8);
    m_tableHistorique->setHorizontalHeaderLabels({tr("Année"), tr("Quantité"), tr("Nb arbres"), tr("Type"), tr("Note"), tr("Date"), tr("Evolution"), tr("Action")});

    int i = 0;
    float noteMoy = 0;
    while (query.next()) {
        m_tableHistorique->insertRow(i);
        const int idHist = query.value(0).toInt();
        const QString annee = query.value(1).toString();
        const QString quantite = query.value(2).toString();
        const QString nbArbres = query.value(3).toString();
        const QString type = query.value(4).toString();
        const QString note = query.value(5).toString();
        const QString dateStock = query.value(6).toDate().toString(QStringLiteral("dd/MM/yyyy"));
        const QStringList vals = {annee, quantite, nbArbres, type, note, dateStock};
        for (int c = 0; c < vals.size(); ++c) m_tableHistorique->setItem(i, c, new QTableWidgetItem(vals.at(c)));
        if (i > 0) {
            const float actuel = quantite.toFloat();
            const float precedent = m_tableHistorique->item(i - 1, 1)->text().toFloat();
            m_tableHistorique->setItem(i, 6, new QTableWidgetItem(actuel > precedent ? QStringLiteral("📈") : (actuel < precedent ? QStringLiteral("📉") : QStringLiteral("➡️"))));
        }
        auto *w = new QWidget(m_tableHistorique);
        auto *lay = new QHBoxLayout(w);
        lay->setContentsMargins(0,0,0,0);
        auto *mod = makeTableButton(tr("Modifier"), QStringLiteral("warning"), w);
        auto *del = makeTableButton(tr("Supprimer"), QStringLiteral("danger"), w);
        lay->addWidget(mod); lay->addWidget(del);
        m_tableHistorique->setCellWidget(i, 7, w);
        connect(del, &QPushButton::clicked, this, [this, idHist]() {
            QSqlQuery q; q.prepare(QStringLiteral("DELETE FROM HISTORIQUE_OLIVES WHERE ID_HIST=:id")); q.bindValue(":id", idHist); q.exec();
            afficherHistorique(m_idSelectionne);
        });
        connect(mod, &QPushButton::clicked, this, [this, idHist, annee, quantite, nbArbres, type, note]() {
            m_anneeH2->setText(annee); m_quantiteH2->setText(quantite); m_nbArbreH2->setText(nbArbres); m_typeH2->setText(type); m_noteH2->setText(note);
            m_idSelectionneHistorique = idHist;
            m_stack->setCurrentIndex(4);
        });
        noteMoy += note.toFloat();
        ++i;
    }
    const float scoreGlobal = (moyenne > 0 && i > 0) ? ((moyenne * 0.7f) + ((noteMoy / i) * 0.3f)) : 0;
    m_labelPerformance->setText(tr("Score global : %1 / 10").arg(QString::number(scoreGlobal, 'f', 1)));
}

void AgriculteurModule::addHistorique()
{
    if (m_idSelectionne <= 0) return;
    const QString anneeStr = m_anneeH->text().trimmed();
    const QString qtStr = m_quantiteH->text().trimmed();
    const QString nbStr = m_nbArbreH->text().trimmed();
    const QString type = m_typeH->text().trimmed();
    const QString noteStr = m_noteH->text().trimmed();
    if (anneeStr.isEmpty() || qtStr.isEmpty() || nbStr.isEmpty() || type.isEmpty() || noteStr.isEmpty()) {
        QMessageBox::warning(this, tr("Erreur"), tr("Tous les champs sont obligatoires !"));
        return;
    }
    const int annee = anneeStr.toInt();
    const float qt = qtStr.toFloat();
    const int nb = nbStr.toInt();
    const float note = noteStr.toFloat();
    if (annee < 2000 || annee > 2100 || qt <= 0 || nb <= 0 || note < 0 || note > 10) {
        QMessageBox::warning(this, tr("Erreur"), tr("Valeurs invalides !"));
        return;
    }

    QSqlQuery queryHist;
    queryHist.prepare(QStringLiteral("INSERT INTO HISTORIQUE_OLIVES (ID_AGRI, ANNEE, NB_ARBRES, TYPE_OLIVES, QUANTITE, NOTE, DATERECOLTE) VALUES (:id,:annee,:nb,:type,:qt,:note,:dateRec)"));
    queryHist.bindValue(":id", m_idSelectionne);
    queryHist.bindValue(":annee", annee);
    queryHist.bindValue(":nb", nb);
    queryHist.bindValue(":type", type);
    queryHist.bindValue(":qt", qt);
    queryHist.bindValue(":note", note);
    queryHist.bindValue(":dateRec", m_dateRecolteH->date());
    if (!queryHist.exec()) { QMessageBox::critical(this, tr("Erreur historique"), queryHist.lastError().text()); return; }

    QSqlQuery qMoy;
    qMoy.prepare(QStringLiteral("SELECT AVG(QUANTITE) FROM HISTORIQUE_OLIVES WHERE ID_AGRI=:id"));
    qMoy.bindValue(":id", m_idSelectionne); qMoy.exec();
    float moyenne = 0; if (qMoy.next()) moyenne = qMoy.value(0).toFloat();

    QSqlQuery updateRendement;
    updateRendement.prepare(QStringLiteral("UPDATE AGRICULTEUR SET RENDE_MOY=:moy WHERE ID_AGRI=:id"));
    updateRendement.bindValue(":moy", moyenne); updateRendement.bindValue(":id", m_idSelectionne); updateRendement.exec();

    QSqlQuery qNote;
    qNote.prepare(QStringLiteral("SELECT AVG(NOTE) FROM HISTORIQUE_OLIVES WHERE ID_AGRI=:id"));
    qNote.bindValue(":id", m_idSelectionne); qNote.exec();
    float noteMoy = 0; if (qNote.next()) noteMoy = qNote.value(0).toFloat();

    QSqlQuery updateNote;
    updateNote.prepare(QStringLiteral("UPDATE AGRICULTEUR SET NOTE_QUALTMOY=:note WHERE ID_AGRI=:id"));
    updateNote.bindValue(":note", noteMoy); updateNote.bindValue(":id", m_idSelectionne); updateNote.exec();

    QSqlQuery queryStock;
    queryStock.prepare(QStringLiteral("INSERT INTO STOCK (ID_STOCK, NOM_STOCK, CATEG_STOCK, DESCRIPT_STOCK, QT_STOCK, ID_AGRI, DATEMAJ_STOCK) VALUES (seq_stock.NEXTVAL, 'Stock Olives', 'Olives', 'Ajout', :qt, :id, SYSDATE)"));
    queryStock.bindValue(":qt", qt); queryStock.bindValue(":id", m_idSelectionne);
    queryStock.exec();

    QSqlQuery updateAgri;
    updateAgri.prepare(QStringLiteral("UPDATE AGRICULTEUR SET NB_ARBRES=:nb, TYPE_OLIVES=:type, DATERECOLTE=:dateRec WHERE ID_AGRI=:id"));
    updateAgri.bindValue(":nb", nb); updateAgri.bindValue(":type", type); updateAgri.bindValue(":dateRec", m_dateRecolteH->date()); updateAgri.bindValue(":id", m_idSelectionne); updateAgri.exec();

    QMessageBox::information(this, tr("Succès"), tr("Historique + Stock ajoutés !"));
    m_anneeH->clear(); m_quantiteH->clear(); m_noteH->clear();
    afficherHistorique(m_idSelectionne); afficherTableau();
}

void AgriculteurModule::saveHistoriqueEdit()
{
    if (m_idSelectionneHistorique <= 0) return;
    QSqlQuery q;
    q.prepare(QStringLiteral("UPDATE HISTORIQUE_OLIVES SET ANNEE=:annee, QUANTITE=:qt, NB_ARBRES=:nb, TYPE_OLIVES=:type, NOTE=:note WHERE ID_HIST=:id"));
    q.bindValue(":annee", m_anneeH2->text().trimmed());
    q.bindValue(":qt", m_quantiteH2->text().trimmed());
    q.bindValue(":nb", m_nbArbreH2->text().trimmed());
    q.bindValue(":type", m_typeH2->text().trimmed());
    q.bindValue(":note", m_noteH2->text().trimmed());
    q.bindValue(":id", m_idSelectionneHistorique);
    if (!q.exec()) { QMessageBox::critical(this, tr("Erreur"), q.lastError().text()); return; }
    QMessageBox::information(this, tr("Succès"), tr("Modification effectuée !"));
    backToHistory();
}

void AgriculteurModule::afficherCourbe(int idAgri, const QString &type)
{
    auto *series = new QLineSeries();
    QSqlQuery query;
    if (type == tr("Note")) query.prepare(QStringLiteral("SELECT ANNEE, NOTE FROM HISTORIQUE_OLIVES WHERE ID_AGRI=:id ORDER BY ANNEE"));
    else if (type == tr("Arbres")) query.prepare(QStringLiteral("SELECT ANNEE, NB_ARBRES FROM HISTORIQUE_OLIVES WHERE ID_AGRI=:id ORDER BY ANNEE"));
    else query.prepare(QStringLiteral("SELECT ANNEE, QUANTITE FROM HISTORIQUE_OLIVES WHERE ID_AGRI=:id ORDER BY ANNEE"));
    query.bindValue(":id", idAgri); query.exec();
    while (query.next()) series->append(query.value(0).toInt(), query.value(1).toFloat());
    auto *chart = new QChart();
    chart->addSeries(series); chart->setTitle(tr("Evolution - %1").arg(type));
    auto *axisX = new QValueAxis(); axisX->setTitleText(tr("Année"));
    auto *axisY = new QValueAxis(); axisY->setTitleText(type);
    chart->addAxis(axisX, Qt::AlignBottom); chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX); series->attachAxis(axisY);
    m_chartView->setChart(chart);
}

void AgriculteurModule::renderSelectedStat()
{
    if (m_idSelectionne <= 0) {
        QMessageBox::warning(this, tr("Erreur"), tr("Sélectionne un agriculteur depuis Consulter > Historique !"));
        return;
    }
    afficherCourbe(m_idSelectionne, m_choixTri->currentText());
}

void AgriculteurModule::afficherPrediction(int id)
{
    auto *historique = new QLineSeries();
    auto *prediction = new QLineSeries();
    QVector<double> x, y;
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT ANNEE, QUANTITE FROM HISTORIQUE_OLIVES WHERE ID_AGRI=:id ORDER BY ANNEE"));
    query.bindValue(":id", id); query.exec();
    int lastYear = 0;
    while (query.next()) {
        int annee = query.value(0).toInt(); double qt = query.value(1).toDouble();
        historique->append(annee, qt); x.push_back(annee); y.push_back(qt); lastYear = annee;
    }
    if (x.size() < 2) { QMessageBox::warning(this, tr("Erreur"), tr("Pas assez de données pour la prédiction !")); return; }
    double sumX=0, sumY=0, sumXY=0, sumX2=0; int n=x.size();
    for (int i=0;i<n;i++){ sumX+=x[i]; sumY+=y[i]; sumXY+=x[i]*y[i]; sumX2+=x[i]*x[i]; }
    double a = (n*sumXY - sumX*sumY) / (n*sumX2 - sumX*sumX);
    double b = (sumY - a*sumX) / n;
    double lastPred = 0;
    for (int i=1;i<=5;i++){ int year=lastYear+i; double pred=a*year+b; if(pred<0) pred=0; prediction->append(year,pred); lastPred=pred; }
    auto *chart = new QChart();
    chart->addSeries(historique); chart->addSeries(prediction); chart->setTitle(tr("Prédiction intelligente de récolte"));
    historique->setName(tr("Historique")); prediction->setName(tr("Prédiction"));
    QPen predPen(Qt::red); predPen.setStyle(Qt::DashLine); predPen.setWidth(2); prediction->setPen(predPen);
    auto *axisX = new QValueAxis(); axisX->setTitleText(tr("Année")); axisX->setRange(x.first(), lastYear + 5);
    auto *axisY = new QValueAxis(); axisY->setTitleText(tr("Quantité"));
    double maxY=0; for(double v:y) maxY=std::max(maxY,v); axisY->setRange(0, maxY*1.2);
    chart->addAxis(axisX, Qt::AlignBottom); chart->addAxis(axisY, Qt::AlignLeft);
    historique->attachAxis(axisX); historique->attachAxis(axisY); prediction->attachAxis(axisX); prediction->attachAxis(axisY);
    chart->legend()->setVisible(true); chart->legend()->setAlignment(Qt::AlignBottom);
    m_chartPrediction->setChart(chart);
    m_labelPrediction->setText(tr("Prédiction IA : %1 olives en %2").arg(QString::number(lastPred, 'f', 2)).arg(lastYear + 5));
}

void AgriculteurModule::afficherTableauDetection()
{
    if (!m_tableDetection) return;
    m_tableDetection->setRowCount(0);
    m_tableDetection->setColumnCount(7);
    m_tableDetection->setHorizontalHeaderLabels({tr("ID"), tr("Nom"), tr("Quantité"), tr("Score"), tr("Risque"), tr("Détection"), tr("Recommandation")});
    QSqlQuery query;
    const QString detectionSql = buildAgriculteurListQuery(QString(), QStringLiteral("ORDER BY A.ID_AGRI DESC"));
    if (!query.exec(detectionSql)) {
        qDebug() << "ERREUR SQL detection agriculteur:" << query.lastError().text() << "REQUETE:" << detectionSql;
        return;
    }
    int i=0;
    while (query.next()) {
        m_tableDetection->insertRow(i);
        const int id=query.value("ID_AGRI").toInt(); const QString nom=query.value("NOM_AGRI").toString();
        const float qt=query.value("QT_STOCK").toFloat(); const float rendement=query.value("RENDE_MOY").toFloat(); const float note=query.value("NOTE_QUALTMOY").toFloat(); const float moyHist=query.value("MOY_HIST").toFloat();
        Agriculteur agri(id, nom, QString(), QString(), QString(), QString(), QString(), 0, QString(), qt, 0, rendement, QString(), note);
        const QStringList vals = {QString::number(id), nom, QString::number(qt), QString::number(agri.calculScore(), 'f', 1) + QStringLiteral(" / 10"), QString::number(agri.calculRisque(), 'f', 1), agri.detectionIntelligente(moyHist), agri.recommandation()};
        for (int c=0;c<vals.size();++c) m_tableDetection->setItem(i,c,new QTableWidgetItem(vals.at(c)));
        ++i;
    }
}

void AgriculteurModule::showDetectionPage()
{
    afficherTableauDetection();
    m_stack->setCurrentIndex(6);
}
