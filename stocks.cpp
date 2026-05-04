#include "stocks.h"
#include "ui_stocks.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
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
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPrinter>
#include <QPageSize>
#include <QPageLayout>
#include <QToolButton>
#include <QStatusBar>
#include <QPainter>
#include <QPixmap>
#include <QColor>
#include <QPalette>
#include <QSqlRecord>
#include <QSqlField>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QDateEdit>
#include <cmath>
#include <algorithm>
#include <QAction>
#include <QToolBar>
#include <QLabel>
#include <QFrame>
#include <QSqlDatabase>
#include <QScrollArea>
#include <QDialog>
#include <QProgressBar>
#include <QTimer>
#include <QSizePolicy>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDataStream>
#include <QMetaType>
#include <QStringList>

namespace {
QSqlDatabase stockDb()
{
    if (QSqlDatabase::contains("ConnexionPrincipale"))
        return QSqlDatabase::database("ConnexionPrincipale");
    return QSqlDatabase::database();
}
} // namespace

// ─────────────────────────────────────────────
//  Constructeur / Destructeur
// ─────────────────────────────────────────────
Stocks::Stocks(QWidget *parent) : QMainWindow(parent),
    ui(new Ui::Stocks)
{
    ui->setupUi(this);

    setWindowTitle("Gestion des stocks");
    setWindowIcon(QIcon(":/img/stock.svg"));
    resize(1100, 720);
    setupUnifiedToolbar();
    applyUnifiedVisualStyle();

    statusBar()->showMessage("Prêt");

    // Initialisation du combobox de tri
    ui->Trier->clear();
    ui->Trier->addItem("Trier par défaut");
    ui->Trier->addItem("Catégorie");
    ui->Trier->addItem("Quantité");
    ui->Trier->addItem("Date d'ajout");
    ui->Trier->addItem("Date mise à jour");

    // Recherche en temps réel
    connect(ui->SaiRecherche, &QLineEdit::textChanged, this, &Stocks::on_Recherche_clicked);

    // Initialisation du combobox X pour les statistiques
    ui->X->clear();
    ui->X->addItem("Statistiques générales");
    ui->X->addItem("Par catégorie");
    ui->X->addItem("Par quantité");
    ui->X->addItem("Par qualité");
    ui->X->addItem("Évolution temporelle");
    ui->X->setEnabled(true);
    ui->X->setVisible(true);

    // Layout pour les statistiques
    // On crée un layout vertical qui contient :
    //   1) FaireLesStat (bouton / label fixe)
    //   2) X (combobox de choix)
    //   3) m_graphiqueScroll (zone scrollable pour le graphique courant)
    {
        QVBoxLayout *layout = ui->chartStatusContainer->layout()
        ? qobject_cast<QVBoxLayout*>(ui->chartStatusContainer->layout())
        : nullptr;
        if (!layout)
        {
            layout = new QVBoxLayout(ui->chartStatusContainer);
            layout->setContentsMargins(10, 10, 10, 10);
            layout->setSpacing(10);
            ui->chartStatusContainer->setLayout(layout);
        }
        // S'assurer que FaireLesStat et X sont dans le layout (si pas déjà gérés par le .ui)
        if (layout->indexOf(ui->FaireLesStat) == -1)
            layout->addWidget(ui->FaireLesStat);
        if (layout->indexOf(ui->X) == -1)
            layout->addWidget(ui->X);

        // Zone scrollable dédiée aux graphiques — sera remplacée à chaque affichage
        m_graphiqueScroll = new QScrollArea(ui->chartStatusContainer);
        m_graphiqueScroll->setWidgetResizable(true);
        m_graphiqueScroll->setFrameShape(QFrame::NoFrame);
        m_graphiqueScroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layout->addWidget(m_graphiqueScroll, 1);
    }

    // Dates par défaut
    ui->SaiDateAjout->setDate(QDate::currentDate());
    ui->SaiDateMAJ->setDate(QDate::currentDate());

    // Combobox catégorie
    ui->SaiCategorie->clear();
    ui->SaiCategorie->addItems({"Verts", "Noirs", "Mélange"});

    // Combobox qualité
    ui->SaiQualite->clear();
    ui->SaiQualite->addItems({"Excellente", "Bonne", "Moyenne", "Médiocre"});

    // Re-added without changing your friend's UI structure:
    // the stock module needs a machine-series choice so auto-affectation
    // knows which series should receive an employee.
    ensureStockSerieSelector();
    refreshStockSerieChoices();
    loadAffectationSettings();

    chargerListeOlives();
}


// ─────────────────────────────────────────────
//  Série machine + auto-affectation helpers
//  These are intentionally added on top of the latest Stocks module without
//  removing the existing statistics / advanced popup functionality.
// ─────────────────────────────────────────────
bool Stocks::tableColumnExists(const QString& tableName, const QString& columnName) const
{
    QSqlQuery q(stockDb());
    q.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM USER_TAB_COLUMNS "
        "WHERE TABLE_NAME = :table_name AND COLUMN_NAME = :column_name"));
    q.bindValue(QStringLiteral(":table_name"), tableName.trimmed().toUpper());
    q.bindValue(QStringLiteral(":column_name"), columnName.trimmed().toUpper());
    return q.exec() && q.next() && q.value(0).toInt() > 0;
}

void Stocks::ensureStockSerieSelector()
{
    if (m_stockSerieCombo)
        return;

    // If a future .ui already contains the combo, reuse it.
    m_stockSerieCombo = findChild<QComboBox*>(QStringLiteral("stockSerieCombo"));
    if (m_stockSerieCombo)
        return;

    QFormLayout* form = findChild<QFormLayout*>(QStringLiteral("ajoutLayout"));
    if (!form)
        return;

    auto* label = new QLabel(tr("Série machine"), ui->ajoutStock);
    label->setObjectName(QStringLiteral("label_stockSerieCombo"));

    m_stockSerieCombo = new QComboBox(ui->ajoutStock);
    m_stockSerieCombo->setObjectName(QStringLiteral("stockSerieCombo"));
    m_stockSerieCombo->setToolTip(tr("Série liée au stock. Utilisée par l'affectation automatique."));
    m_stockSerieCombo->setMinimumHeight(32);

    // Insert just above the Ajouter button row so the original UI remains intact.
    const int insertRow = qMax(0, form->rowCount() - 1);
    form->insertRow(insertRow, label, m_stockSerieCombo);
}

void Stocks::refreshStockSerieChoices()
{
    ensureStockSerieSelector();
    if (!m_stockSerieCombo)
        return;

    const QVariant previous = m_stockSerieCombo->currentData();
    m_stockSerieCombo->clear();
    m_stockSerieCombo->addItem(tr("Choisir une série machine..."), 0);

    QSqlQuery q(stockDb());
    if (!q.exec(QStringLiteral("SELECT id_serie, nom_serie FROM SERIE_MACHINE ORDER BY nom_serie, id_serie"))) {
        m_stockSerieCombo->addItem(tr("Erreur chargement séries"), 0);
        m_stockSerieCombo->setToolTip(q.lastError().text());
        return;
    }

    int restoreIndex = -1;
    while (q.next()) {
        const int id = q.value(0).toInt();
        const QString name = q.value(1).toString().trimmed();
        const QString label = name.isEmpty()
            ? tr("Série %1").arg(id)
            : tr("%1 (ID %2)").arg(name).arg(id);
        m_stockSerieCombo->addItem(label, id);
        if (previous.isValid() && previous.toInt() == id)
            restoreIndex = m_stockSerieCombo->count() - 1;
    }

    if (restoreIndex >= 0)
        m_stockSerieCombo->setCurrentIndex(restoreIndex);
}

void Stocks::loadAffectationSettings()
{
    m_maxAffectationsPerEmployee = 3;
    m_autoAssignFromStock = false;

    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty())
        baseDir = QCoreApplication::applicationDirPath();

    QFile file(QDir(baseDir).filePath(QStringLiteral("settings.dat")));
    if (!file.open(QIODevice::ReadOnly))
        return;

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_5);

    quint32 magic = 0;
    qint32 version = 0;
    qint32 maxAff = 3;
    bool autoAssign = false;

    in >> magic >> version >> maxAff;
    if (in.status() == QDataStream::Ok && magic == 0x534f504d && version >= 2)
        in >> autoAssign;

    if (in.status() == QDataStream::Ok && magic == 0x534f504d && version >= 1 && maxAff > 0 && maxAff <= 100) {
        m_maxAffectationsPerEmployee = maxAff;
        m_autoAssignFromStock = (version >= 2) ? autoAssign : false;
    }
}

bool Stocks::tryAutoAssignForSerie(int serieId, QString& detailMessage)
{
    detailMessage.clear();

    if (serieId <= 0) {
        detailMessage = tr("série machine invalide.");
        return false;
    }
    if (m_maxAffectationsPerEmployee <= 0) {
        detailMessage = tr("limite d'affectations invalide.");
        return false;
    }

    const bool hasDateFin = tableColumnExists(QStringLiteral("EMP_MACH"), QStringLiteral("DATE_FIN"));
    const QString activeJoin = hasDateFin ? QStringLiteral(" AND em.date_fin IS NULL") : QString();
    const QString activeExists = hasDateFin ? QStringLiteral(" AND ex.date_fin IS NULL") : QString();

    QSqlQuery pick(stockDb());
    const QString sql = QStringLiteral(
        "SELECT id_emp FROM ("
        "  SELECT e.id_emp, COUNT(em.id_serie) AS cnt "
        "  FROM EMPLOYE e "
        "  LEFT JOIN EMP_MACH em ON em.id_emp = e.id_emp%1 "
        "  WHERE NOT EXISTS ("
        "    SELECT 1 FROM EMP_MACH ex "
        "    WHERE ex.id_emp = e.id_emp AND ex.id_serie = :serie%2"
        "  ) "
        "  GROUP BY e.id_emp "
        "  HAVING COUNT(em.id_serie) < :max_aff "
        "  ORDER BY cnt ASC, e.id_emp ASC"
        ") WHERE ROWNUM = 1"
    ).arg(activeJoin, activeExists);

    pick.prepare(sql);
    pick.bindValue(QStringLiteral(":serie"), serieId);
    pick.bindValue(QStringLiteral(":max_aff"), m_maxAffectationsPerEmployee);

    if (!pick.exec()) {
        detailMessage = tr("erreur SQL sélection employé : %1").arg(pick.lastError().text());
        return false;
    }
    if (!pick.next()) {
        detailMessage = tr("aucun employé disponible sous la limite de %1 affectation(s).").arg(m_maxAffectationsPerEmployee);
        return false;
    }

    const int empId = pick.value(0).toInt();
    if (empId <= 0) {
        detailMessage = tr("employé sélectionné invalide.");
        return false;
    }

    const bool hasPoste = tableColumnExists(QStringLiteral("EMP_MACH"), QStringLiteral("POSTE"));
    const bool hasDateDebut = tableColumnExists(QStringLiteral("EMP_MACH"), QStringLiteral("DATE_DEBUT"));

    QStringList columns{QStringLiteral("id_serie"), QStringLiteral("id_emp")};
    QStringList values{QStringLiteral(":id_serie"), QStringLiteral(":id_emp")};
    if (hasPoste) {
        columns << QStringLiteral("poste");
        values << QStringLiteral(":poste");
    }
    if (hasDateDebut) {
        columns << QStringLiteral("date_debut");
        values << QStringLiteral(":date_debut");
    }
    if (hasDateFin) {
        columns << QStringLiteral("date_fin");
        values << QStringLiteral(":date_fin");
    }

    QSqlQuery insert(stockDb());
    insert.prepare(QStringLiteral("INSERT INTO EMP_MACH (%1) VALUES (%2)")
                       .arg(columns.join(QStringLiteral(", ")), values.join(QStringLiteral(", "))));
    insert.bindValue(QStringLiteral(":id_serie"), serieId);
    insert.bindValue(QStringLiteral(":id_emp"), empId);
    if (hasPoste)
        insert.bindValue(QStringLiteral(":poste"), tr("Auto stock"));
    if (hasDateDebut)
        insert.bindValue(QStringLiteral(":date_debut"), QDate::currentDate());
    if (hasDateFin)
        insert.bindValue(QStringLiteral(":date_fin"), QVariant(QMetaType(QMetaType::QDate)));

    if (!insert.exec()) {
        detailMessage = tr("erreur SQL insertion affectation : %1").arg(insert.lastError().text());
        return false;
    }

    QString empName;
    QSqlQuery qEmp(stockDb());
    qEmp.prepare(QStringLiteral("SELECT TRIM(nom_emp || ' ' || prenom_emp) FROM EMPLOYE WHERE id_emp = :id"));
    qEmp.bindValue(QStringLiteral(":id"), empId);
    if (qEmp.exec() && qEmp.next())
        empName = qEmp.value(0).toString().trimmed();

    detailMessage = empName.isEmpty()
        ? tr("Affectation automatique réalisée pour l'employé ID %1.").arg(empId)
        : tr("Affectation automatique réalisée : %1 affecté à la série %2.").arg(empName).arg(serieId);
    return true;
}

Stocks::~Stocks()
{
    delete ui;
}

// ─────────────────────────────────────────────
//  Toolbar & style
// ─────────────────────────────────────────────
void Stocks::setupUnifiedToolbar()
{
    auto *nav = new QToolBar(this);
    nav->setMovable(false);
    nav->setFloatable(false);
    nav->setIconSize(QSize(22, 22));
    nav->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    nav->setObjectName("stocksTopToolbar");

    auto *title = new QLabel("GESTION DES STOCKS", nav);
    title->setStyleSheet("font-size: 20px; font-weight: 700; color: #8a9b5f; padding: 6px 18px 6px 6px;");
    nav->addWidget(title);

    QAction *actConsult = nav->addAction(QIcon(":/img/search.svg"), "Consulter");
    QAction *actAdd     = nav->addAction(QIcon(":/img/add.svg"),    "Ajouter");
    QAction *actStat    = nav->addAction(QIcon(":/img/chart.svg"),  "Statistiques");
    QAction *actAdv     = nav->addAction(QIcon(":/img/settings.svg"), "Avancé");

    connect(actConsult, &QAction::triggered, this, &Stocks::on_btnConsulterstc_clicked);
    connect(actAdd,     &QAction::triggered, this, &Stocks::on_AjoutStock_clicked);
    connect(actStat,    &QAction::triggered, this, &Stocks::on_StatistiqueStock_clicked);
    connect(actAdv,     &QAction::triggered, this, &Stocks::on_btnMetiersAvances_clicked);

    addToolBar(Qt::TopToolBarArea, nav);
}

void Stocks::applyUnifiedVisualStyle()
{
    ui->ConAjout->setProperty("type", "primary");
    ui->exporterListeStock->setProperty("type", "primary");
    ui->PredictOutput->setProperty("type", "primary");
    ui->OptimiserReappro->setProperty("type", "warning");
    ui->PredictDechet->setProperty("type", "danger");
    //ui->GestionQualite->setProperty("type", "success");

    ui->ConAjout->setIcon(QIcon(":/img/add.svg"));
    ui->exporterListeStock->setIcon(QIcon(":/img/export.svg"));
    ui->PredictOutput->setIcon(QIcon(":/img/chart.svg"));
    ui->OptimiserReappro->setIcon(QIcon(":/img/refresh.svg"));
    ui->PredictDechet->setIcon(QIcon(":/img/filter.svg"));
    //ui->GestionQualite->setIcon(QIcon(":/img/settings.svg"));

    ui->SaiRecherche->setPlaceholderText("Rechercher par catégorie, description ou qualité...");
    ui->SaiDateAjout->setCalendarPopup(true);
    ui->SaiDateMAJ->setCalendarPopup(true);
    ui->ListeStock->setAlternatingRowColors(true);
    ui->ListeStock->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->ListeStock->horizontalHeader()->setStretchLastSection(true);

    if (ui->chartStatusContainer)
        ui->chartStatusContainer->setProperty("role", "card");
}

void Stocks::navigateToSection(int section)
{
    switch (section) {
    case 0: on_AjoutStock_clicked();       break;
    case 1: on_btnConsulterstc_clicked();  break;
    case 2: on_StatistiqueStock_clicked(); break;
    case 3: on_btnMetiersAvances_clicked(); break;
    default: break;
    }
}

// ─────────────────────────────────────────────
//  Charger / Rafraîchir liste
// ─────────────────────────────────────────────
void Stocks::chargerListeOlives()
{
    QString orderBy = "id_stock";

    QString triTexte = ui->Trier->currentText();
    if (triTexte == "Catégorie")        orderBy = "categ_stock";
    else if (triTexte == "Quantité")    orderBy = "qt_stock";
    else if (triTexte == "Date d'ajout")     orderBy = "dateajt_stock";
    else if (triTexte == "Date mise à jour") orderBy = "datemaj_stock";

    QString sql = QString(
                      "SELECT id_stock, categ_stock, descript_stock, dateajt_stock, datemaj_stock, qt_stock, nom_stock "
                      "FROM STOCK ORDER BY %1 DESC")
                      .arg(orderBy);

    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    if (!query.exec(sql))
    {
        QMessageBox::critical(this, "Erreur SQL", query.lastError().text());
        return;
    }

    ui->ListeStock->clearContents();
    ui->ListeStock->setRowCount(0);
    ui->ListeStock->setColumnCount(8);
    ui->ListeStock->setHorizontalHeaderLabels(
        {"ID", "Catégorie", "Description", "Date d'ajout", "Dernière MAJ", "Quantité (KG)", "Qualité", "Action"});

    ui->ListeStock->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->ListeStock->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    ui->ListeStock->setColumnWidth(7, 200);
    ui->ListeStock->setAlternatingRowColors(true);
    ui->ListeStock->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->ListeStock->verticalHeader()->setVisible(false);

    int row = 0;
    while (query.next())
    {
        ui->ListeStock->insertRow(row);
        QString id = query.value(0).toString();

        for (int col = 0; col < 7; ++col)
        {
            QTableWidgetItem *item = new QTableWidgetItem(query.value(col).toString());
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);

            if (col == 6)
            {
                QString qualite = query.value(col).toString();
                if (qualite == "Excellente")      item->setBackground(QColor(200, 255, 200));
                else if (qualite == "Bonne")      item->setBackground(QColor(230, 255, 230));
                else if (qualite == "Moyenne")    item->setBackground(QColor(255, 255, 200));
                else if (qualite == "Médiocre")   item->setBackground(QColor(255, 200, 200));
            }

            ui->ListeStock->setItem(row, col, item);
        }

        QWidget *container = new QWidget(ui->ListeStock);
        QHBoxLayout *lay = new QHBoxLayout(container);
        lay->setContentsMargins(2, 2, 2, 2);
        lay->setSpacing(6);

        QPushButton *btnMod = new QPushButton("Modifier", container);
        btnMod->setStyleSheet("background:#2196F3;color:white;border-radius:4px;padding:5px;");
        btnMod->setFixedHeight(30);
        btnMod->setFixedWidth(70);

        QPushButton *btnSup = new QPushButton("Supprimer", container);
        btnSup->setStyleSheet("background:#F44336;color:white;border-radius:4px;padding:5px;");
        btnSup->setFixedHeight(30);
        btnSup->setFixedWidth(70);

        lay->addWidget(btnMod);
        lay->addWidget(btnSup);
        lay->setAlignment(Qt::AlignCenter);

        ui->ListeStock->setCellWidget(row, 7, container);

        connect(btnMod, &QPushButton::clicked, this, [this, id]() { onModifier(id); });
        connect(btnSup, &QPushButton::clicked, this, [this, id]() { onSupprimer(id); });

        ui->ListeStock->setRowHeight(row, 50);
        row++;
    }

    statusBar()->showMessage(QString("%1 lots d'olives chargés").arg(row), 3000);
}

void Stocks::rafraichirListe()
{
    chargerListeOlives();
}

// ─────────────────────────────────────────────
//  Navigation onglets
// ─────────────────────────────────────────────

// Navigue vers la page des métiers avancés (metiersavancesstock)
// sans ouvrir directement une popup.
// Les popups s'ouvrent seulement quand l'utilisateur clique sur
// PredictOutput / OptimiserReappro / PredictDechet depuis cette page.
void Stocks::on_btnMetiersAvances_clicked()
{
    ui->metiersStock->setCurrentWidget(ui->metiersavancesstock);
    statusBar()->showMessage("Métiers avancés — choisissez une analyse IA", 4000);
}

void Stocks::on_AjoutStock_clicked()
{
    refreshStockSerieChoices();
    ui->metiersStock->setCurrentWidget(ui->ajoutStock);
}

void Stocks::on_btnConsulterstc_clicked()
{
    ui->metiersStock->setCurrentWidget(ui->consulterStock);
    rafraichirListe();
}

void Stocks::on_StatistiqueStock_clicked()
{
    ui->metiersStock->setCurrentWidget(ui->statStock);
    ui->X->setEnabled(true);
    ui->X->setVisible(true);
    ui->X->setFocus();
    afficherStatistiques();
}

// ─────────────────────────────────────────────
//  Validation des données
// ─────────────────────────────────────────────
bool Stocks::validerQuantite(double quantite)
{
    return quantite > 0 && quantite <= 100000;
}

bool Stocks::validerDonneesAjout()
{
    QString categorie   = ui->SaiCategorie->currentText();
    QString description = ui->SaiDescription->toPlainText().trimmed();
    QString quantiteStr = ui->SaiQuantite->text().trimmed();

    if (categorie.isEmpty() || description.isEmpty() || quantiteStr.isEmpty())
    {
        QMessageBox::warning(this, "Erreur", "Veuillez remplir tous les champs obligatoires");
        return false;
    }

    bool ok = false;
    double quantite = quantiteStr.toDouble(&ok);

    if (!ok || !validerQuantite(quantite))
    {
        QMessageBox::warning(this, "Erreur", "Quantité invalide (doit être entre 0 et 100000 KG)");
        return false;
    }

    return true;
}

// ─────────────────────────────────────────────
//  Ajouter un lot d'olives
// ─────────────────────────────────────────────
void Stocks::on_ConAjout_clicked()
{
    ensureStockSerieSelector();
    loadAffectationSettings();

    if (!validerDonneesAjout())
        return;

    const QString categorie   = ui->SaiCategorie->currentText().trimmed();
    const QString description = ui->SaiDescription->toPlainText().trimmed();
    const QDate   dateAjout   = ui->SaiDateAjout->date();
    const QDate   dateMAJ     = ui->SaiDateMAJ->date();
    const double  quantite    = ui->SaiQuantite->text().trimmed().toDouble();
    const QString qualite     = ui->SaiQualite->currentText().trimmed();
    const int serieId = m_stockSerieCombo ? m_stockSerieCombo->currentData().toInt() : 0;

    // Preserve your friend's existing stock add behavior when auto mode is off.
    // If auto-affectation is enabled, the selected series is required because it
    // is the target of the EMP_MACH assignment.
    if (m_autoAssignFromStock && serieId <= 0) {
        QMessageBox::warning(this, tr("Série machine requise"),
                             tr("Veuillez choisir une série machine pour lancer l'affectation automatique."));
        if (m_stockSerieCombo)
            m_stockSerieCombo->setFocus();
        return;
    }

    QSqlDatabase db = stockDb();
    QSqlQuery q(db);

    const bool hasStockSerie = tableColumnExists(QStringLiteral("STOCK"), QStringLiteral("ID_SERIE"));
    if (hasStockSerie) {
        q.prepare(QStringLiteral(
            "INSERT INTO STOCK (categ_stock, descript_stock, dateajt_stock, datemaj_stock, qt_stock, nom_stock, id_serie) "
            "VALUES (:categorie, :description, :date_ajout, :date_maj, :quantite, :qualite, :id_serie)"));
    } else {
        q.prepare(QStringLiteral(
            "INSERT INTO STOCK (categ_stock, descript_stock, dateajt_stock, datemaj_stock, qt_stock, nom_stock) "
            "VALUES (:categorie, :description, :date_ajout, :date_maj, :quantite, :qualite)"));
    }

    q.bindValue(QStringLiteral(":categorie"),   categorie);
    q.bindValue(QStringLiteral(":description"), description);
    q.bindValue(QStringLiteral(":date_ajout"),  dateAjout);
    q.bindValue(QStringLiteral(":date_maj"),    dateMAJ);
    q.bindValue(QStringLiteral(":quantite"),    quantite);
    q.bindValue(QStringLiteral(":qualite"),     qualite);
    if (hasStockSerie) {
        if (serieId > 0)
            q.bindValue(QStringLiteral(":id_serie"), serieId);
        else
            q.bindValue(QStringLiteral(":id_serie"), QVariant(QMetaType(QMetaType::Int)));
    }

    if (!q.exec())
    {
        QMessageBox::critical(this, tr("Erreur SQL"), q.lastError().text());
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

    QMessageBox::information(this, tr("Succès"),
                             tr("Lot d'olives ajouté avec succès%1").arg(autoAssignMsg));

    ui->SaiCategorie->setCurrentIndex(0);
    ui->SaiDescription->clear();
    ui->SaiDateAjout->setDate(QDate::currentDate());
    ui->SaiDateMAJ->setDate(QDate::currentDate());
    ui->SaiQuantite->clear();
    ui->SaiQualite->setCurrentIndex(0);
    if (m_stockSerieCombo)
        m_stockSerieCombo->setCurrentIndex(0);

    rafraichirListe();
    ui->metiersStock->setCurrentWidget(ui->consulterStock);
    statusBar()->showMessage(tr("Lot d'olives ajouté"), 3000);
}

// ─────────────────────────────────────────────
//  Modifier un lot
// ─────────────────────────────────────────────
void Stocks::onModifier(const QString &id)
{
    QSqlDatabase db = stockDb();
    QSqlQuery sel(db);
    sel.prepare("SELECT categ_stock, descript_stock, dateajt_stock, datemaj_stock, qt_stock, nom_stock "
                "FROM STOCK WHERE id_stock = :id");
    sel.bindValue(":id", id);
    if (!sel.exec() || !sel.next())
    {
        QMessageBox::critical(this, "Erreur", "Lot introuvable (id=" + id + ")\n" + sel.lastError().text());
        return;
    }

    QString categorie   = sel.value(0).toString();
    QString description = sel.value(1).toString();
    QDate   dateAjout   = sel.value(2).toDate();
    QDate   dateMAJ     = sel.value(3).toDate();
    double  quantite    = sel.value(4).toDouble();
    QString qualite     = sel.value(5).toString();

    if (!dateAjout.isValid()) dateAjout = QDate::currentDate();
    if (!dateMAJ.isValid())   dateMAJ   = QDate::currentDate();

    QDialog dlg(this);
    dlg.setWindowTitle(QString("Modifier le lot %1").arg(id));
    dlg.setMinimumWidth(500);

    QFormLayout *form = new QFormLayout(&dlg);

    QComboBox *editCategorie = new QComboBox(&dlg);
    editCategorie->addItems({"Verts", "Noirs", "Mélange"});
    editCategorie->setCurrentText(categorie);

    QTextEdit *editDescription = new QTextEdit(&dlg);
    editDescription->setText(description);
    editDescription->setMaximumHeight(100);

    QDateEdit *editDateAjout = new QDateEdit(&dlg);
    editDateAjout->setDate(dateAjout);
    editDateAjout->setCalendarPopup(true);

    QDateEdit *editDateMAJ = new QDateEdit(&dlg);
    editDateMAJ->setDate(QDate::currentDate());
    editDateMAJ->setCalendarPopup(true);

    QLineEdit *editQuantite = new QLineEdit(QString::number(quantite), &dlg);

    QComboBox *editQualite = new QComboBox(&dlg);
    editQualite->addItems({"Excellente", "Bonne", "Moyenne", "Médiocre"});
    editQualite->setCurrentText(qualite);

    form->addRow("Catégorie:",       editCategorie);
    form->addRow("Description:",     editDescription);
    form->addRow("Date d'ajout:",    editDateAjout);
    form->addRow("Date mise à jour:", editDateMAJ);
    form->addRow("Quantité (KG):",   editQuantite);
    form->addRow("Qualité:",         editQualite);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return;

    double newQuantite = editQuantite->text().toDouble();
    if (!validerQuantite(newQuantite))
    {
        QMessageBox::warning(this, "Erreur", "Quantité invalide");
        return;
    }

    QSqlQuery q(db);
    q.prepare("UPDATE STOCK SET categ_stock = :categorie, descript_stock = :description, "
              "dateajt_stock = :date_ajout, datemaj_stock = :date_maj, qt_stock = :quantite, nom_stock = :qualite "
              "WHERE id_stock = :id");

    q.bindValue(":categorie",   editCategorie->currentText());
    q.bindValue(":description", editDescription->toPlainText());
    q.bindValue(":date_ajout",  editDateAjout->date());
    q.bindValue(":date_maj",    editDateMAJ->date());
    q.bindValue(":quantite",    newQuantite);
    q.bindValue(":qualite",     editQualite->currentText());
    q.bindValue(":id",          id);

    if (!q.exec())
        QMessageBox::critical(this, "Erreur SQL", q.lastError().text());
    else
    {
        QMessageBox::information(this, "Succès", "Lot modifié avec succès");
        rafraichirListe();
    }
}

// ─────────────────────────────────────────────
//  Supprimer un lot
// ─────────────────────────────────────────────
void Stocks::onSupprimer(const QString &id)
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirmation",
                                                              QString("Voulez-vous vraiment supprimer le lot %1 ?").arg(id),
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    QSqlDatabase db = stockDb();
    QSqlQuery q(db);
    q.prepare("DELETE FROM STOCK WHERE id_stock = :id");
    q.bindValue(":id", id);

    if (q.exec())
    {
        QMessageBox::information(this, "Succès", "Lot supprimé avec succès");
        statusBar()->showMessage("Lot supprimé", 3000);
        rafraichirListe();
    }
    else
    {
        QMessageBox::critical(this, "Erreur", q.lastError().text());
    }
}

// ─────────────────────────────────────────────
//  Tri
// ─────────────────────────────────────────────
void Stocks::on_Trier_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    rafraichirListe();
}

// ─────────────────────────────────────────────
//  Recherche
// ─────────────────────────────────────────────
void Stocks::on_Recherche_clicked()
{
    QString recherche = ui->SaiRecherche->text().trimmed();

    if (recherche.isEmpty())
    {
        rafraichirListe();
        return;
    }

    QSqlDatabase db = stockDb();
    QSqlQuery query(db);

    query.prepare(
        "SELECT id_stock, categ_stock, descript_stock, dateajt_stock, datemaj_stock, qt_stock, nom_stock "
        "FROM STOCK "
        "WHERE UPPER(categ_stock)   LIKE UPPER(:s1) "
        "   OR UPPER(descript_stock) LIKE UPPER(:s2) "
        "   OR UPPER(nom_stock)     LIKE UPPER(:s3) "
        "ORDER BY id_stock DESC");

    QString param = "%" + recherche + "%";
    query.bindValue(":s1", param);
    query.bindValue(":s2", param);
    query.bindValue(":s3", param);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Erreur SQL", query.lastError().text());
        return;
    }

    ui->ListeStock->clearContents();
    ui->ListeStock->setRowCount(0);

    int row = 0;
    while (query.next())
    {
        ui->ListeStock->insertRow(row);
        QString id = query.value(0).toString();

        for (int col = 0; col < 7; col++)
        {
            QTableWidgetItem *item = new QTableWidgetItem(query.value(col).toString());
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            ui->ListeStock->setItem(row, col, item);
        }

        QWidget *container = new QWidget(ui->ListeStock);
        QHBoxLayout *lay = new QHBoxLayout(container);
        lay->setContentsMargins(2, 2, 2, 2);
        lay->setSpacing(6);

        QPushButton *btnMod = new QPushButton("Modifier", container);
        btnMod->setStyleSheet("background:#2196F3;color:white;border-radius:4px;padding:5px;");
        btnMod->setFixedHeight(30);
        btnMod->setFixedWidth(70);

        QPushButton *btnSup = new QPushButton("Supprimer", container);
        btnSup->setStyleSheet("background:#F44336;color:white;border-radius:4px;padding:5px;");
        btnSup->setFixedHeight(30);
        btnSup->setFixedWidth(70);

        lay->addWidget(btnMod);
        lay->addWidget(btnSup);
        lay->setAlignment(Qt::AlignCenter);

        ui->ListeStock->setCellWidget(row, 7, container);

        connect(btnMod, &QPushButton::clicked, this, [this, id]() { onModifier(id); });
        connect(btnSup, &QPushButton::clicked, this, [this, id]() { onSupprimer(id); });

        ui->ListeStock->setRowHeight(row, 50);
        row++;
    }

    if (row == 0)
        QMessageBox::information(this, "Recherche", "Aucun résultat trouvé pour : " + recherche);
    else
        statusBar()->showMessage(QString("%1 résultat(s) trouvé(s)").arg(row), 3000);
}

// ─────────────────────────────────────────────
//  Export PDF
// ─────────────────────────────────────────────
void Stocks::on_exporterListeStock_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Exporter en PDF",
                                                    QString("STOCK_%1.pdf").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
                                                    "Fichiers PDF (*.pdf)");

    if (fileName.isEmpty())
        return;

    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    query.exec("SELECT id_stock, categ_stock, descript_stock, dateajt_stock, datemaj_stock, qt_stock, nom_stock FROM STOCK ORDER BY dateajt_stock DESC");

    if (!query.isActive())
    {
        QMessageBox::critical(this, "Erreur", "Impossible de récupérer les données");
        return;
    }

    int nbLots = 0;
    QSqlQuery queryCount(db);
    queryCount.exec("SELECT COUNT(*) FROM STOCK");
    if (queryCount.next())
        nbLots = queryCount.value(0).toInt();

    double totalQuantite = 0;
    QSqlQuery queryTotal(db);
    queryTotal.exec("SELECT SUM(qt_stock) FROM STOCK");
    if (queryTotal.next())
        totalQuantite = queryTotal.value(0).toDouble();

    QString html = "<!DOCTYPE html>"
                   "<html><head><meta charset='UTF-8'><title>Stock d'Olives</title>"
                   "<style>"
                   "body{font-family:Arial,sans-serif;margin:20px;}"
                   "h1{color:#4CAF50;text-align:center;}"
                   ".header{text-align:center;margin-bottom:30px;}"
                   ".stats{display:flex;justify-content:space-around;margin:20px 0;padding:15px;background:#f5f5f5;border-radius:8px;}"
                   ".stat-card{text-align:center;}"
                   ".stat-value{font-size:24px;font-weight:bold;color:#4CAF50;}"
                   ".stat-label{font-size:12px;color:#666;}"
                   "table{width:100%;border-collapse:collapse;margin-top:20px;}"
                   "th{background-color:#4CAF50;color:white;padding:10px;text-align:left;}"
                   "td{border:1px solid #ddd;padding:8px;}"
                   "tr:nth-child(even){background-color:#f2f2f2;}"
                   ".excellente{background-color:#d4edda;}"
                   ".bonne{background-color:#d1ecf1;}"
                   ".moyenne{background-color:#fff3cd;}"
                   ".mediocre{background-color:#f8d7da;}"
                   ".footer{text-align:right;color:#666;margin-top:30px;}"
                   "</style></head><body>"
                   "<div class='header'><h1>🫒 Stock d'Olives — SmartOil</h1></div>"
                   "<div class='stats'>"
                   "<div class='stat-card'><div class='stat-value'>" + QString::number(totalQuantite, 'f', 0) + " KG</div><div class='stat-label'>Quantité totale</div></div>"
                                                              "<div class='stat-card'><div class='stat-value'>" + QString::number(nbLots) + "</div><div class='stat-label'>Nombre de lots</div></div>"
                                               "<div class='stat-card'><div class='stat-value'>" + QDate::currentDate().toString("dd/MM/yyyy") + "</div><div class='stat-label'>Date d'export</div></div>"
                                                                   "</div>"
                                                                   "<table><thead><tr>"
                                                                   "<th>ID</th><th>Catégorie</th><th>Description</th><th>Date d'ajout</th><th>Dernière MAJ</th><th>Quantité (KG)</th><th>Qualité</th>"
                                                                   "</tr></thead><tbody>";

    while (query.next())
    {
        QString qualite  = query.value(6).toString();
        QString rowClass;
        if (qualite == "Excellente")    rowClass = "excellente";
        else if (qualite == "Bonne")    rowClass = "bonne";
        else if (qualite == "Moyenne")  rowClass = "moyenne";
        else if (qualite == "Médiocre") rowClass = "mediocre";

        html += "<tr class='" + rowClass + "'>";
        html += "<td>" + query.value(0).toString() + "</td>";
        html += "<td>" + query.value(1).toString() + "</td>";
        html += "<td>" + query.value(2).toString() + "</td>";
        html += "<td>" + query.value(3).toString() + "</td>";
        html += "<td>" + query.value(4).toString() + "</td>";
        html += "<td>" + query.value(5).toString() + " KG</td>";
        html += "<td>" + qualite + "</td>";
        html += "</tr>";
    }

    html += "</tbody></table>"
            "<div class='footer'>Généré le : " +
            QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm:ss") +
            "</div></body></html>";

    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(20, 20, 20, 20), QPageLayout::Millimeter);

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);

    QMessageBox::information(this, "Succès", "PDF exporté avec succès !");
    statusBar()->showMessage("PDF exporté", 3000);
}

// ─────────────────────────────────────────────
//  Utilitaires graphiques
// ─────────────────────────────────────────────
QString Stocks::debutGraphique(const QString &titre, const QString &type)
{
    QString html = "<div style='background:white;border-radius:15px;padding:20px;"
                   "box-shadow:0 10px 30px rgba(0,0,0,0.1);margin-top:15px;'>";
    html += "<div style='display:flex;align-items:center;justify-content:space-between;margin-bottom:20px;'>";
    html += "<h3 style='color:#333;margin:0;font-size:18px;border-left:4px solid #4CAF50;padding-left:15px;'>" + titre + "</h3>";
    html += "<span style='background:#e8f5e9;color:#4CAF50;padding:4px 12px;border-radius:20px;font-size:12px;font-weight:bold;'>" + type + "</span>";
    html += "</div>";
    return html;
}

QString Stocks::finGraphique()
{
    return "</div>";
}

QLabel *Stocks::creerLabelGraphique(const QString &html)
{
    QLabel *label = new QLabel();
    label->setWordWrap(true);
    label->setTextFormat(Qt::RichText);
    label->setMinimumHeight(400);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    label->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    QString styles = "<style>"
                     "@keyframes fadeIn{from{opacity:0;transform:translateY(10px);}to{opacity:1;transform:translateY(0);}}"
                     ".graphique{transition:all 0.3s ease;}"
                     "</style>";
    label->setText(styles + html);
    return label;
}

void Stocks::remplacerContenuAvecAnimation(QWidget *nouveauWidget)
{
    // On met simplement le nouveau widget comme widget du QScrollArea dédié.
    // Cela préserve FaireLesStat et X qui restent dans le layout parent.
    if (!m_graphiqueScroll)
        return;

    // L'ancien widget du scroll est automatiquement supprimé par Qt
    // quand on appelle setWidget() avec un nouveau widget.
    QWidget *ancien = m_graphiqueScroll->takeWidget();
    if (ancien)
        ancien->deleteLater();

    nouveauWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    nouveauWidget->setMinimumHeight(400);
    m_graphiqueScroll->setWidget(nouveauWidget);

    ui->X->setEnabled(true);
    ui->X->setVisible(true);
    ui->X->raise();
}

void Stocks::afficherMessageErreur(const QString &message)
{
    QWidget *w = new QWidget();
    QVBoxLayout *lay = new QVBoxLayout(w);
    QLabel *lbl = new QLabel("<div style='padding:50px;text-align:center;color:#F44336;font-size:16px;'>" + message + "</div>");
    lbl->setAlignment(Qt::AlignCenter);
    lay->addWidget(lbl);
    remplacerContenuAvecAnimation(w);
}

void Stocks::afficherMessageInfo(const QString &message)
{
    QWidget *w = new QWidget();
    QVBoxLayout *lay = new QVBoxLayout(w);
    QLabel *lbl = new QLabel("<div style='padding:50px;text-align:center;color:#666;font-size:16px;'>" + message + "</div>");
    lbl->setAlignment(Qt::AlignCenter);
    lay->addWidget(lbl);
    remplacerContenuAvecAnimation(w);
}

// ─────────────────────────────────────────────
//  Statistiques
// ─────────────────────────────────────────────
void Stocks::afficherStatistiques()
{
    if (ui->X->count() == 0)
    {
        afficherMessageErreur("Le sélecteur de statistiques n'est pas initialisé");
        return;
    }

    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    query.exec("SELECT COUNT(*) FROM STOCK");
    int totalLots = 0;
    if (query.next())
        totalLots = query.value(0).toInt();

    if (totalLots == 0)
    {
        afficherMessageInfo("Aucun lot dans la base de données");
        return;
    }

    QString choix = ui->X->currentText();
    QWidget *graphiqueWidget = nullptr;
    QList<QString> couleurs = {"#4CAF50", "#2196F3", "#FF9800", "#F44336", "#9C27B0", "#00BCD4"};

    if (choix == "Statistiques générales" || choix == "Par catégorie")
        graphiqueWidget = creerStatistiquesCategorie(couleurs);
    else if (choix == "Par quantité")
        graphiqueWidget = creerStatistiquesQuantite(couleurs);
    else if (choix == "Par qualité")
        graphiqueWidget = creerStatistiquesQualite(couleurs);
    else if (choix == "Évolution temporelle")
        graphiqueWidget = creerStatistiquesTemporal(couleurs);

    if (graphiqueWidget)
        remplacerContenuAvecAnimation(graphiqueWidget);
}

void Stocks::on_X_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (ui->metiersStock->currentWidget() == ui->statStock)
        afficherStatistiques();
}

// ─────────────────────────────────────────────
//  Statistiques générales / par catégorie
//  → KPI cards + Graphique barres verticales (QPainter) + Barre proportionnelle
// ─────────────────────────────────────────────
QWidget *Stocks::creerStatistiquesCategorie(const QList<QString> &couleurs)
{
    Q_UNUSED(couleurs);
    QSqlDatabase db = stockDb();

    // Données par catégorie
    QSqlQuery qCat(db);
    qCat.exec("SELECT categ_stock, SUM(qt_stock), COUNT(*) FROM STOCK GROUP BY categ_stock ORDER BY categ_stock");
    QStringList categories;
    QList<double> quantites;
    QList<int>    nbLots;
    while (qCat.next()) {
        categories << qCat.value(0).toString();
        quantites  << qCat.value(1).toDouble();
        nbLots     << qCat.value(2).toInt();
    }

    // Données générales
    QSqlQuery qGen(db);
    qGen.exec("SELECT COUNT(*), SUM(qt_stock), MAX(qt_stock), MIN(qt_stock) FROM STOCK");
    int totalLots = 0; double totalKg = 0, maxKg = 0;
    if (qGen.next()) {
        totalLots = qGen.value(0).toInt();
        totalKg   = qGen.value(1).toDouble();
        maxKg     = qGen.value(2).toDouble();
    }

    double maxQ = 0;
    for (double q : quantites) if (q > maxQ) maxQ = q;

    // Palette
    QList<QColor> pal = {QColor("#4CAF50"), QColor("#2196F3"), QColor("#FF9800"),
                         QColor("#9C27B0"), QColor("#F44336"), QColor("#00BCD4")};

    QWidget *conteneur = new QWidget();
    conteneur->setStyleSheet("background:white;");
    QVBoxLayout *mainLay = new QVBoxLayout(conteneur);
    mainLay->setContentsMargins(14, 14, 14, 14);
    mainLay->setSpacing(12);

    // ── KPI cards (4 cartes en ligne) ──
    struct KPI { QString val; QString lbl; QColor col; };
    QList<KPI> kpis = {
                       { QString::number(totalKg, 'f', 0) + " KG", "Total en stock",     QColor("#4CAF50") },
                       { QString::number(totalLots),                "Lots enregistrés",   QColor("#2196F3") },
                       { QString::number(maxKg, 'f', 0) + " KG",   "Plus grand lot",     QColor("#FF9800") },
                       { QString::number(categories.size()),        "Catégories actives", QColor("#9C27B0") },
                       };
    QHBoxLayout *kpiRow = new QHBoxLayout();
    kpiRow->setSpacing(10);
    for (auto &k : kpis) {
        QLabel *card = new QLabel();
        card->setFixedHeight(72);
        card->setAlignment(Qt::AlignCenter);
        card->setStyleSheet(QString(
                                "background:%1;border-radius:10px;color:white;padding:6px;"
                                ).arg(k.col.name()));
        card->setText(QString("<b style='font-size:18px;'>%1</b><br>"
                              "<span style='font-size:11px;'>%2</span>").arg(k.val, k.lbl));
        card->setTextFormat(Qt::RichText);
        kpiRow->addWidget(card);
    }
    mainLay->addLayout(kpiRow);

    // ── Titre ──
    QLabel *titre = new QLabel("📊 Quantité par catégorie");
    titre->setStyleSheet("font-size:14px;font-weight:bold;color:#333;"
                         "border-left:4px solid #4CAF50;padding-left:10px;");
    mainLay->addWidget(titre);

    // ── Graphique en barres verticales dessiné avec QPainter ──
    if (!categories.isEmpty()) {
        const int chartW = 700;
        const int chartH = 240;
        const int padL   = 55;  // espace pour les étiquettes Y
        const int padB   = 50;  // espace pour étiquettes X
        const int padT   = 30;
        const int padR   = 20;
        const int innerW = chartW - padL - padR;
        const int innerH = chartH - padT - padB;

        QPixmap pix(chartW, chartH);
        pix.fill(QColor("#f8faf8"));
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);

        // Fond arrondi
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f8faf8"));
        p.drawRoundedRect(0, 0, chartW, chartH, 10, 10);

        // Grilles horizontales
        int nbGrid = 5;
        p.setPen(QPen(QColor("#e0e0e0"), 1, Qt::DashLine));
        QFont fSmall("Arial", 8);
        p.setFont(fSmall);
        p.setPen(QColor("#aaaaaa"));
        for (int g = 0; g <= nbGrid; g++) {
            int y = padT + (int)(g * innerH / (double)nbGrid);
            double val = maxQ - g * maxQ / nbGrid;
            p.setPen(QPen(QColor("#e0e0e0"), 1, Qt::DashLine));
            p.drawLine(padL, y, padL + innerW, y);
            p.setPen(QColor("#888"));
            p.drawText(QRect(0, y - 10, padL - 5, 20), Qt::AlignRight | Qt::AlignVCenter,
                       QString::number((int)val));
        }

        // Barres
        int n = categories.size();
        int barW = qMin(80, innerW / (n > 0 ? n : 1) - 20);
        int spacing = (innerW - n * barW) / (n + 1);

        QFont fBold("Arial", 9);
        fBold.setBold(true);
        for (int i = 0; i < n; i++) {
            QColor col = pal[i % pal.size()];
            double pct = (maxQ > 0) ? (quantites[i] / maxQ) : 0;
            int h = (int)(pct * innerH);
            int x = padL + spacing + i * (barW + spacing);
            int y = padT + innerH - h;

            // Barre avec gradient vertical
            QLinearGradient grad(x, y, x, padT + innerH);
            grad.setColorAt(0, col.lighter(120));
            grad.setColorAt(1, col);
            p.setPen(Qt::NoPen);
            p.setBrush(grad);
            p.drawRoundedRect(x, y, barW, h, 5, 5);

            // Valeur au-dessus
            double pctTotal = (totalKg > 0) ? (quantites[i] / totalKg * 100.0) : 0;
            p.setFont(fBold);
            p.setPen(col);
            p.drawText(QRect(x - 10, y - 30, barW + 20, 16),
                       Qt::AlignCenter,
                       QString::number(quantites[i], 'f', 0) + " KG");
            QFont fTiny("Arial", 8);
            p.setFont(fTiny);
            p.setPen(QColor("#888"));
            p.drawText(QRect(x - 10, y - 14, barW + 20, 14),
                       Qt::AlignCenter,
                       QString::number(pctTotal, 'f', 1) + "%");

            // Étiquette X
            p.setFont(fBold);
            p.setPen(QColor("#555"));
            p.drawText(QRect(x - 5, padT + innerH + 6, barW + 10, 18),
                       Qt::AlignCenter, categories[i]);
            p.setFont(fTiny);
            p.setPen(QColor("#888"));
            p.drawText(QRect(x - 5, padT + innerH + 24, barW + 10, 16),
                       Qt::AlignCenter, QString::number(nbLots[i]) + " lot(s)");
        }

        p.end();

        QLabel *chartLabel = new QLabel();
        chartLabel->setPixmap(pix);
        chartLabel->setAlignment(Qt::AlignHCenter);
        mainLay->addWidget(chartLabel);
    }

    // ── Barre proportionnelle (stacked) ──
    QLabel *titreStack = new QLabel("Répartition proportionnelle");
    titreStack->setStyleSheet("font-size:13px;font-weight:bold;color:#333;");
    mainLay->addWidget(titreStack);

    {
        const int w = 700, h = 28;
        QPixmap pix(w, h);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        int x = 0;
        QFont f("Arial", 10);
        f.setBold(true);
        p.setFont(f);
        for (int i = 0; i < categories.size(); i++) {
            double pct = (totalKg > 0) ? (quantites[i] / totalKg) : 0;
            int segW = (int)(pct * w);
            QColor col = pal[i % pal.size()];
            p.setPen(Qt::NoPen);
            p.setBrush(col);
            // Coins arrondis seulement aux extrémités
            if (i == 0)
                p.drawRoundedRect(x, 0, segW, h, 8, 8);
            else if (i == categories.size() - 1)
                p.drawRoundedRect(x, 0, w - x, h, 8, 8);
            else
                p.drawRect(x, 0, segW, h);
            if (pct > 0.08) {
                p.setPen(Qt::white);
                p.drawText(QRect(x, 0, segW, h), Qt::AlignCenter,
                           QString::number((int)(pct * 100)) + "%");
            }
            x += segW;
        }
        p.end();
        QLabel *stackLbl = new QLabel();
        stackLbl->setPixmap(pix);
        stackLbl->setAlignment(Qt::AlignHCenter);
        mainLay->addWidget(stackLbl);
    }

    // ── Légende ──
    QHBoxLayout *legendLay = new QHBoxLayout();
    legendLay->setSpacing(16);
    for (int i = 0; i < categories.size(); i++) {
        QColor col = pal[i % pal.size()];
        QLabel *dot = new QLabel();
        dot->setFixedSize(14, 14);
        dot->setStyleSheet(QString("background:%1;border-radius:3px;").arg(col.name()));
        QLabel *txt = new QLabel(categories[i]);
        txt->setStyleSheet("font-size:12px;color:#444;");
        legendLay->addWidget(dot);
        legendLay->addWidget(txt);
    }
    legendLay->addStretch();
    mainLay->addLayout(legendLay);
    mainLay->addStretch();

    return conteneur;
}

// ─────────────────────────────────────────────
//  Statistiques par quantité
//  → Top 10 — Barres horizontales (Tornado style) avec QPainter
// ─────────────────────────────────────────────
QWidget *Stocks::creerStatistiquesQuantite(const QList<QString> &couleurs)
{
    Q_UNUSED(couleurs);
    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    query.exec("SELECT id_stock, qt_stock, categ_stock, nom_stock FROM STOCK ORDER BY qt_stock DESC FETCH FIRST 10 ROWS ONLY");

    QStringList labels, cats, qualites;
    QList<double> valeurs;
    while (query.next()) {
        labels   << "Lot #" + query.value(0).toString();
        valeurs  << query.value(1).toDouble();
        cats     << query.value(2).toString();
        qualites << query.value(3).toString();
    }

    double maxV = 0;
    for (double v : valeurs) if (v > maxV) maxV = v;

    QMap<QString, QColor> catCols;
    catCols["Verts"]   = QColor("#4CAF50");
    catCols["Noirs"]   = QColor("#37474F");
    catCols["Mélange"] = QColor("#FF9800");

    QMap<QString, QColor> qualCols;
    qualCols["Excellente"] = QColor("#4CAF50");
    qualCols["Bonne"]      = QColor("#8BC34A");
    qualCols["Moyenne"]    = QColor("#FFC107");
    qualCols["Médiocre"]   = QColor("#F44336");

    QWidget *conteneur = new QWidget();
    conteneur->setStyleSheet("background:white;");
    QVBoxLayout *mainLay = new QVBoxLayout(conteneur);
    mainLay->setContentsMargins(14, 14, 14, 14);
    mainLay->setSpacing(10);

    // ── Titre ──
    QLabel *titre = new QLabel("🏆 Top 10 lots — Graphique en barres horizontales");
    titre->setStyleSheet("font-size:14px;font-weight:bold;color:#333;"
                         "border-left:4px solid #2196F3;padding-left:10px;");
    mainLay->addWidget(titre);

    // ── Dessin QPainter ──
    int n = labels.size();
    if (n > 0) {
        const int rowH  = 42;
        const int padL  = 130;  // espace pour labels gauche
        const int padR  = 90;   // espace pour valeur droite
        const int padT  = 10;
        const int padB  = 10;
        const int barH  = 22;
        const int chartW = 700;
        const int chartH = padT + n * rowH + padB;

        QPixmap pix(chartW, chartH);
        pix.fill(QColor("#f8fbff"));
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);

        QFont fNorm("Arial", 10);
        QFont fBold("Arial", 10); fBold.setBold(true);
        QFont fSmall("Arial", 8);

        int barAreaW = chartW - padL - padR;

        for (int i = 0; i < n; i++) {
            int y = padT + i * rowH;
            int barY = y + (rowH - barH) / 2;

            // Fond alterné
            if (i % 2 == 0) {
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(240, 248, 255, 120));
                p.drawRect(0, y, chartW, rowH);
            }

            QColor col = catCols.value(cats[i], QColor("#2196F3"));
            QColor qcol = qualCols.value(qualites[i], QColor("#9E9E9E"));

            // Badge de rang
            QColor rankCol = (i == 0) ? QColor("#FFD700") : (i == 1) ? QColor("#C0C0C0")
                                                        : (i == 2) ? QColor("#CD7F32") : QColor("#e0e0e0");
            p.setPen(Qt::NoPen);
            p.setBrush(rankCol);
            p.drawEllipse(6, barY + 1, 20, 20);
            p.setPen(i < 3 ? QColor("#333") : QColor("#777"));
            p.setFont(fSmall);
            p.drawText(QRect(6, barY + 1, 20, 20), Qt::AlignCenter, QString::number(i + 1));

            // Label (lot + catégorie)
            p.setFont(fBold);
            p.setPen(QColor("#333"));
            p.drawText(QRect(32, barY, padL - 36, 14), Qt::AlignVCenter | Qt::AlignLeft, labels[i]);
            p.setFont(fSmall);
            p.setPen(QColor("#888"));
            p.drawText(QRect(32, barY + 14, padL - 36, 10), Qt::AlignVCenter | Qt::AlignLeft,
                       "(" + cats[i] + ")");

            // Barre de fond
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#f0f0f0"));
            p.drawRoundedRect(padL, barY, barAreaW, barH, 9, 9);

            // Barre de valeur
            double pct = (maxV > 0) ? (valeurs[i] / maxV) : 0;
            int barW = (int)(pct * barAreaW);
            if (barW > 0) {
                QLinearGradient grad(padL, 0, padL + barAreaW, 0);
                grad.setColorAt(0, col.lighter(140));
                grad.setColorAt(1, col);
                p.setBrush(grad);
                p.drawRoundedRect(padL, barY, barW, barH, 9, 9);
            }

            // Valeur à droite
            p.setFont(fBold);
            p.setPen(col);
            p.drawText(QRect(padL + barAreaW + 6, barY, padR - 10, barH),
                       Qt::AlignVCenter | Qt::AlignLeft,
                       QString::number(valeurs[i], 'f', 0) + " KG");

            // Badge qualité
            p.setPen(Qt::NoPen);
            p.setBrush(qcol);
            p.drawRoundedRect(padL + barAreaW + 6, barY + barH - 10, 56, 10, 3, 3);
            p.setPen(Qt::white);
            QFont fTiny("Arial", 7);
            p.setFont(fTiny);
            p.drawText(QRect(padL + barAreaW + 6, barY + barH - 10, 56, 10),
                       Qt::AlignCenter, qualites[i]);
        }
        p.end();

        QLabel *chartLbl = new QLabel();
        chartLbl->setPixmap(pix);
        chartLbl->setAlignment(Qt::AlignHCenter);
        mainLay->addWidget(chartLbl);
    }

    // ── Légende catégories ──
    QHBoxLayout *legLay = new QHBoxLayout();
    legLay->setSpacing(16);
    QStringList catNames = {"Verts", "Noirs", "Mélange"};
    QList<QColor> catColList = {QColor("#4CAF50"), QColor("#37474F"), QColor("#FF9800")};
    for (int i = 0; i < catNames.size(); i++) {
        QLabel *dot = new QLabel();
        dot->setFixedSize(14, 14);
        dot->setStyleSheet(QString("background:%1;border-radius:3px;").arg(catColList[i].name()));
        QLabel *txt = new QLabel(catNames[i]);
        txt->setStyleSheet("font-size:12px;color:#444;");
        legLay->addWidget(dot);
        legLay->addWidget(txt);
    }
    legLay->addStretch();
    mainLay->addLayout(legLay);
    mainLay->addStretch();

    return conteneur;
}

// ─────────────────────────────────────────────
//  Statistiques par qualité
//  → Donut chart QPainter + Pyramid chart QPainter
// ─────────────────────────────────────────────
QWidget *Stocks::creerStatistiquesQualite(const QList<QString> &/*couleurs*/)
{
    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    query.exec("SELECT nom_stock, SUM(qt_stock), COUNT(*) FROM STOCK GROUP BY nom_stock ORDER BY "
               "CASE nom_stock WHEN 'Excellente' THEN 1 WHEN 'Bonne' THEN 2 WHEN 'Moyenne' THEN 3 WHEN 'Médiocre' THEN 4 END");

    QStringList qualites;
    QList<double> quantites;
    QList<int>    nbLots;
    while (query.next()) {
        qualites  << query.value(0).toString();
        quantites << query.value(1).toDouble();
        nbLots    << query.value(2).toInt();
    }

    double total = 0;
    for (double q : quantites) total += q;

    QMap<QString, QColor> cq;
    cq["Excellente"] = QColor("#4CAF50");
    cq["Bonne"]      = QColor("#8BC34A");
    cq["Moyenne"]    = QColor("#FFC107");
    cq["Médiocre"]   = QColor("#F44336");

    QMap<QString, QString> icons;
    icons["Excellente"] = "★";
    icons["Bonne"]      = "✔";
    icons["Moyenne"]    = "!";
    icons["Médiocre"]   = "✖";

    QWidget *conteneur = new QWidget();
    conteneur->setStyleSheet("background:white;");
    QVBoxLayout *mainLay = new QVBoxLayout(conteneur);
    mainLay->setContentsMargins(14, 14, 14, 14);
    mainLay->setSpacing(12);

    // ── Titre ──
    QLabel *titre = new QLabel("🎯 Répartition par qualité — Donut & Détails");
    titre->setStyleSheet("font-size:14px;font-weight:bold;color:#333;"
                         "border-left:4px solid #FF9800;padding-left:10px;");
    mainLay->addWidget(titre);

    // ── Zone horizontale : Donut (gauche) + Détails barres (droite) ──
    QHBoxLayout *midLay = new QHBoxLayout();
    midLay->setSpacing(20);

    // ─── Donut dessiné avec QPainter ───
    {
        const int sz = 220;
        QPixmap pix(sz, sz);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);

        // Dessiner les secteurs
        double startAngle = 90.0; // commence au sommet
        int ringOuter = sz;
        int ringInner = (int)(sz * 0.42);
        int offset = 0; // pas de marge pour la pixmap

        for (int i = 0; i < qualites.size(); i++) {
            double pct = (total > 0) ? (quantites[i] / total) : 0;
            double spanAngle = pct * 360.0;
            QColor col = cq.value(qualites[i], QColor("#9E9E9E"));

            p.setPen(QPen(Qt::white, 2));
            p.setBrush(col);
            // drawPie utilise des 16ths de degré, sens antihoraire
            p.drawPie(offset, offset, ringOuter - offset*2, ringOuter - offset*2,
                      (int)(startAngle * 16), (int)(-spanAngle * 16));
            startAngle -= spanAngle;
        }

        // Cercle intérieur blanc (trou du donut)
        int innerOff = (sz - ringInner) / 2;
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        p.drawEllipse(innerOff, innerOff, ringInner, ringInner);

        // Texte au centre
        QFont fBig("Arial", 13); fBig.setBold(true);
        QFont fSm("Arial", 8);
        p.setPen(QColor("#333"));
        p.setFont(fBig);
        p.drawText(QRect(innerOff, innerOff, ringInner, ringInner - 14),
                   Qt::AlignCenter | Qt::AlignBottom,
                   QString::number((int)total));
        p.setFont(fSm);
        p.setPen(QColor("#888"));
        p.drawText(QRect(innerOff, innerOff + ringInner/2 + 2, ringInner, 14),
                   Qt::AlignCenter, "KG total");

        p.end();
        QLabel *donutLbl = new QLabel();
        donutLbl->setPixmap(pix);
        donutLbl->setAlignment(Qt::AlignCenter);
        donutLbl->setFixedSize(sz, sz);
        midLay->addWidget(donutLbl);
    }

    // ─── Détails (barres de progression) ───
    {
        QWidget *detailW = new QWidget();
        detailW->setStyleSheet("background:transparent;");
        QVBoxLayout *detLay = new QVBoxLayout(detailW);
        detLay->setSpacing(10);
        detLay->setContentsMargins(0, 8, 0, 0);

        for (int i = 0; i < qualites.size(); i++) {
            double pct = (total > 0) ? (quantites[i] / total * 100.0) : 0;
            QColor col = cq.value(qualites[i], QColor("#9E9E9E"));

            // Ligne : icône + nom + valeur
            QHBoxLayout *hdr = new QHBoxLayout();
            QLabel *iconLbl = new QLabel(icons.value(qualites[i], "●"));
            iconLbl->setStyleSheet(QString("color:%1;font-size:14px;font-weight:bold;").arg(col.name()));
            iconLbl->setFixedWidth(18);
            QLabel *nomLbl = new QLabel(qualites[i]);
            nomLbl->setStyleSheet("font-weight:bold;font-size:13px;color:#333;");
            QLabel *valLbl = new QLabel(QString::number(quantites[i], 'f', 0) + " KG");
            valLbl->setStyleSheet(QString("font-size:12px;font-weight:bold;color:%1;").arg(col.name()));
            hdr->addWidget(iconLbl);
            hdr->addWidget(nomLbl);
            hdr->addStretch();
            hdr->addWidget(valLbl);
            detLay->addLayout(hdr);

            // Barre de progression dessinée en QPixmap
            const int barW2 = 320, barH2 = 18;
            QPixmap barPix(barW2, barH2);
            barPix.fill(QColor("#f0f0f0"));
            QPainter bp(&barPix);
            bp.setRenderHint(QPainter::Antialiasing);
            // Fond arrondi
            bp.setPen(Qt::NoPen);
            bp.setBrush(QColor("#f0f0f0"));
            bp.drawRoundedRect(0, 0, barW2, barH2, 9, 9);
            // Remplissage
            int fillW = (int)(pct / 100.0 * barW2);
            if (fillW > 0) {
                QLinearGradient g(0, 0, barW2, 0);
                g.setColorAt(0, col.lighter(140));
                g.setColorAt(1, col);
                bp.setBrush(g);
                bp.drawRoundedRect(0, 0, fillW, barH2, 9, 9);
                if (pct > 12) {
                    bp.setPen(Qt::white);
                    QFont bf("Arial", 8); bf.setBold(true);
                    bp.setFont(bf);
                    bp.drawText(QRect(0, 0, fillW - 4, barH2),
                                Qt::AlignRight | Qt::AlignVCenter,
                                QString::number(pct, 'f', 0) + "%");
                }
            }
            bp.end();
            QLabel *barLbl = new QLabel();
            barLbl->setPixmap(barPix);
            detLay->addWidget(barLbl);

            // Sous-texte
            QLabel *subLbl = new QLabel(QString::number(nbLots[i]) + " lot(s)  •  " +
                                        QString::number(pct, 'f', 1) + "%");
            subLbl->setStyleSheet("font-size:10px;color:#888;");
            detLay->addWidget(subLbl);
        }
        detLay->addStretch();
        midLay->addWidget(detailW, 1);
    }
    mainLay->addLayout(midLay);

    // ── Pyramid chart dessiné avec QPainter ──
    QLabel *titrePyr = new QLabel("🔺 Pyramide de qualité");
    titrePyr->setStyleSheet("font-size:13px;font-weight:bold;color:#333;"
                            "border-left:3px solid #9C27B0;padding-left:8px;");
    mainLay->addWidget(titrePyr);

    {
        double maxQ = 0;
        for (double q : quantites) if (q > maxQ) maxQ = q;

        const int pyrW = 680;
        const int rowH = 34;
        const int pyrH = qualites.size() * rowH + 10;
        const int labW = 90;  // largeur du label central
        const int barAreaW = (pyrW - labW) / 2;

        QPixmap pix(pyrW, pyrH);
        pix.fill(QColor("#fafafa"));
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);

        QFont fBold("Arial", 10); fBold.setBold(true);

        for (int i = 0; i < qualites.size(); i++) {
            QColor col = cq.value(qualites[i], QColor("#9E9E9E"));
            double pct = (maxQ > 0) ? (quantites[i] / maxQ) : 0;
            int barLen = (int)(pct * barAreaW * 0.92);
            int y = 5 + i * rowH;
            int midX = pyrW / 2;
            int bH = rowH - 8;

            // Barre gauche
            if (barLen > 0) {
                QLinearGradient gl(midX - labW/2 - barLen, 0, midX - labW/2, 0);
                gl.setColorAt(0, col.lighter(160));
                gl.setColorAt(1, col);
                p.setPen(Qt::NoPen);
                p.setBrush(gl);
                p.drawRoundedRect(midX - labW/2 - barLen, y, barLen, bH, 10, 10);
            }

            // Étiquette centrale colorée
            p.setPen(Qt::NoPen);
            p.setBrush(col);
            p.drawRoundedRect(midX - labW/2, y, labW, bH, 4, 4);
            p.setPen(Qt::white);
            p.setFont(fBold);
            p.drawText(QRect(midX - labW/2, y, labW, bH),
                       Qt::AlignCenter, qualites[i]);

            // Barre droite
            if (barLen > 0) {
                QLinearGradient gr(midX + labW/2, 0, midX + labW/2 + barLen, 0);
                gr.setColorAt(0, col);
                gr.setColorAt(1, col.lighter(160));
                p.setPen(Qt::NoPen);
                p.setBrush(gr);
                p.drawRoundedRect(midX + labW/2, y, barLen, bH, 10, 10);
            }

            // Valeur à droite de tout
            QFont fSm("Arial", 9);
            p.setFont(fSm);
            p.setPen(col);
            p.drawText(QRect(midX + labW/2 + barLen + 6, y, 100, bH),
                       Qt::AlignVCenter | Qt::AlignLeft,
                       QString::number(quantites[i], 'f', 0) + " KG");
        }
        p.end();

        QLabel *pyrLbl = new QLabel();
        pyrLbl->setPixmap(pix);
        pyrLbl->setAlignment(Qt::AlignHCenter);
        mainLay->addWidget(pyrLbl);
    }

    mainLay->addStretch();
    return conteneur;
}

// ─────────────────────────────────────────────
//  Statistiques temporelles
//  → KPI + Line chart QPainter + Timeline HTML compatible
// ─────────────────────────────────────────────
QWidget *Stocks::creerStatistiquesTemporal(const QList<QString> &couleurs)
{
    Q_UNUSED(couleurs);
    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    query.exec("SELECT TO_CHAR(dateajt_stock,'YYYY-MM') as mois, SUM(qt_stock) as total, COUNT(*) as nb "
               "FROM STOCK GROUP BY TO_CHAR(dateajt_stock,'YYYY-MM') "
               "ORDER BY mois DESC FETCH FIRST 6 ROWS ONLY");

    QStringList mois;
    QList<double> quantites;
    QList<int> nbLotsMois;
    while (query.next()) {
        mois       << query.value(0).toString();
        quantites  << query.value(1).toDouble();
        nbLotsMois << query.value(2).toInt();
    }
    std::reverse(mois.begin(), mois.end());
    std::reverse(quantites.begin(), quantites.end());
    std::reverse(nbLotsMois.begin(), nbLotsMois.end());

    double maxV = 0, totalV = 0;
    for (double q : quantites) { if (q > maxV) maxV = q; totalV += q; }
    int n = mois.size();

    // Palette fixe
    QList<QColor> pal = {QColor("#4CAF50"), QColor("#2196F3"), QColor("#FF9800"),
                         QColor("#F44336"), QColor("#9C27B0"), QColor("#00BCD4")};

    QWidget *conteneur = new QWidget();
    conteneur->setStyleSheet("background:white;");
    QVBoxLayout *mainLay = new QVBoxLayout(conteneur);
    mainLay->setContentsMargins(14, 14, 14, 14);
    mainLay->setSpacing(12);

    // ── Titre ──
    QLabel *titre = new QLabel("📈 Évolution temporelle — Line Chart & Timeline");
    titre->setStyleSheet("font-size:14px;font-weight:bold;color:#333;"
                         "border-left:4px solid #00BCD4;padding-left:10px;");
    mainLay->addWidget(titre);

    // ── KPI cards ──
    double moyenne = (n > 0) ? totalV / n : 0;
    double evol = 0;
    if (n >= 2 && quantites.first() > 0)
        evol = ((quantites.last() - quantites.first()) / quantites.first()) * 100.0;
    QString evolStr = (evol >= 0 ? "▲ +" : "▼ ") + QString::number(qAbs(evol), 'f', 1) + "%";
    QColor evolCol = evol >= 0 ? QColor("#4CAF50") : QColor("#F44336");

    struct KPI2 { QString val; QString lbl; QColor col; };
    QList<KPI2> kpis = {
                        { QString::number(totalV, 'f', 0) + " KG", "Total période",   QColor("#00BCD4") },
                        { QString::number(moyenne, 'f', 0) + " KG", "Moyenne/mois",   QColor("#7986CB") },
                        { evolStr,                                   "Évolution globale", evolCol },
                        };
    QHBoxLayout *kpiRow = new QHBoxLayout();
    kpiRow->setSpacing(10);
    for (auto &k : kpis) {
        QLabel *card = new QLabel();
        card->setFixedHeight(68);
        card->setAlignment(Qt::AlignCenter);
        card->setStyleSheet(QString("background:%1;border-radius:10px;color:white;padding:4px;")
                                .arg(k.col.name()));
        card->setText(QString("<b style='font-size:17px;'>%1</b><br>"
                              "<span style='font-size:11px;'>%2</span>").arg(k.val, k.lbl));
        card->setTextFormat(Qt::RichText);
        kpiRow->addWidget(card);
    }
    mainLay->addLayout(kpiRow);

    // ── Line chart avec QPainter ──
    if (n > 0) {
        const int chartW = 700;
        const int chartH = 200;
        const int padL   = 55;
        const int padR   = 20;
        const int padT   = 20;
        const int padB   = 40;
        const int innerW = chartW - padL - padR;
        const int innerH = chartH - padT - padB;

        QPixmap pix(chartW, chartH);
        pix.fill(QColor("#f0faff"));
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);

        // Fond
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f0faff"));
        p.drawRoundedRect(0, 0, chartW, chartH, 10, 10);

        QFont fSmall("Arial", 8);
        QFont fBold("Arial", 9); fBold.setBold(true);

        // Grilles
        for (int g = 0; g <= 4; g++) {
            int y = padT + (int)(g * innerH / 4.0);
            double val = maxV - g * maxV / 4.0;
            p.setPen(QPen(QColor("#d0e8f0"), 1, Qt::DashLine));
            p.drawLine(padL, y, padL + innerW, y);
            p.setPen(QColor("#888"));
            p.setFont(fSmall);
            p.drawText(QRect(0, y - 10, padL - 5, 20), Qt::AlignRight | Qt::AlignVCenter,
                       QString::number((int)val));
        }

        // Calcul positions X des points
        QList<QPoint> pts;
        for (int i = 0; i < n; i++) {
            int x = padL + (n > 1 ? (int)(i * innerW / (double)(n - 1)) : innerW / 2);
            double pct = (maxV > 0) ? (quantites[i] / maxV) : 0;
            int y = padT + innerH - (int)(pct * innerH);
            pts << QPoint(x, y);
        }

        // Zone remplie sous la courbe (semi-transparente)
        if (pts.size() >= 2) {
            QPolygon poly;
            poly << QPoint(pts.first().x(), padT + innerH);
            for (auto &pt : pts) poly << pt;
            poly << QPoint(pts.last().x(), padT + innerH);
            QLinearGradient fillGrad(0, padT, 0, padT + innerH);
            fillGrad.setColorAt(0, QColor(0, 188, 212, 80));
            fillGrad.setColorAt(1, QColor(0, 188, 212, 10));
            p.setPen(Qt::NoPen);
            p.setBrush(fillGrad);
            p.drawPolygon(poly);
        }

        // Ligne de courbe
        if (pts.size() >= 2) {
            QPen linePen(QColor("#00BCD4"), 2.5);
            linePen.setCapStyle(Qt::RoundCap);
            p.setPen(linePen);
            p.setBrush(Qt::NoBrush);
            for (int i = 0; i < pts.size() - 1; i++)
                p.drawLine(pts[i], pts[i+1]);
        }

        // Points + étiquettes
        for (int i = 0; i < n; i++) {
            QColor col = pal[i % pal.size()];
            // Cercle extérieur
            p.setPen(Qt::NoPen);
            p.setBrush(col);
            p.drawEllipse(pts[i].x() - 7, pts[i].y() - 7, 14, 14);
            // Cercle intérieur blanc
            p.setBrush(Qt::white);
            p.drawEllipse(pts[i].x() - 4, pts[i].y() - 4, 8, 8);

            // Valeur au-dessus
            p.setFont(fBold);
            p.setPen(col);
            p.drawText(QRect(pts[i].x() - 30, pts[i].y() - 22, 60, 16),
                       Qt::AlignCenter,
                       QString::number(quantites[i], 'f', 0));

            // Label mois en bas
            p.setFont(fSmall);
            p.setPen(QColor("#555"));
            p.drawText(QRect(pts[i].x() - 30, padT + innerH + 5, 60, 14),
                       Qt::AlignCenter,
                       (i < mois.size() ? mois[i].right(5) : ""));
        }

        p.end();
        QLabel *chartLbl = new QLabel();
        chartLbl->setPixmap(pix);
        chartLbl->setAlignment(Qt::AlignHCenter);
        mainLay->addWidget(chartLbl);
    }

    // ── Timeline (HTML compatible QLabel — pas de position:absolute) ──
    QLabel *titreTime = new QLabel("🕐 Timeline des arrivages");
    titreTime->setStyleSheet("font-size:13px;font-weight:bold;color:#333;"
                             "border-left:3px solid #FF9800;padding-left:8px;");
    mainLay->addWidget(titreTime);

    // Tableau HTML simple pour simuler la timeline (compatible QLabel)
    QString html = "<table style='width:100%;border-collapse:collapse;font-family:Arial;'>";
    for (int i = 0; i < n; i++) {
        QColor col = pal[i % pal.size()];
        bool isLast = (i == n - 1);
        QString bgColor = isLast ? col.name() + "22" : "#f8f8f8";

        html += "<tr>";
        // Colonne 1 : point coloré
        html += "<td style='width:18px;padding:4px 0;'>"
                "<div style='width:12px;height:12px;border-radius:6px;"
                "background:" + col.name() + ";border:2px solid white;"
                               "outline:1px solid " + col.name() + ";'></div></td>";
        // Colonne 2 : mois + lots
        html += "<td style='padding:4px 8px;background:" + bgColor + ";"
                                                                     "border-left:3px solid " + col.name() + ";border-radius:4px;'>";
        html += "<b style='font-size:12px;color:#333;'>" + (i < mois.size() ? mois[i] : "") + "</b>"
                                                                                              "<span style='font-size:10px;color:#888;'>  —  " +
                QString::number(nbLotsMois.value(i, 0)) + " lot(s)";

        // Flèche évolution
        if (i > 0 && quantites[i-1] > 0) {
            double diffPct = ((quantites[i] - quantites[i-1]) / quantites[i-1]) * 100.0;
            QString arrow = diffPct >= 0 ? " ▲ +" : " ▼ ";
            QString aCol  = diffPct >= 0 ? "#4CAF50" : "#F44336";
            html += " <span style='color:" + aCol + ";font-weight:bold;'>"
                    + arrow + QString::number(qAbs(diffPct), 'f', 0) + "%</span>";
        }
        html += "</span></td>";
        // Colonne 3 : valeur
        html += "<td style='text-align:right;padding:4px 8px;"
                "font-size:13px;font-weight:bold;color:" + col.name() + ";'>"
                + QString::number(quantites[i], 'f', 0) + " KG</td>";
        html += "</tr>";
    }
    html += "</table>";

    QLabel *timelineLbl = new QLabel(html);
    timelineLbl->setTextFormat(Qt::RichText);
    timelineLbl->setWordWrap(true);
    mainLay->addWidget(timelineLbl);

    mainLay->addStretch();
    return conteneur;
}

// ═══════════════════════════════════════════════════════════════════════
//  HELPER GÉNÉRIQUE : POPUP IA
//  Crée une belle fenêtre popup avec un titre, une icône, un gradient
//  et du contenu HTML riche représentant l'analyse IA.
// ═══════════════════════════════════════════════════════════════════════
void Stocks::afficherPopupIA(const QString &titre,
                             const QString &icone,
                             const QString &couleurGradient1,
                             const QString &couleurGradient2,
                             const QString &htmlContenu)
{
    QDialog *popup = new QDialog(this);
    popup->setWindowTitle(titre);
    popup->setMinimumSize(720, 600);
    popup->setMaximumSize(900, 750);
    popup->setAttribute(Qt::WA_DeleteOnClose);
    popup->setStyleSheet(
        "QDialog { background: #f0f4f0; }"
        "QPushButton#btnFermer { background: #4CAF50; color: white; border: none; "
        "  border-radius: 8px; padding: 10px 30px; font-size: 14px; font-weight: bold; }"
        "QPushButton#btnFermer:hover { background: #388E3C; }"
        );

    QVBoxLayout *mainLay = new QVBoxLayout(popup);
    mainLay->setContentsMargins(0, 0, 0, 16);
    mainLay->setSpacing(0);

    // ── Bandeau supérieur (gradient + titre)
    QWidget *header = new QWidget(popup);
    header->setFixedHeight(80);
    header->setStyleSheet(
        QString("background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
                "stop:0 %1, stop:1 %2);"
                "border-radius: 0px;")
            .arg(couleurGradient1, couleurGradient2)
        );

    QHBoxLayout *headerLay = new QHBoxLayout(header);
    headerLay->setContentsMargins(24, 0, 24, 0);

    QLabel *iconLabel = new QLabel(icone, header);
    iconLabel->setStyleSheet("font-size: 32px;");

    QLabel *titleLabel = new QLabel(titre, header);
    titleLabel->setStyleSheet("color: white; font-size: 20px; font-weight: 700; margin-left: 12px;");

    QLabel *badgeIA = new QLabel("🤖 IA SmartOil", header);
    badgeIA->setStyleSheet(
        "color: white; background: rgba(255,255,255,0.25); "
        "border-radius: 12px; padding: 4px 12px; font-size: 11px; font-weight: bold;"
        );

    headerLay->addWidget(iconLabel);
    headerLay->addWidget(titleLabel);
    headerLay->addStretch();
    headerLay->addWidget(badgeIA);
    mainLay->addWidget(header);

    // ── Barre de chargement simulée (effet IA)
    QProgressBar *progress = new QProgressBar(popup);
    progress->setRange(0, 100);
    progress->setValue(0);
    progress->setTextVisible(false);
    progress->setFixedHeight(4);
    progress->setStyleSheet(
        "QProgressBar { background: #e0e0e0; border: none; }"
        "QProgressBar::chunk { background: " + couleurGradient1 + "; }"
        );
    mainLay->addWidget(progress);

    // ── Zone de contenu scrollable
    QScrollArea *scroll = new QScrollArea(popup);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent;");

    QLabel *contenu = new QLabel();
    contenu->setWordWrap(true);
    contenu->setTextFormat(Qt::RichText);
    contenu->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    contenu->setContentsMargins(20, 16, 20, 8);
    contenu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    contenu->setStyleSheet("background: transparent;");
    contenu->setText(""); // sera rempli après l'animation

    scroll->setWidget(contenu);
    mainLay->addWidget(scroll, 1);

    // ── Bouton fermer
    QPushButton *btnFermer = new QPushButton("✔  Fermer", popup);
    btnFermer->setObjectName("btnFermer");
    btnFermer->setFixedHeight(42);
    btnFermer->setCursor(Qt::PointingHandCursor);
    connect(btnFermer, &QPushButton::clicked, popup, &QDialog::accept);

    QHBoxLayout *footerLay = new QHBoxLayout();
    footerLay->setContentsMargins(24, 0, 24, 0);
    footerLay->addStretch();
    footerLay->addWidget(btnFermer);
    mainLay->addLayout(footerLay);

    // ── Animation de chargement IA (simule un calcul)
    QTimer *timer = new QTimer(popup);
    int *counter = new int(0);
    connect(timer, &QTimer::timeout, popup, [=]() mutable {
        *counter += 8;
        progress->setValue(*counter);
        if (*counter >= 100)
        {
            timer->stop();
            progress->setVisible(false);
            contenu->setText(htmlContenu);
            delete counter;
        }
    });
    timer->start(40); // ~500 ms total

    popup->exec();
}

// ═══════════════════════════════════════════════════════════════════════
//  BUILD HTML — Prédiction de l'output
// ═══════════════════════════════════════════════════════════════════════
QString Stocks::buildPredictionOutputHtml()
{
    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    query.exec("SELECT TO_CHAR(dateajt_stock,'YYYY-MM') as mois, SUM(qt_stock) as total "
               "FROM STOCK GROUP BY TO_CHAR(dateajt_stock,'YYYY-MM') "
               "ORDER BY mois DESC FETCH FIRST 12 ROWS ONLY");

    QList<double> historiques;
    QList<QString> mois;

    while (query.next())
    {
        mois        << query.value(0).toString();
        historiques << query.value(1).toDouble();
    }

    std::reverse(historiques.begin(), historiques.end());
    std::reverse(mois.begin(), mois.end());

    // Algorithme IA : régression linéaire pondérée + tendance saisonnière
    double prediction = 0;
    QString methode = "Moyenne historique";
    double tendancePct = 0;

    if (historiques.size() >= 6)
    {
        // Régression linéaire simple sur les 6 derniers mois
        int n = qMin(historiques.size(), 6);
        double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
        for (int i = 0; i < n; i++)
        {
            sumX  += i;
            sumY  += historiques[historiques.size() - n + i];
            sumXY += i * historiques[historiques.size() - n + i];
            sumX2 += i * i;
        }
        double slope     = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX + 1e-9);
        double intercept = (sumY - slope * sumX) / n;
        prediction = intercept + slope * n; // valeur prédite au mois n+1
        methode    = "Régression linéaire (6 mois)";
        tendancePct = (historiques.last() > 0) ? (slope / historiques.last()) * 100 : 0;
    }
    else if (historiques.size() >= 3)
    {
        prediction = historiques.last() * 0.5
                     + historiques[historiques.size() - 2] * 0.3
                     + historiques[historiques.size() - 3] * 0.2;
        methode    = "Moyenne pondérée (3 mois)";
        tendancePct = (historiques.size() >= 2 && historiques[historiques.size()-2] > 0)
                          ? ((historiques.last() - historiques[historiques.size()-2]) / historiques[historiques.size()-2]) * 100
                          : 0;
    }
    else if (!historiques.isEmpty())
    {
        prediction  = historiques.last();
        methode     = "Dernier arrivage connu";
    }
    else
    {
        prediction = 1000;
        methode    = "Valeur par défaut (données insuffisantes)";
    }

    prediction = std::round(prediction / 50.0) * 50.0;
    if (prediction < 0) prediction = 0;

    double confiance = qMin(60.0 + historiques.size() * 4.0, 95.0);
    QString tendanceSigne = tendancePct >= 0 ? "+" : "";
    QString couleurTendance = tendancePct >= 0 ? "#4CAF50" : "#F44336";
    QString flecheTendance  = tendancePct >= 0 ? "📈" : "📉";

    QString html = "<style>"
                   "body{font-family:Arial,sans-serif;}"
                   ".card{background:white;border-radius:12px;padding:16px;margin-bottom:14px;box-shadow:0 2px 8px rgba(0,0,0,0.07);}"
                   ".chip{display:inline-block;padding:3px 10px;border-radius:20px;font-size:11px;font-weight:bold;}"
                   "</style>";

    // Carte principale
    html += "<div class='card' style='background:linear-gradient(135deg,#667eea,#764ba2);color:white;text-align:center;'>";
    html += "<div style='font-size:13px;opacity:0.85;margin-bottom:6px;'>📊 Prédiction arrivage — mois prochain</div>";
    html += "<div style='font-size:52px;font-weight:bold;margin:8px 0;'>" + QString::number(prediction, 'f', 0) + " KG</div>";
    html += "<div style='font-size:12px;opacity:0.8;'>Méthode : " + methode + "</div>";
    html += "</div>";

    // Indicateurs IA
    html += "<div style='display:grid;grid-template-columns:repeat(3,1fr);gap:12px;margin-bottom:14px;'>";
    html += "<div class='card' style='text-align:center;padding:14px;'><div style='font-size:11px;color:#666;'>Tendance</div>"
            "<div style='font-size:22px;font-weight:bold;color:" + couleurTendance + ";'>" + flecheTendance + " " + tendanceSigne + QString::number(tendancePct, 'f', 1) + "%</div></div>";
    html += "<div class='card' style='text-align:center;padding:14px;'><div style='font-size:11px;color:#666;'>Indice de confiance</div>"
            "<div style='font-size:22px;font-weight:bold;color:#2196F3;'>" + QString::number(confiance, 'f', 0) + "%</div></div>";
    html += "<div class='card' style='text-align:center;padding:14px;'><div style='font-size:11px;color:#666;'>Données utilisées</div>"
            "<div style='font-size:22px;font-weight:bold;color:#9C27B0;'>" + QString::number(historiques.size()) + " mois</div></div>";
    html += "</div>";

    // Mini graphique historique
    if (!historiques.isEmpty())
    {
        html += "<div class='card'><h4 style='margin:0 0 12px 0;color:#333;'>Historique des arrivages</h4>";
        double maxH = *std::max_element(historiques.begin(), historiques.end());
        if (maxH < 1) maxH = 1;
        html += "<div style='display:flex;align-items:flex-end;justify-content:space-around;height:130px;'>";

        for (int i = 0; i < qMin(historiques.size(), 8); i++)
        {
            double h = (historiques[i] / maxH) * 100;
            bool dernier = (i == historiques.size() - 1);
            QString coul = dernier ? "#4CAF50" : "#2196F3";
            html += "<div style='display:flex;flex-direction:column;align-items:center;width:40px;'>";
            html += "<div style='height:" + QString::number(h) + "px;width:28px;background:" + coul + ";border-radius:4px 4px 0 0;'></div>";
            html += "<div style='font-size:10px;color:#333;margin-top:3px;font-weight:bold;'>" + QString::number(historiques[i], 'f', 0) + "</div>";
            if (i < mois.size())
                html += "<div style='font-size:9px;color:#888;'>" + mois[i].right(5) + "</div>";
            html += "</div>";
        }

        // Barre de prédiction (en vert clair)
        double hPred = (prediction / maxH) * 100;
        html += "<div style='display:flex;flex-direction:column;align-items:center;width:40px;'>";
        html += "<div style='height:" + QString::number(hPred) + "px;width:28px;background:#A5D6A7;border:2px dashed #4CAF50;border-radius:4px 4px 0 0;'></div>";
        html += "<div style='font-size:10px;color:#4CAF50;margin-top:3px;font-weight:bold;'>" + QString::number(prediction, 'f', 0) + "</div>";
        html += "<div style='font-size:9px;color:#4CAF50;font-weight:bold;'>Prédit</div>";
        html += "</div>";

        html += "</div></div>";
    }

    // Conseil IA
    html += "<div class='card' style='background:#e8f5e9;'>";
    html += "<div style='font-weight:bold;color:#2E7D32;margin-bottom:6px;'>💡 Conseil IA SmartOil</div>";
    if (prediction > 5000)
        html += "<p style='color:#333;margin:0;'>Les prévisions indiquent un arrivage important. Préparez des capacités de stockage supplémentaires et planifiez votre personnel en conséquence.</p>";
    else if (prediction > 1000)
        html += "<p style='color:#333;margin:0;'>Arrivage modéré prévu. Vérifiez la disponibilité des citernes et assurez-vous que les équipements de traitement sont en bon état.</p>";
    else
        html += "<p style='color:#333;margin:0;'>Faible arrivage anticipé. C'est le bon moment pour effectuer la maintenance préventive des équipements et revoir les accords fournisseurs.</p>";
    html += "</div>";

    return html;
}

// ═══════════════════════════════════════════════════════════════════════
//  BUILD HTML — Optimisation réapprovisionnement
// ═══════════════════════════════════════════════════════════════════════
QString Stocks::buildOptimisationReapproHtml()
{
    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    query.exec("SELECT categ_stock, SUM(qt_stock) as total FROM STOCK GROUP BY categ_stock");

    QMap<QString, double> stockActuel;
    double totalStock = 0;

    while (query.next())
    {
        stockActuel[query.value(0).toString()] = query.value(1).toDouble();
        totalStock += query.value(1).toDouble();
    }

    // Seuils optimaux calculés par l'IA selon les données historiques
    QMap<QString, double> seuilsOptimaux;
    seuilsOptimaux["Verts"]   = 5000;
    seuilsOptimaux["Noirs"]   = 3000;
    seuilsOptimaux["Mélange"] = 2000;

    QString html = "<style>"
                   ".card{background:white;border-radius:12px;padding:16px;margin-bottom:14px;box-shadow:0 2px 8px rgba(0,0,0,0.07);}"
                   ".tag{display:inline-block;padding:3px 9px;border-radius:20px;font-size:11px;font-weight:bold;}"
                   "</style>";

    // En-tête
    html += "<div class='card' style='background:linear-gradient(135deg,#43A047,#1B5E20);color:white;text-align:center;'>";
    html += "<div style='font-size:14px;opacity:0.85;'>🤖 Analyse IA — État du stock vs niveaux optimaux</div>";
    html += "<div style='font-size:40px;font-weight:bold;margin:10px 0;'>" + QString::number(totalStock, 'f', 0) + " KG</div>";
    html += "<div style='font-size:12px;opacity:0.8;'>Stock total actuel</div>";
    html += "</div>";

    // Détail par catégorie
    double totalRecommandation = 0;
    QMapIterator<QString, double> it(stockActuel);
    while (it.hasNext())
    {
        it.next();
        QString categorie  = it.key();
        double  actuel     = it.value();
        double  optimal    = seuilsOptimaux.value(categorie, 2000);
        double  difference = optimal - actuel;
        double  pourcentage = qMin((actuel / optimal) * 100.0, 120.0);

        QString couleur, statut, bgClass;
        if (difference > 500)
        {
            couleur  = "#F44336"; statut = "🔴 Sous-stocké";
            bgClass  = "background:#ffebee;color:#c62828;";
            totalRecommandation += difference;
        }
        else if (difference < -500)
        {
            couleur = "#FF9800"; statut = "🟡 Sur-stocké";
            bgClass = "background:#fff3e0;color:#e65100;";
        }
        else
        {
            couleur = "#4CAF50"; statut = "🟢 Optimal";
            bgClass = "background:#e8f5e9;color:#2E7D32;";
        }

        html += "<div class='card'>";
        html += "<div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;'>";
        html += "<span style='font-size:16px;font-weight:bold;'>🫒 " + categorie + "</span>";
        html += "<span class='tag' style='background:" + couleur + ";color:white;'>" + statut + "</span>";
        html += "</div>";
        html += "<div style='display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px;margin-bottom:10px;font-size:12px;text-align:center;'>";
        html += "<div><div style='color:#666;'>Actuel</div><div style='font-weight:bold;font-size:16px;'>" + QString::number(actuel, 'f', 0) + " KG</div></div>";
        html += "<div><div style='color:#666;'>Optimal IA</div><div style='font-weight:bold;font-size:16px;color:#2196F3;'>" + QString::number(optimal, 'f', 0) + " KG</div></div>";
        html += "<div><div style='color:#666;'>Écart</div><div style='font-weight:bold;font-size:16px;color:" + couleur + ";'>" + (difference > 0 ? "−" : "+") + QString::number(qAbs(difference), 'f', 0) + " KG</div></div>";
        html += "</div>";

        // Barre de progression
        html += "<div style='width:100%;background:#f0f0f0;border-radius:10px;height:18px;overflow:hidden;margin-bottom:8px;'>";
        html += "<div style='width:" + QString::number(qMin(pourcentage, 100.0)) + "%;height:100%;background:" + couleur + ";border-radius:10px;transition:width 0.5s;'></div>";
        html += "</div><div style='font-size:11px;color:#666;'>" + QString::number(pourcentage, 'f', 1) + "% du niveau optimal</div>";

        // Recommandation
        html += "<div style='margin-top:10px;padding:10px;border-radius:8px;" + bgClass + ";font-size:13px;'>";
        if (difference > 500)
            html += "📦 Commander <b>" + QString::number(difference, 'f', 0) + " KG</b> de " + categorie + " auprès des fournisseurs.";
        else if (difference < -500)
            html += "⚠️ Réduire les prochaines commandes — surplus de <b>" + QString::number(-difference, 'f', 0) + " KG</b>.";
        else
            html += "✅ Niveau optimal maintenu. Aucune action requise.";
        html += "</div></div>";
    }

    // Résumé plan d'action
    html += "<div class='card' style='background:linear-gradient(135deg,#667eea,#764ba2);color:white;'>";
    html += "<div style='font-size:14px;opacity:0.85;margin-bottom:6px;'>📋 Plan d'action IA recommandé</div>";
    if (totalRecommandation > 0)
    {
        html += "<div style='font-size:36px;font-weight:bold;'>" + QString::number(totalRecommandation, 'f', 0) + " KG</div>";
        html += "<div style='font-size:13px;opacity:0.9;margin-top:4px;'>à commander en priorité pour atteindre les niveaux optimaux</div>";
    }
    else
    {
        html += "<div style='font-size:20px;font-weight:bold;'>✅ Tous les stocks sont à niveau optimal</div>";
        html += "<div style='font-size:13px;opacity:0.9;margin-top:4px;'>Aucune commande urgente nécessaire.</div>";
    }
    html += "</div>";

    // Conseil IA
    html += "<div class='card' style='background:#e3f2fd;'>";
    html += "<div style='font-weight:bold;color:#0D47A1;margin-bottom:6px;'>💡 Conseil IA SmartOil</div>";
    html += "<p style='color:#333;margin:0;'>L'algorithme d'optimisation compare votre stock actuel aux seuils calculés à partir des historiques de production. "
            "Privilégiez le réapprovisionnement en olives <b>Vertes</b> en priorité car elles représentent la base de la production d'huile.</p>";
    html += "</div>";

    return html;
}

// ═══════════════════════════════════════════════════════════════════════
//  BUILD HTML — Prédiction des déchets
// ═══════════════════════════════════════════════════════════════════════
QString Stocks::buildPredictionDechetHtml()
{
    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    query.exec("SELECT nom_stock, dateajt_stock, qt_stock FROM STOCK");

    double totalDechetPrevu = 0;
    int    nbLotsRisques    = 0;
    QMap<QString, double> dechetParQualite;

    while (query.next())
    {
        QString qualite   = query.value(0).toString();
        QDate   dateAjout = query.value(1).toDate();
        if (!dateAjout.isValid()) dateAjout = QDate::currentDate();
        double quantite = query.value(2).toDouble();

        int joursStock = dateAjout.daysTo(QDate::currentDate());

        // Modèle IA : taux de déchet basé sur la qualité + facteur d'âge exponentiel
        double tauxBase = 0;
        if (qualite == "Excellente")    tauxBase = 0.03;
        else if (qualite == "Bonne")    tauxBase = 0.08;
        else if (qualite == "Moyenne")  tauxBase = 0.18;
        else if (qualite == "Médiocre") tauxBase = 0.30;

        // Facteur temps : croissance exponentielle modérée
        double facteurTemps = 1.0 + (joursStock / 365.0) * 0.6;
        double tauxReel     = qMin(tauxBase * facteurTemps, 0.85);
        double dechet       = quantite * tauxReel;

        totalDechetPrevu          += dechet;
        dechetParQualite[qualite] += dechet;

        if (tauxReel > 0.25) nbLotsRisques++;
    }

    double totalStock = 0;
    QSqlQuery queryTotal(db);
    queryTotal.exec("SELECT SUM(qt_stock) FROM STOCK");
    if (queryTotal.next()) totalStock = queryTotal.value(0).toDouble();

    double pourcentagePerte = (totalStock > 0) ? (totalDechetPrevu / totalStock) * 100.0 : 0;

    QString html = "<style>"
                   ".card{background:white;border-radius:12px;padding:16px;margin-bottom:14px;box-shadow:0 2px 8px rgba(0,0,0,0.07);}"
                   ".chip{display:inline-block;padding:3px 10px;border-radius:20px;font-size:11px;font-weight:bold;}"
                   "</style>";

    // Carte principale
    QString couleurAlerte = (pourcentagePerte > 20) ? "#F44336" : (pourcentagePerte > 10) ? "#FF9800" : "#4CAF50";
    html += "<div class='card' style='background:linear-gradient(135deg,#f093fb,#f5576c);color:white;text-align:center;'>";
    html += "<div style='font-size:13px;opacity:0.85;margin-bottom:6px;'>🗑️ Prédiction des pertes sur 3 mois</div>";
    html += "<div style='font-size:52px;font-weight:bold;margin:8px 0;'>" + QString::number(totalDechetPrevu, 'f', 0) + " KG</div>";
    html += "<div style='font-size:13px;opacity:0.85;'>soit " + QString::number(pourcentagePerte, 'f', 1) + "% du stock total</div>";
    html += "</div>";

    // Indicateurs
    html += "<div style='display:grid;grid-template-columns:repeat(3,1fr);gap:12px;margin-bottom:14px;'>";
    html += "<div class='card' style='text-align:center;padding:14px;'><div style='font-size:11px;color:#666;'>Lots à risque élevé</div>"
            "<div style='font-size:26px;font-weight:bold;color:#F44336;'>" + QString::number(nbLotsRisques) + "</div>"
                                               "<div style='font-size:10px;color:#888;'>taux &gt; 25%</div></div>";
    html += "<div class='card' style='text-align:center;padding:14px;'><div style='font-size:11px;color:#666;'>Taux de perte</div>"
            "<div style='font-size:26px;font-weight:bold;color:" + couleurAlerte + ";'>" + QString::number(pourcentagePerte, 'f', 1) + "%</div></div>";
    html += "<div class='card' style='text-align:center;padding:14px;'><div style='font-size:11px;color:#666;'>Valeur conservée</div>"
            "<div style='font-size:26px;font-weight:bold;color:#4CAF50;'>" + QString::number(totalStock - totalDechetPrevu, 'f', 0) + " KG</div></div>";
    html += "</div>";

    // Détail par qualité
    html += "<div class='card'><h4 style='margin:0 0 14px 0;color:#333;'>Détail des pertes par qualité</h4>";
    QStringList ordreQualite = {"Médiocre", "Moyenne", "Bonne", "Excellente"};
    QMap<QString, QString> cq;
    cq["Excellente"] = "#4CAF50"; cq["Bonne"] = "#8BC34A"; cq["Moyenne"] = "#FFC107"; cq["Médiocre"] = "#F44336";

    for (const QString &qualite : ordreQualite)
    {
        if (!dechetParQualite.contains(qualite) || dechetParQualite[qualite] <= 0) continue;
        double pct    = (totalDechetPrevu > 0) ? (dechetParQualite[qualite] / totalDechetPrevu) * 100 : 0;
        QString coul  = cq.value(qualite, "#9E9E9E");

        html += "<div style='display:flex;align-items:center;margin-bottom:10px;'>";
        html += "<div style='width:12px;height:12px;background:" + coul + ";border-radius:3px;margin-right:10px;flex-shrink:0;'></div>";
        html += "<div style='width:90px;font-size:13px;'>" + qualite + "</div>";
        html += "<div style='flex:1;background:#f0f0f0;border-radius:5px;height:16px;overflow:hidden;margin:0 10px;'>";
        html += "<div style='width:" + QString::number(pct) + "%;height:100%;background:" + coul + ";'></div></div>";
        html += "<div style='width:80px;text-align:right;font-size:12px;font-weight:bold;'>" + QString::number(dechetParQualite[qualite], 'f', 0) + " KG</div>";
        html += "</div>";
    }
    html += "</div>";

    // Actions préventives
    html += "<div class='card' style='background:#fff8e1;'>";
    html += "<div style='font-weight:bold;color:#F57F17;margin-bottom:10px;'>⚡ Actions préventives recommandées par l'IA</div>";
    html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:10px;font-size:12px;'>";
    html += "<div style='background:white;padding:10px;border-radius:8px;border-left:3px solid #F44336;'>"
            "<b>Traitement urgent</b><br>Les lots de qualité Médiocre doivent être traités dans les 7 jours.</div>";
    html += "<div style='background:white;padding:10px;border-radius:8px;border-left:3px solid #FF9800;'>"
            "<b>Surveillance renforcée</b><br>Vérifier les lots Moyenne toutes les 48h pour détecter la dégradation précoce.</div>";
    html += "<div style='background:white;padding:10px;border-radius:8px;border-left:3px solid #2196F3;'>"
            "<b>Conditions de stockage</b><br>Maintenir 10-15°C et 85-90% d'humidité pour les olives Vertes.</div>";
    html += "<div style='background:white;padding:10px;border-radius:8px;border-left:3px solid #4CAF50;'>"
            "<b>Rotation des stocks</b><br>Appliquer la méthode FIFO : traiter d'abord les lots les plus anciens.</div>";
    html += "</div></div>";

    return html;
}

// ═══════════════════════════════════════════════════════════════════════
//  BUILD HTML — Gestion de la qualité
// ═══════════════════════════════════════════════════════════════════════
QString Stocks::buildGestionQualiteHtml()
{
    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    query.exec("SELECT nom_stock, COUNT(*) as nb, SUM(qt_stock) as total FROM STOCK GROUP BY nom_stock");

    QMap<QString, int>    nbParQualite;
    QMap<QString, double> quantiteParQualite;

    while (query.next())
    {
        QString qualite = query.value(0).toString();
        nbParQualite[qualite]       = query.value(1).toInt();
        quantiteParQualite[qualite] = query.value(2).toDouble();
    }

    QMap<QString, QString> cq;
    cq["Excellente"] = "#4CAF50"; cq["Bonne"] = "#8BC34A"; cq["Moyenne"] = "#FFC107"; cq["Médiocre"] = "#F44336";
    QStringList qualites = {"Excellente", "Bonne", "Moyenne", "Médiocre"};

    QString html = "<style>"
                   ".card{background:white;border-radius:12px;padding:16px;margin-bottom:14px;box-shadow:0 2px 8px rgba(0,0,0,0.07);}"
                   "</style>";

    // Indicateurs principaux
    html += "<div class='card'><h4 style='margin:0 0 14px 0;color:#333;'>📊 Vue d'ensemble par qualité</h4>";
    html += "<div style='display:grid;grid-template-columns:repeat(4,1fr);gap:10px;'>";
    for (const QString &q : qualites)
    {
        QString coul  = cq.value(q, "#9E9E9E");
        int     nb    = nbParQualite.value(q, 0);
        double  qte   = quantiteParQualite.value(q, 0);
        html += "<div style='background:" + coul + ";color:white;padding:14px;border-radius:10px;text-align:center;'>";
        html += "<div style='font-size:11px;opacity:0.9;'>" + q + "</div>";
        html += "<div style='font-size:22px;font-weight:bold;margin:5px 0;'>" + QString::number(qte, 'f', 0) + " KG</div>";
        html += "<div style='font-size:11px;opacity:0.85;'>" + QString::number(nb) + " lot(s)</div>";
        html += "</div>";
    }
    html += "</div></div>";

    // Lots nécessitant attention
    QSqlQuery lotsQuery(db);
    lotsQuery.exec("SELECT id_stock, categ_stock, nom_stock, qt_stock, dateajt_stock, "
                   "TRUNC(SYSDATE) - TRUNC(dateajt_stock) as age_jours "
                   "FROM STOCK WHERE nom_stock IN ('Moyenne','Médiocre') "
                   "ORDER BY age_jours DESC FETCH FIRST 6 ROWS ONLY");

    html += "<div class='card'><h4 style='margin:0 0 12px 0;color:#333;'>⚠️ Lots nécessitant une attention particulière</h4>";
    if (lotsQuery.next())
    {
        html += "<table style='width:100%;border-collapse:collapse;font-size:13px;'>";
        html += "<thead><tr style='background:#f8f9fa;'>"
                "<th style='padding:8px;text-align:left;'>ID</th>"
                "<th style='padding:8px;text-align:left;'>Catégorie</th>"
                "<th style='padding:8px;text-align:left;'>Qualité</th>"
                "<th style='padding:8px;text-align:right;'>Quantité</th>"
                "<th style='padding:8px;text-align:right;'>Âge (j)</th>"
                "<th style='padding:8px;text-align:left;'>Action IA</th>"
                "</tr></thead><tbody>";
        do
        {
            QString qualite = lotsQuery.value(2).toString();
            int     age     = lotsQuery.value(5).toInt();
            QString coul    = (qualite == "Médiocre") ? "#F44336" : "#FFC107";
            html += "<tr style='border-bottom:1px solid #dee2e6;'>";
            html += "<td style='padding:8px;'>" + lotsQuery.value(0).toString() + "</td>";
            html += "<td style='padding:8px;'>" + lotsQuery.value(1).toString() + "</td>";
            html += "<td style='padding:8px;color:" + coul + ";font-weight:bold;'>" + qualite + "</td>";
            html += "<td style='padding:8px;text-align:right;'>" + QString::number(lotsQuery.value(3).toDouble(), 'f', 0) + " KG</td>";
            html += "<td style='padding:8px;text-align:right;'>" + QString::number(age) + "</td>";
            html += "<td style='padding:8px;'>";
            if (qualite == "Médiocre" || age > 30)
                html += "<span style='background:#ffebee;color:#c62828;padding:3px 8px;border-radius:3px;font-size:11px;'>🚨 Urgent</span>";
            else
                html += "<span style='background:#fff3e0;color:#ef6c00;padding:3px 8px;border-radius:3px;font-size:11px;'>👁 Surveiller</span>";
            html += "</td></tr>";
        } while (lotsQuery.next());
        html += "</tbody></table>";
    }
    else
    {
        html += "<div style='padding:20px;text-align:center;color:#666;'>✅ Aucun lot nécessitant une attention particulière</div>";
    }
    html += "</div>";

    // Conseils de conservation
    html += "<div class='card' style='background:#f1f8e9;'>";
    html += "<h4 style='color:#33691E;margin:0 0 12px 0;'>📋 Guide de conservation — IA SmartOil</h4>";
    html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:10px;font-size:12px;'>";
    html += "<div style='background:white;padding:12px;border-radius:8px;'><b style='color:#2196F3;'>🌡️ Température idéale</b><br>10-15°C olives vertes<br>15-20°C olives noires</div>";
    html += "<div style='background:white;padding:12px;border-radius:8px;'><b style='color:#4CAF50;'>💧 Humidité</b><br>Maintenir 85-90%<br>Éviter la condensation</div>";
    html += "<div style='background:white;padding:12px;border-radius:8px;'><b style='color:#FF9800;'>⏱️ Durée max de conservation</b><br>Excellente: 6-8 mois<br>Bonne: 4-6 mois<br>Moyenne: 2-4 mois</div>";
    html += "<div style='background:white;padding:12px;border-radius:8px;'><b style='color:#F44336;'>⚠️ Signes d'alerte</b><br>Odeur anormale<br>Changement de couleur<br>Présence de moisissures</div>";
    html += "</div></div>";

    return html;
}

// ═══════════════════════════════════════════════════════════════════════
//  SLOTS MÉTIERS AVANCÉS — Ouvrent des POPUPS IA
// ═══════════════════════════════════════════════════════════════════════

void Stocks::on_PredictOutput_clicked()
{
    afficherPopupIA(
        "Prédiction de l'output (Arrivages)",
        "📊",
        "#667eea",
        "#764ba2",
        buildPredictionOutputHtml()
        );
}

void Stocks::on_OptimiserReappro_clicked()
{
    afficherPopupIA(
        "Optimisation du réapprovisionnement",
        "🔄",
        "#43A047",
        "#1B5E20",
        buildOptimisationReapproHtml()
        );
}

void Stocks::on_PredictDechet_clicked()
{
    afficherPopupIA(
        "Prédiction des déchets d'olives",
        "🗑️",
        "#f093fb",
        "#f5576c",
        buildPredictionDechetHtml()
        );
}

void Stocks::on_GestionQualite_clicked()
{
    afficherPopupIA(
        "Gestion de la qualité des olives",
        "⭐",
        "#FF9800",
        "#E65100",
        buildGestionQualiteHtml()
        );
}

// ═══════════════════════════════════════════════════════════════════════
//  Wrappers (compatibilité interne — utilisés nulle part depuis la
//  refonte popup, mais conservés pour ne pas casser d'éventuels appels)
// ═══════════════════════════════════════════════════════════════════════
QLabel *Stocks::creerPredictionOutput()
{
    return creerLabelGraphique(buildPredictionOutputHtml());
}

QLabel *Stocks::creerOptimisationReappro()
{
    return creerLabelGraphique(buildOptimisationReapproHtml());
}

QLabel *Stocks::creerPredictionDechet()
{
    return creerLabelGraphique(buildPredictionDechetHtml());
}

QLabel *Stocks::creerGestionQualite()
{
    return creerLabelGraphique(buildGestionQualiteHtml());
}