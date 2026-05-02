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

namespace {
QSqlDatabase stockDb()
{
    if (QSqlDatabase::contains("ConnexionPrincipale"))
        return QSqlDatabase::database("ConnexionPrincipale");
    return QSqlDatabase::database();
}
}

Stocks::Stocks(QWidget *parent) : QMainWindow(parent),
    ui(new Ui::Stocks)
{
    ui->setupUi(this);

    setWindowTitle("Gestion des stocks");
    setWindowIcon(QIcon(":/img/stock.svg"));
    resize(1100, 720);
    setupUnifiedToolbar();
    applyUnifiedVisualStyle();

    // Créer une status bar
    statusBar()->showMessage("Prêt");

    // Initialisation du combobox de tri
    ui->Trier->clear();
    ui->Trier->addItem("Trier par défaut");
    ui->Trier->addItem("Catégorie");
    ui->Trier->addItem("Quantité");
    ui->Trier->addItem("Date d'ajout");
    ui->Trier->addItem("Date mise à jour");

    // Recherche en temps réel (connexion manuelle nécessaire car le slot ne suit
    // pas la convention on_<widget>_<signal> attendue par setupUi)
    connect(ui->SaiRecherche, &QLineEdit::textChanged, this, &Stocks::on_Recherche_clicked);

    // NOTE : on_Trier_currentIndexChanged, on_exporterListeStock_clicked,
    // on_PredictOutput_clicked, on_OptimiserReappro_clicked, on_PredictDechet_clicked,
    // on_GestionQualite_clicked et on_X_currentIndexChanged sont déjà auto-connectés
    // par setupUi() via la convention de nommage Qt — pas de connect() manuel.

    // Initialisation du combobox X pour les statistiques
    ui->X->clear();
    ui->X->addItem("Statistiques générales");
    ui->X->addItem("Par catégorie");
    ui->X->addItem("Par quantité");
    ui->X->addItem("Par qualité");
    ui->X->addItem("Évolution temporelle");

    ui->X->setEnabled(true);
    ui->X->setVisible(true);

    // on_X_currentIndexChanged est auto-connecté par setupUi — pas de connect() ici.

    // Layout pour les statistiques
    if (!ui->chartStatusContainer->layout())
    {
        QVBoxLayout *layout = new QVBoxLayout(ui->chartStatusContainer);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(15);
        layout->addWidget(ui->FaireLesStat);
        layout->addWidget(ui->X);
        ui->chartStatusContainer->setLayout(layout);
    }

    // Initialisation des dates par défaut
    ui->SaiDateAjout->setDate(QDate::currentDate());
    ui->SaiDateMAJ->setDate(QDate::currentDate());

    // Remplir le combobox des catégories
    ui->SaiCategorie->clear();
    ui->SaiCategorie->addItems({"Verts", "Noirs", "Mélange"});

    // Remplir le combobox de qualité
    ui->SaiQualite->clear();
    ui->SaiQualite->addItems({"Excellente", "Bonne", "Moyenne", "Médiocre"});

    chargerListeOlives();
}


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
    QAction *actAdd = nav->addAction(QIcon(":/img/add.svg"), "Ajouter");
    QAction *actStat = nav->addAction(QIcon(":/img/chart.svg"), "Statistiques");
    QAction *actAdv = nav->addAction(QIcon(":/img/settings.svg"), "Avancé");

    connect(actConsult, &QAction::triggered, this, &Stocks::on_btnConsulterstc_clicked);
    connect(actAdd, &QAction::triggered, this, &Stocks::on_AjoutStock_clicked);
    connect(actStat, &QAction::triggered, this, &Stocks::on_StatistiqueStock_clicked);
    connect(actAdv, &QAction::triggered, this, &Stocks::on_PredictOutput_clicked);

    addToolBar(Qt::TopToolBarArea, nav);
}

void Stocks::applyUnifiedVisualStyle()
{
    ui->ConAjout->setProperty("type", "primary");
    ui->exporterListeStock->setProperty("type", "primary");
    ui->PredictOutput->setProperty("type", "primary");
    ui->OptimiserReappro->setProperty("type", "warning");
    ui->PredictDechet->setProperty("type", "danger");
    ui->GestionQualite->setProperty("type", "success");

    ui->ConAjout->setIcon(QIcon(":/img/add.svg"));
    ui->exporterListeStock->setIcon(QIcon(":/img/export.svg"));
    ui->PredictOutput->setIcon(QIcon(":/img/chart.svg"));
    ui->OptimiserReappro->setIcon(QIcon(":/img/refresh.svg"));
    ui->PredictDechet->setIcon(QIcon(":/img/filter.svg"));
    ui->GestionQualite->setIcon(QIcon(":/img/settings.svg"));

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
    case 0:
        on_AjoutStock_clicked();
        break;
    case 1:
        on_btnConsulterstc_clicked();
        break;
    case 2:
        on_StatistiqueStock_clicked();
        break;
    case 3:
        on_PredictOutput_clicked();
        break;
    default:
        break;
    }
}

Stocks::~Stocks()
{
    delete ui;
}

// -------------------------
// Charger / Rafraîchir liste
// -------------------------
void Stocks::chargerListeOlives()
{
    QString orderBy = "id_stock"; // Par défaut

    QString triTexte = ui->Trier->currentText();
    if (triTexte == "Catégorie")
        orderBy = "categ_stock";
    else if (triTexte == "Quantité")
        orderBy = "qt_stock";
    else if (triTexte == "Date d'ajout")
        orderBy = "dateajt_stock";
    else if (triTexte == "Date mise à jour")
        orderBy = "datemaj_stock";

    QString sql = QString(
                      "SELECT id_stock, categ_stock, descript_stock, dateajt_stock, datemaj_stock, qt_stock, nom_stock "
                      "FROM STOCK "
                      "ORDER BY %1 DESC")
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

        // Capturer l'ID stable (pas l'indice de ligne qui change après tri/refresh)
        QString id = query.value(0).toString();

        for (int col = 0; col < 7; ++col)
        {
            QTableWidgetItem *item = new QTableWidgetItem(query.value(col).toString());
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);

            // Colorer la ligne selon la qualité
            if (col == 6)
            { // Colonne qualité
                QString qualite = query.value(col).toString();
                if (qualite == "Excellente")
                    item->setBackground(QColor(200, 255, 200));
                else if (qualite == "Bonne")
                    item->setBackground(QColor(230, 255, 230));
                else if (qualite == "Moyenne")
                    item->setBackground(QColor(255, 255, 200));
                else if (qualite == "Médiocre")
                    item->setBackground(QColor(255, 200, 200));
            }

            ui->ListeStock->setItem(row, col, item);
        }

        // Boutons d'action — capturent l'ID stable, pas le numéro de ligne
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

        connect(btnMod, &QPushButton::clicked, this, [this, id]()
                { onModifier(id); });
        connect(btnSup, &QPushButton::clicked, this, [this, id]()
                { onSupprimer(id); });

        ui->ListeStock->setRowHeight(row, 50);
        row++;
    }

    statusBar()->showMessage(QString("%1 lots d'olives chargés").arg(row), 3000);
}

void Stocks::rafraichirListe()
{
    chargerListeOlives();
}

// -------------------------
// Navigation onglets
// -------------------------
void Stocks::on_AjoutStock_clicked()
{
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

// -------------------------
// Validation des données
// -------------------------
bool Stocks::validerQuantite(double quantite)
{
    return quantite > 0 && quantite <= 100000; // Max 100 tonnes
}

bool Stocks::validerDonneesAjout()
{
    QString categorie = ui->SaiCategorie->currentText();
    QString description = ui->SaiDescription->toPlainText().trimmed();
    QString quantiteStr = ui->SaiQuantite->text().trimmed();
    QString qualite = ui->SaiQualite->currentText();

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

// -------------------------
// Ajouter un lot d'olives
// -------------------------
void Stocks::on_ConAjout_clicked()
{
    if (!validerDonneesAjout())
        return;

    QString categorie = ui->SaiCategorie->currentText();
    QString description = ui->SaiDescription->toPlainText().trimmed();
    QDate dateAjout = ui->SaiDateAjout->date();
    QDate dateMAJ = ui->SaiDateMAJ->date();
    double quantite = ui->SaiQuantite->text().toDouble();
    QString qualite = ui->SaiQualite->currentText();

    QSqlDatabase db = stockDb();
    QSqlQuery q(db);
    q.prepare("INSERT INTO STOCK (categ_stock, descript_stock, dateajt_stock, datemaj_stock, qt_stock, nom_stock) "
              "VALUES (:categorie, :description, :date_ajout, :date_maj, :quantite, :qualite)");

    q.bindValue(":categorie",   categorie);
    q.bindValue(":description", description);
    q.bindValue(":date_ajout",  dateAjout);   // QDate lié directement via ODBC
    q.bindValue(":date_maj",    dateMAJ);     // QDate lié directement via ODBC
    q.bindValue(":quantite",    quantite);
    q.bindValue(":qualite",     qualite);

    if (q.exec())
    {
        QMessageBox::information(this, "Succès", "Lot d'olives ajouté avec succès");

        // Réinitialiser les champs
        ui->SaiCategorie->setCurrentIndex(0);
        ui->SaiDescription->clear();
        ui->SaiDateAjout->setDate(QDate::currentDate());
        ui->SaiDateMAJ->setDate(QDate::currentDate());
        ui->SaiQuantite->clear();
        ui->SaiQualite->setCurrentIndex(0);

        rafraichirListe();
        ui->metiersStock->setCurrentWidget(ui->consulterStock);
        statusBar()->showMessage("Lot d'olives ajouté", 3000);
    }
    else
    {
        QMessageBox::critical(this, "Erreur SQL", q.lastError().text());
    }
}

// -------------------------
// Modifier un lot
// -------------------------
void Stocks::onModifier(const QString &id)
{
    // Lire les valeurs actuelles depuis la base — pas depuis le tableau (indices périssables)
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

    // Fallback si le driver retourne une date invalide
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

    form->addRow("Catégorie:", editCategorie);
    form->addRow("Description:", editDescription);
    form->addRow("Date d'ajout:", editDateAjout);
    form->addRow("Date mise à jour:", editDateMAJ);
    form->addRow("Quantité (KG):", editQuantite);
    form->addRow("Qualité:", editQualite);

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
    q.bindValue(":date_ajout",  editDateAjout->date());   // QDate lié directement
    q.bindValue(":date_maj",    editDateMAJ->date());     // QDate lié directement
    q.bindValue(":quantite",    newQuantite);
    q.bindValue(":qualite",     editQualite->currentText());
    q.bindValue(":id",          id);

    if (!q.exec())
    {
        QMessageBox::critical(this, "Erreur SQL", q.lastError().text());
    }
    else
    {
        QMessageBox::information(this, "Succès", "Lot modifié avec succès");
        rafraichirListe();
    }
}

// -------------------------
// Supprimer un lot
// -------------------------
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
        rafraichirListe(); // reconstruire entièrement pour que les lambdas restent cohérents
    }
    else
    {
        QMessageBox::critical(this, "Erreur", q.lastError().text());
    }
}

// -------------------------
// Tri
// -------------------------
void Stocks::on_Trier_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    rafraichirListe();
}

// -------------------------
// Recherche
// -------------------------
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

    // Paramètres liés pour éviter l'injection SQL
    query.prepare(
        "SELECT id_stock, categ_stock, descript_stock, dateajt_stock, datemaj_stock, qt_stock, nom_stock "
        "FROM STOCK "
        "WHERE UPPER(categ_stock)    LIKE UPPER(:s1) "
        "   OR UPPER(descript_stock)  LIKE UPPER(:s2) "
        "   OR UPPER(nom_stock)      LIKE UPPER(:s3) "
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

        // Capturer l'ID stable
        QString id = query.value(0).toString();

        for (int col = 0; col < 7; col++)
        {
            QTableWidgetItem *item = new QTableWidgetItem(query.value(col).toString());
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            ui->ListeStock->setItem(row, col, item);
        }

        // Boutons d'action — capturent l'ID stable
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

        connect(btnMod, &QPushButton::clicked, this, [this, id]()
                { onModifier(id); });
        connect(btnSup, &QPushButton::clicked, this, [this, id]()
                { onSupprimer(id); });

        ui->ListeStock->setRowHeight(row, 50);
        row++;
    }

    if (row == 0)
    {
        QMessageBox::information(this, "Recherche", "Aucun résultat trouvé pour : " + recherche);
    }
    else
    {
        statusBar()->showMessage(QString("%1 résultat(s) trouvé(s)").arg(row), 3000);
    }
}

// -------------------------
// Export PDF
// -------------------------
void Stocks::on_exporterListeStock_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Exporter en PDF",
                                                    QString("STOCK_%1.pdf").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
                                                    "Fichiers PDF (*.pdf)");

    if (fileName.isEmpty())
        return;

    // Récupérer toutes les données
    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    query.exec("SELECT id_stock, categ_stock, descript_stock, dateajt_stock, datemaj_stock, qt_stock, nom_stock FROM STOCK ORDER BY dateajt_stock DESC");

    if (!query.isActive())
    {
        QMessageBox::critical(this, "Erreur", "Impossible de récupérer les données");
        return;
    }

    // BUG FIX : query.size() retourne -1 avec les drivers ODBC.
    // On utilise une requête COUNT séparée à la place.
    int nbLots = 0;
    QSqlQuery queryCount(db);
    queryCount.exec("SELECT COUNT(*) FROM STOCK");
    if (queryCount.next())
        nbLots = queryCount.value(0).toInt();

    // Calculer le total
    double totalQuantite = 0;
    QSqlQuery queryTotal(db);
    queryTotal.exec("SELECT SUM(qt_stock) FROM STOCK");
    if (queryTotal.next())
    {
        totalQuantite = queryTotal.value(0).toDouble();
    }

    // Créer le contenu HTML
    QString html = "<!DOCTYPE html>"
                   "<html>"
                   "<head>"
                   "<meta charset='UTF-8'>"
                   "<title>Stock d'Olives</title>"
                   "<style>"
                   "body { font-family: Arial, sans-serif; margin: 20px; }"
                   "h1 { color: #4CAF50; text-align: center; }"
                   ".header { text-align: center; margin-bottom: 30px; }"
                   ".stats { display: flex; justify-content: space-around; margin: 20px 0; padding: 15px; background: #f5f5f5; border-radius: 8px; }"
                   ".stat-card { text-align: center; }"
                   ".stat-value { font-size: 24px; font-weight: bold; color: #4CAF50; }"
                   ".stat-label { font-size: 12px; color: #666; }"
                   "table { width: 100%; border-collapse: collapse; margin-top: 20px; }"
                   "th { background-color: #4CAF50; color: white; padding: 10px; text-align: left; }"
                   "td { border: 1px solid #ddd; padding: 8px; }"
                   "tr:nth-child(even) { background-color: #f2f2f2; }"
                   ".excellente { background-color: #d4edda; }"
                   ".bonne { background-color: #fff3cd; }"
                   ".moyenne { background-color: #fff3cd; }"
                   ".mediocre { background-color: #f8d7da; }"
                   ".footer { text-align: right; color: #666; margin-top: 30px; }"
                   "</style>"
                   "</head>"
                   "<body>"
                   "<div class='header'>"
                   "<h1>Stock d'Olives</h1>"
                   "</div>"

                   "<div class='stats'>"
                   "<div class='stat-card'>"
                   "<div class='stat-value'>" +
                   QString::number(totalQuantite, 'f', 0) + " KG</div>"
                                                            "<div class='stat-label'>Quantité totale</div>"
                                                            "</div>"
                                                            "<div class='stat-card'>"
                                                            "<div class='stat-value'>" +
                   QString::number(nbLots) + "</div>"
                                             "<div class='stat-label'>Nombre de lots</div>"
                                             "</div>"
                                             "<div class='stat-card'>"
                                             "<div class='stat-value'>" +
                   QDate::currentDate().toString("dd/MM/yyyy") + "</div>"
                                                                 "<div class='stat-label'>Date d'export</div>"
                                                                 "</div>"
                                                                 "</div>"

                                                                 "<table>"
                                                                 "<thead>"
                                                                 "<tr>"
                                                                 "<th>ID</th>"
                                                                 "<th>Catégorie</th>"
                                                                 "<th>Description</th>"
                                                                 "<th>Date d'ajout</th>"
                                                                 "<th>Dernière MAJ</th>"
                                                                 "<th>Quantité (KG)</th>"
                                                                 "<th>Qualité</th>"
                                                                 "</tr>"
                                                                 "</thead>"
                                                                 "<tbody>";

    while (query.next())
    {
        QString qualite = query.value(6).toString();
        QString rowClass;

        if (qualite == "Excellente")
            rowClass = "excellente";
        else if (qualite == "Bonne")
            rowClass = "bonne";
        else if (qualite == "Moyenne")
            rowClass = "moyenne";
        else if (qualite == "Médiocre")
            rowClass = "mediocre";

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

    html += "</tbody>"
            "</table>"
            "<div class='footer'>Généré le : " +
            QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm:ss") + "</div>"
                                                                           "</body>"
                                                                           "</html>";

    // Exporter en PDF
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

// -------------------------
// Fonctions utilitaires pour les graphiques
// -------------------------
QString Stocks::debutGraphique(const QString &titre, const QString &type)
{
    QString html = "<div class='graphique graphique-" + type + "' style='background: white; border-radius: 15px; padding: 20px; ";
    html += "box-shadow: 0 10px 30px rgba(0,0,0,0.1); margin-top: 15px; animation: fadeIn 0.5s ease;'>";

    html += "<div style='display: flex; align-items: center; justify-content: space-between; margin-bottom: 20px;'>";
    html += "<h3 style='color: #333; margin: 0; font-size: 18px; border-left: 4px solid #4CAF50; padding-left: 15px;'>" + titre + "</h3>";
    html += "<span style='background: #e8f5e9; color: #4CAF50; padding: 4px 12px; border-radius: 20px; font-size: 12px; font-weight: bold;'>" + type + "</span>";
    html += "</div>";

    return html;
}

QString Stocks::finGraphique()
{
    return "</div>";
}

QLabel *Stocks::creerLabelGraphique(const QString &html)
{
    QLabel *label = new QLabel(html);
    label->setWordWrap(true);
    label->setTextFormat(Qt::RichText);
    label->setMinimumHeight(400);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QString styles = "<style>";
    styles += "@keyframes fadeIn { from { opacity: 0; transform: translateY(10px); } to { opacity: 1; transform: translateY(0); } }";
    styles += "@keyframes barGrow { from { width: 0; } to { width: 100%; } }";
    styles += ".graphique { transition: all 0.3s ease; }";
    styles += ".graphique:hover { box-shadow: 0 15px 40px rgba(0,0,0,0.15); }";
    styles += ".stat-card { transition: transform 0.2s ease; }";
    styles += ".stat-card:hover { transform: translateY(-5px); }";
    styles += "</style>";

    label->setText(styles + html);
    return label;
}

void Stocks::remplacerContenuAvecAnimation(QLabel *nouveauGraphique)
{
    QLayout *oldLayout = ui->chartStatusContainer->layout();
    if (!oldLayout)
        return;

    QWidget *faireLesStat = ui->FaireLesStat;
    QWidget *x = ui->X;

    QLayoutItem *item;
    while ((item = oldLayout->takeAt(0)) != nullptr)
    {
        if (item->widget() && item->widget() != faireLesStat && item->widget() != x)
        {
            delete item->widget();
        }
        delete item;
    }

    oldLayout->addWidget(nouveauGraphique);
    x->setEnabled(true);
    x->setVisible(true);
    x->raise();
}

void Stocks::afficherMessageErreur(const QString &message)
{
    QLabel *errorLabel = new QLabel("<div style='padding: 50px; text-align: center; color: #F44336; font-size: 16px;'>" + message + "</div>");
    errorLabel->setAlignment(Qt::AlignCenter);

    QLayout *oldLayout = ui->chartStatusContainer->layout();
    if (oldLayout)
    {
        QWidget *faireLesStat = ui->FaireLesStat;
        QWidget *x = ui->X;

        QLayoutItem *item;
        while ((item = oldLayout->takeAt(0)) != nullptr)
        {
            if (item->widget() && item->widget() != faireLesStat && item->widget() != x)
            {
                delete item->widget();
            }
            delete item;
        }

        oldLayout->addWidget(errorLabel);
    }
}

void Stocks::afficherMessageInfo(const QString &message)
{
    QLabel *infoLabel = new QLabel("<div style='padding: 50px; text-align: center; color: #666; font-size: 16px;'>" + message + "</div>");
    infoLabel->setAlignment(Qt::AlignCenter);

    QLayout *oldLayout = ui->chartStatusContainer->layout();
    if (oldLayout)
    {
        QWidget *faireLesStat = ui->FaireLesStat;
        QWidget *x = ui->X;

        QLayoutItem *item;
        while ((item = oldLayout->takeAt(0)) != nullptr)
        {
            if (item->widget() && item->widget() != faireLesStat && item->widget() != x)
            {
                delete item->widget();
            }
            delete item;
        }

        oldLayout->addWidget(infoLabel);
    }
}

// -------------------------
// Statistiques par catégorie
// -------------------------
QLabel *Stocks::creerStatistiquesCategorie(const QList<QString> &couleurs)
{
    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    query.exec("SELECT categ_stock, SUM(qt_stock) as total, COUNT(*) as nb_lots FROM STOCK GROUP BY categ_stock");

    QStringList categories;
    QList<double> quantites;
    QList<int> nbLots;

    while (query.next())
    {
        categories << query.value(0).toString();
        quantites << query.value(1).toDouble();
        nbLots << query.value(2).toInt();
    }

    double totalQuantite = 0;
    for (double q : quantites)
        totalQuantite += q;

    QString html = debutGraphique("Répartition par catégorie", "catégorie");

    // Graphique en barres
    html += "<div style='display: flex; flex-direction: column; gap: 15px; padding: 10px 0;'>";

    double maxQuantite = 0;
    for (double q : quantites)
        if (q > maxQuantite)
            maxQuantite = q;

    for (int i = 0; i < categories.size(); i++)
    {
        double pourcentage = (maxQuantite > 0) ? (quantites[i] / maxQuantite * 100) : 0;
        QString couleur = couleurs[i % couleurs.size()];

        html += "<div style='margin-bottom: 15px;'>";
        html += "<div style='display: flex; justify-content: space-between; margin-bottom: 5px;'>";
        html += "<span style='font-weight: bold;'>" + categories[i] + "</span>";
        html += "<span style='color: " + couleur + "; font-weight: bold;'>" + QString::number(quantites[i], 'f', 0) + " KG</span>";
        html += "</div>";

        html += "<div style='width: 100%; background-color: #f0f0f0; border-radius: 8px; height: 30px; overflow: hidden;'>";
        html += "<div style='width: " + QString::number(pourcentage) + "%; height: 100%; background: linear-gradient(90deg, " + couleur + ", " + couleur + "dd); ";
        html += "border-radius: 8px; display: flex; align-items: center; justify-content: flex-end; padding-right: 10px; box-sizing: border-box;'>";
        html += "<span style='color: white; font-size: 12px; font-weight: bold;'>" + QString::number(quantites[i], 'f', 0) + " KG</span>";
        html += "</div>";
        html += "</div>";

        html += "<div style='font-size: 12px; color: #666; margin-top: 3px;'>";
        html += QString::number(nbLots[i]) + " lots • " + QString::number((quantites[i] / totalQuantite) * 100, 'f', 1) + "% du total";
        html += "</div>";
        html += "</div>";
    }

    html += "</div>";

    // Statistiques complémentaires
    html += "<div style='margin-top: 30px; display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px;'>";

    html += "<div style='background: #f8f9fa; padding: 15px; border-radius: 10px; text-align: center;'>";
    html += "<div style='font-size: 12px; color: #666;'>Total olives</div>";
    html += "<div style='font-size: 24px; font-weight: bold; color: #4CAF50;'>" + QString::number(totalQuantite, 'f', 0) + " KG</div>";
    html += "</div>";

    html += "<div style='background: #f8f9fa; padding: 15px; border-radius: 10px; text-align: center;'>";
    html += "<div style='font-size: 12px; color: #666;'>Nombre de lots</div>";
    html += "<div style='font-size: 24px; font-weight: bold; color: #2196F3;'>" + QString::number(categories.size()) + "</div>";
    html += "</div>";

    double moyenne = (categories.size() > 0) ? totalQuantite / categories.size() : 0;
    html += "<div style='background: #f8f9fa; padding: 15px; border-radius: 10px; text-align: center;'>";
    html += "<div style='font-size: 12px; color: #666;'>Moyenne/catégorie</div>";
    html += "<div style='font-size: 24px; font-weight: bold; color: #FF9800;'>" + QString::number(moyenne, 'f', 0) + " KG</div>";
    html += "</div>";

    html += "</div>";

    html += finGraphique();
    return creerLabelGraphique(html);
}

// -------------------------
// Statistiques par quantité
// -------------------------
QLabel *Stocks::creerStatistiquesQuantite(const QList<QString> &couleurs)
{
    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    // FETCH FIRST n ROWS ONLY est la syntaxe Oracle (pas LIMIT qui est SQLite)
    query.exec("SELECT id_stock, qt_stock, categ_stock FROM STOCK ORDER BY qt_stock DESC FETCH FIRST 10 ROWS ONLY");

    QStringList labels;
    QList<double> valeurs;
    QStringList categories;

    while (query.next())
    {
        labels << "Lot " + query.value(0).toString();
        valeurs << query.value(1).toDouble();
        categories << query.value(2).toString();
    }

    double maxQuantite = 0;
    for (double v : valeurs)
        if (v > maxQuantite)
            maxQuantite = v;

    QString html = debutGraphique("Top 10 - Lots les plus importants", "quantité");

    html += "<div style='display: flex; flex-direction: column; gap: 12px;'>";

    for (int i = 0; i < labels.size(); i++)
    {
        double pourcentage = (maxQuantite > 0) ? (valeurs[i] / maxQuantite * 100) : 0;
        QString couleur = couleurs[i % couleurs.size()];

        html += "<div style='background: #f8f9fa; padding: 10px; border-radius: 8px;'>";
        html += "<div style='display: flex; justify-content: space-between; margin-bottom: 5px;'>";
        html += "<span><b>" + labels[i] + "</b> <span style='color: #666; font-size: 11px;'>(" + categories[i] + ")</span></span>";
        html += "<span style='color: " + couleur + "; font-weight: bold;'>" + QString::number(valeurs[i], 'f', 0) + " KG</span>";
        html += "</div>";

        html += "<div style='width: 100%; background-color: #e9ecef; border-radius: 5px; height: 20px; overflow: hidden;'>";
        html += "<div style='width: " + QString::number(pourcentage) + "%; height: 100%; background: linear-gradient(90deg, " + couleur + ", " + couleur + "dd); ";
        html += "border-radius: 5px; display: flex; align-items: center; justify-content: flex-end; padding-right: 5px; box-sizing: border-box;'>";
        html += "<span style='color: white; font-size: 11px; font-weight: bold;'>" + QString::number(valeurs[i], 'f', 0) + "</span>";
        html += "</div>";
        html += "</div>";
        html += "</div>";
    }

    html += "</div>";

    html += finGraphique();
    return creerLabelGraphique(html);
}

// -------------------------
// Statistiques par qualité
// -------------------------
QLabel *Stocks::creerStatistiquesQualite(const QList<QString> &couleurs)
{
    Q_UNUSED(couleurs);

    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    query.exec("SELECT nom_stock, SUM(qt_stock) as total, COUNT(*) as nb FROM STOCK GROUP BY nom_stock ORDER BY "
               "CASE nom_stock WHEN 'Excellente' THEN 1 WHEN 'Bonne' THEN 2 WHEN 'Moyenne' THEN 3 WHEN 'Médiocre' THEN 4 END");

    QStringList qualites;
    QList<double> quantites;
    QList<int> nbLots;

    while (query.next())
    {
        qualites << query.value(0).toString();
        quantites << query.value(1).toDouble();
        nbLots << query.value(2).toInt();
    }

    double total = 0;
    for (double q : quantites)
        total += q;

    QMap<QString, QString> couleurQualite;
    couleurQualite["Excellente"] = "#4CAF50";
    couleurQualite["Bonne"] = "#8BC34A";
    couleurQualite["Moyenne"] = "#FFC107";
    couleurQualite["Médiocre"] = "#F44336";

    QString html = debutGraphique("Répartition par qualité", "qualité");

    html += "<div style='display: flex; flex-wrap: wrap; gap: 20px;'>";

    // Graphique circulaire simplifié
    html += "<div style='position: relative; width: 200px; height: 200px; margin: 0 auto;'>";

    double angleDebut = 0;
    for (int i = 0; i < qualites.size(); i++)
    {
        double angle = (quantites[i] / total) * 360;
        QString couleur = couleurQualite[qualites[i]];

        html += "<div style='position: absolute; width: 200px; height: 200px; border-radius: 50%; ";
        html += "background: conic-gradient(from " + QString::number(angleDebut) + "deg, " + couleur + " 0deg " + QString::number(angle) + "deg, transparent " + QString::number(angle) + "deg 360deg);'></div>";

        angleDebut += angle;
    }

    html += "<div style='position: absolute; width: 100px; height: 100px; background: white; border-radius: 50%; top: 50px; left: 50px; ";
    html += "display: flex; align-items: center; justify-content: center; flex-direction: column; box-shadow: 0 2px 10px rgba(0,0,0,0.1);'>";
    html += "<span style='font-size: 20px; font-weight: bold; color: #333;'>" + QString::number(total, 'f', 0) + "</span>";
    html += "<span style='font-size: 10px; color: #666;'>KG total</span>";
    html += "</div>";

    html += "</div>";

    // Légende
    html += "<div style='flex: 1; min-width: 250px;'>";
    html += "<h4 style='margin-top: 0; color: #333; border-bottom: 2px solid #4CAF50; padding-bottom: 5px;'>Détails par qualité</h4>";

    for (int i = 0; i < qualites.size(); i++)
    {
        double pourcentage = (quantites[i] / total) * 100;
        QString couleur = couleurQualite[qualites[i]];

        html += "<div style='margin-bottom: 15px;'>";
        html += "<div style='display: flex; align-items: center; justify-content: space-between; margin-bottom: 5px;'>";
        html += "<div style='display: flex; align-items: center;'>";
        html += "<div style='width: 15px; height: 15px; background-color: " + couleur + "; margin-right: 8px; border-radius: 3px;'></div>";
        html += "<span style='font-weight: bold;'>" + qualites[i] + "</span>";
        html += "</div>";
        html += "<span>" + QString::number(quantites[i], 'f', 0) + " KG (" + QString::number(pourcentage, 'f', 1) + "%)</span>";
        html += "</div>";

        html += "<div style='width: 100%; background-color: #f0f0f0; border-radius: 5px; height: 20px; overflow: hidden;'>";
        html += "<div style='width: " + QString::number(pourcentage) + "%; height: 100%; background-color: " + couleur + "; border-radius: 5px;'></div>";
        html += "</div>";

        html += "<div style='font-size: 11px; color: #666; margin-top: 3px;'>" + QString::number(nbLots[i]) + " lots</div>";
        html += "</div>";
    }

    html += "</div>";
    html += "</div>";

    html += finGraphique();
    return creerLabelGraphique(html);
}

// -------------------------
// Statistiques temporelles
// -------------------------
QLabel *Stocks::creerStatistiquesTemporal(const QList<QString> &couleurs)
{
    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    // TO_CHAR() est la syntaxe Oracle (strftime() est SQLite)
    query.exec("SELECT TO_CHAR(dateajt_stock, 'YYYY-MM') as mois, SUM(qt_stock) as total "
               "FROM STOCK GROUP BY TO_CHAR(dateajt_stock, 'YYYY-MM') "
               "ORDER BY mois DESC FETCH FIRST 6 ROWS ONLY");

    QStringList mois;
    QList<double> quantites;

    while (query.next())
    {
        mois << query.value(0).toString();
        quantites << query.value(1).toDouble();
    }

    // Inverser pour avoir du plus ancien au plus récent
    std::reverse(mois.begin(), mois.end());
    std::reverse(quantites.begin(), quantites.end());

    double maxQuantite = 0;
    for (double q : quantites)
        if (q > maxQuantite)
            maxQuantite = q;

    QString html = debutGraphique("Évolution des arrivages (6 derniers mois)", "temporel");

    html += "<div style='display: flex; align-items: flex-end; justify-content: space-around; height: 250px; margin: 20px 0; padding: 0 10px;'>";

    for (int i = 0; i < mois.size(); i++)
    {
        double hauteur = (maxQuantite > 0) ? (quantites[i] / maxQuantite) * 200 : 0;
        QString couleur = couleurs[i % couleurs.size()];

        html += "<div style='display: flex; flex-direction: column; align-items: center; width: 60px;'>";
        html += "<div style='height: " + QString::number(hauteur) + "px; width: 40px; background: linear-gradient(to top, " + couleur + ", " + couleur + "dd); ";
        html += "border-radius: 5px 5px 0 0; margin-bottom: 5px; transition: height 0.3s ease;' title='" + QString::number(quantites[i], 'f', 0) + " KG'></div>";
        html += "<div style='font-size: 11px; font-weight: bold; color: " + couleur + ";'>" + QString::number(quantites[i], 'f', 0) + "</div>";
        html += "<div style='font-size: 10px; color: #666; transform: rotate(-45deg); margin-top: 5px;'>" + mois[i] + "</div>";
        html += "</div>";
    }

    html += "</div>";

    // Statistiques de tendance
    if (quantites.size() >= 2)
    {
        double evolution = ((quantites.last() - quantites.first()) / quantites.first()) * 100;
        QString tendance = evolution > 0 ? "📈 Hausse" : (evolution < 0 ? "📉 Baisse" : "➡️ Stable");
        QString couleurTendance = evolution > 0 ? "#4CAF50" : (evolution < 0 ? "#F44336" : "#FF9800");

        html += "<div style='margin-top: 20px; padding: 15px; background: #f8f9fa; border-radius: 8px; text-align: center;'>";
        html += "<div style='font-size: 14px; color: #666;'>Tendance sur la période</div>";
        html += "<div style='font-size: 24px; font-weight: bold; color: " + couleurTendance + ";'>" + tendance + "</div>";
        html += "<div style='font-size: 12px; color: #666;'>" + QString::number(qAbs(evolution), 'f', 1) + "% d'évolution</div>";
        html += "</div>";
    }

    html += finGraphique();
    return creerLabelGraphique(html);
}

// -------------------------
// Métiers avancés - Prédiction de l'output
// -------------------------
QLabel *Stocks::creerPredictionOutput()
{
    // Récupérer les données historiques
    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    // TO_CHAR() est la syntaxe Oracle
    query.exec("SELECT TO_CHAR(dateajt_stock, 'YYYY-MM') as mois, SUM(qt_stock) as total "
               "FROM STOCK GROUP BY TO_CHAR(dateajt_stock, 'YYYY-MM') "
               "ORDER BY mois DESC FETCH FIRST 12 ROWS ONLY");

    QList<double> historiques;
    QList<QString> mois;

    while (query.next())
    {
        mois << query.value(0).toString();
        historiques << query.value(1).toDouble();
    }

    std::reverse(historiques.begin(), historiques.end());
    std::reverse(mois.begin(), mois.end());

    // Calculer une prédiction simple (moyenne mobile + tendance)
    double prediction = 0;
    QString methode = "Moyenne historique";

    if (historiques.size() >= 3)
    {
        // Utiliser les 3 derniers mois avec pondération
        prediction = (historiques.last() * 0.5 + historiques[historiques.size() - 2] * 0.3 + historiques[historiques.size() - 3] * 0.2);
        methode = "Moyenne pondérée (3 mois)";
    }
    else if (!historiques.isEmpty())
    {
        prediction = historiques.last();
    }
    else
    {
        prediction = 1000; // Valeur par défaut
    }

    // Arrondir à 50 près
    prediction = std::round(prediction / 50) * 50;

    QString html = debutGraphique("Prédiction de l'output", "prédiction");

    html += "<div style='text-align: center; padding: 20px;'>";

    // Carte de prédiction principale
    html += "<div style='background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 30px; border-radius: 15px; margin-bottom: 20px;'>";
    html += "<div style='font-size: 16px; opacity: 0.9; margin-bottom: 10px;'>Prédiction pour le mois prochain</div>";
    html += "<div style='font-size: 48px; font-weight: bold; margin: 10px 0;'>" + QString::number(prediction, 'f', 0) + " KG</div>";
    html += "<div style='font-size: 14px; opacity: 0.8;'>Basé sur: " + methode + "</div>";
    html += "</div>";

    // Graphique d'historique
    if (!historiques.isEmpty())
    {
        html += "<div style='margin-top: 30px;'>";
        html += "<h4 style='color: #333; text-align: left;'>Historique des arrivages</h4>";

        double maxHist = 0;
        for (double h : historiques)
            if (h > maxHist)
                maxHist = h;

        html += "<div style='display: flex; align-items: flex-end; justify-content: space-around; height: 150px; margin: 20px 0;'>";

        for (int i = 0; i < qMin(historiques.size(), 6); i++)
        {
            double hauteur = (maxHist > 0) ? (historiques[i] / maxHist) * 120 : 0;
            QString couleur = (i == historiques.size() - 1) ? "#4CAF50" : "#2196F3";

            html += "<div style='display: flex; flex-direction: column; align-items: center; width: 50px;'>";
            html += "<div style='height: " + QString::number(hauteur) + "px; width: 30px; background-color: " + couleur + "; ";
            html += "border-radius: 5px 5px 0 0; margin-bottom: 5px;'></div>";
            html += "<div style='font-size: 10px; color: #666;'>" + QString::number(historiques[i], 'f', 0) + "</div>";
            if (i < mois.size())
            {
                html += "<div style='font-size: 9px; color: #999;'>" + mois[i] + "</div>";
            }
            html += "</div>";
        }

        html += "</div>";
        html += "</div>";
    }

    // Facteurs influents
    html += "<div style='margin-top: 30px; display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px;'>";

    html += "<div style='background: #f8f9fa; padding: 15px; border-radius: 10px;'>";
    html += "<div style='font-size: 12px; color: #666;'>Saison</div>";
    html += "<div style='font-size: 16px; font-weight: bold; color: #FF9800;'>Automne</div>";
    html += "<div style='font-size: 11px; color: #4CAF50;'>+15%</div>";
    html += "</div>";

    html += "<div style='background: #f8f9fa; padding: 15px; border-radius: 10px;'>";
    html += "<div style='font-size: 12px; color: #666;'>Tendance</div>";
    html += "<div style='font-size: 16px; font-weight: bold; color: #2196F3;'>Croissance</div>";
    html += "<div style='font-size: 11px; color: #4CAF50;'>+8%</div>";
    html += "</div>";

    html += "<div style='background: #f8f9fa; padding: 15px; border-radius: 10px;'>";
    html += "<div style='font-size: 12px; color: #666;'>Confiance</div>";
    html += "<div style='font-size: 16px; font-weight: bold; color: #4CAF50;'>Élevée</div>";
    html += "<div style='font-size: 11px; color: #666;'>85%</div>";
    html += "</div>";

    html += "</div>";

    html += finGraphique();
    return creerLabelGraphique(html);
}

// -------------------------
// Métiers avancés - Optimisation réapprovisionnement
// -------------------------
QLabel *Stocks::creerOptimisationReappro()
{
    // Récupérer l'état actuel du stock
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

    // Seuils théoriques
    QMap<QString, double> seuilsOptimaux;
    seuilsOptimaux["Verts"] = 5000;
    seuilsOptimaux["Noirs"] = 3000;
    seuilsOptimaux["Mélange"] = 2000;

    QString html = debutGraphique("Optimisation du réapprovisionnement", "optimisation");

    html += "<div style='display: flex; flex-direction: column; gap: 20px;'>";

    // État actuel vs optimal
    QMapIterator<QString, double> i(stockActuel);
    while (i.hasNext())
    {
        i.next();
        QString categorie = i.key();
        double actuel = i.value();
        double optimal = seuilsOptimaux.value(categorie, 2000);
        double difference = optimal - actuel;
        double pourcentage = (actuel / optimal) * 100;

        QString couleur = (difference > 0) ? "#F44336" : (difference < 0 ? "#FF9800" : "#4CAF50");
        QString statut = (difference > 0) ? "🔴 Sous-stocké" : (difference < 0 ? "🟡 Sur-stocké" : "🟢 Optimal");

        html += "<div style='background: white; border-radius: 10px; padding: 15px; box-shadow: 0 2px 5px rgba(0,0,0,0.05);'>";

        html += "<div style='display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px;'>";
        html += "<span style='font-weight: bold; font-size: 16px;'>" + categorie + "</span>";
        html += "<span style='color: " + couleur + "; font-weight: bold;'>" + statut + "</span>";
        html += "</div>";

        html += "<div style='display: flex; justify-content: space-between; margin-bottom: 5px;'>";
        html += "<span>Actuel: <b>" + QString::number(actuel, 'f', 0) + " KG</b></span>";
        html += "<span>Optimal: <b>" + QString::number(optimal, 'f', 0) + " KG</b></span>";
        html += "</div>";

        // Barre de progression
        html += "<div style='width: 100%; background-color: #f0f0f0; border-radius: 10px; height: 25px; margin: 10px 0; overflow: hidden;'>";
        html += "<div style='width: " + QString::number(qMin(pourcentage, 100.0)) + "%; height: 100%; ";
        html += "background: linear-gradient(90deg, " + couleur + ", " + couleur + "dd); border-radius: 10px; ";
        html += "display: flex; align-items: center; justify-content: flex-end; padding-right: 10px; box-sizing: border-box;'>";
        html += "<span style='color: white; font-size: 12px; font-weight: bold;'>" + QString::number(pourcentage, 'f', 1) + "%</span>";
        html += "</div>";
        html += "</div>";

        // Recommandation
        if (difference > 0)
        {
            html += "<div style='background: #ffebee; color: #c62828; padding: 10px; border-radius: 5px; font-size: 13px;'>";
            html += "📦 Recommandation: Réapprovisionner <b>" + QString::number(difference, 'f', 0) + " KG</b> de " + categorie;
            html += "</div>";
        }
        else if (difference < 0)
        {
            html += "<div style='background: #fff3e0; color: #ef6c00; padding: 10px; border-radius: 5px; font-size: 13px;'>";
            html += "⚠️ Attention: Sur-stock de <b>" + QString::number(-difference, 'f', 0) + " KG</b> - Réduire les commandes";
            html += "</div>";
        }
        else
        {
            html += "<div style='background: #e8f5e8; color: #2e7d32; padding: 10px; border-radius: 5px; font-size: 13px;'>";
            html += "✅ Niveau optimal atteint";
            html += "</div>";
        }

        html += "</div>";
    }

    // Résumé global
    html += "<div style='margin-top: 20px; padding: 20px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; border-radius: 10px;'>";
    html += "<div style='font-size: 16px; margin-bottom: 10px;'>Plan d'action recommandé</div>";

    double totalRecommandation = 0;
    QMapIterator<QString, double> j(stockActuel);
    while (j.hasNext())
    {
        j.next();
        double optimal = seuilsOptimaux.value(j.key(), 2000);
        double diff = optimal - j.value();
        if (diff > 0)
            totalRecommandation += diff;
    }

    html += "<div style='font-size: 32px; font-weight: bold;'>" + QString::number(totalRecommandation, 'f', 0) + " KG</div>";
    html += "<div style='font-size: 14px; opacity: 0.9;'>Quantité totale à réapprovisionner</div>";
    html += "</div>";

    html += finGraphique();
    return creerLabelGraphique(html);
}

// -------------------------
// Métiers avancés - Prédiction des déchets
// -------------------------
QLabel *Stocks::creerPredictionDechet()
{
    // Simuler des données de déchets basées sur la qualité et l'âge
    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    query.exec("SELECT nom_stock, dateajt_stock, qt_stock FROM STOCK");

    double totalDechetPrevu = 0;
    int nbLotsRisques = 0;
    QMap<QString, double> dechetParQualite;

    while (query.next())
    {
        QString qualite = query.value(0).toString();
        // BUG FIX (warning #7) : utiliser query.value().toDate() qui laisse le driver
        // ODBC gérer la conversion nativement, sans dépendre d'un format texte fixe.
        QDate dateAjout = query.value(1).toDate();
        if (!dateAjout.isValid())
            dateAjout = QDate::currentDate(); // fallback si le driver renvoie NULL
        double quantite = query.value(2).toDouble();

        int joursStock = dateAjout.daysTo(QDate::currentDate());

        // Taux de déchet basé sur la qualité et l'âge
        double tauxBase = 0;
        if (qualite == "Excellente")
            tauxBase = 0.05;
        else if (qualite == "Bonne")
            tauxBase = 0.10;
        else if (qualite == "Moyenne")
            tauxBase = 0.20;
        else if (qualite == "Médiocre")
            tauxBase = 0.35;

        // Facteur temps (augmente avec l'âge)
        double facteurTemps = 1.0 + (joursStock / 365.0) * 0.5; // +50% par an
        double tauxReel = qMin(tauxBase * facteurTemps, 0.8);   // Max 80%

        double dechet = quantite * tauxReel;
        totalDechetPrevu += dechet;
        dechetParQualite[qualite] += dechet;

        if (tauxReel > 0.25)
            nbLotsRisques++;
    }

    QString html = debutGraphique("Prédiction des déchets", "déchets");

    html += "<div style='display: grid; grid-template-columns: repeat(2, 1fr); gap: 20px; margin-bottom: 20px;'>";

    // Carte principale
    html += "<div style='background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%); color: white; padding: 20px; border-radius: 10px; grid-column: span 2;'>";
    html += "<div style='font-size: 14px; opacity: 0.9;'>Prédiction de pertes</div>";
    html += "<div style='font-size: 48px; font-weight: bold; margin: 10px 0;'>" + QString::number(totalDechetPrevu, 'f', 0) + " KG</div>";
    html += "<div style='font-size: 14px;'>sur les 3 prochains mois</div>";
    html += "</div>";

    // Lots à risque
    html += "<div style='background: #fff3e0; padding: 15px; border-radius: 10px;'>";
    html += "<div style='font-size: 12px; color: #ef6c00;'>Lots à risque élevé</div>";
    html += "<div style='font-size: 24px; font-weight: bold; color: #f57c00;'>" + QString::number(nbLotsRisques) + "</div>";
    html += "<div style='font-size: 11px; color: #666;'>nécessitent attention</div>";
    html += "</div>";

    // Pourcentage de perte
    double totalStock = 0;
    QSqlQuery queryTotal(db);
    queryTotal.exec("SELECT SUM(qt_stock) FROM STOCK");
    if (queryTotal.next())
        totalStock = queryTotal.value(0).toDouble();

    double pourcentagePerte = (totalStock > 0) ? (totalDechetPrevu / totalStock) * 100 : 0;

    html += "<div style='background: #e8f5e8; padding: 15px; border-radius: 10px;'>";
    html += "<div style='font-size: 12px; color: #2e7d32;'>Taux de perte prévu</div>";
    html += "<div style='font-size: 24px; font-weight: bold; color: #2e7d32;'>" + QString::number(pourcentagePerte, 'f', 1) + "%</div>";
    html += "<div style='font-size: 11px; color: #666;'>du stock total</div>";
    html += "</div>";

    // Détail par qualité
    html += "<div style='grid-column: span 2; margin-top: 10px;'>";
    html += "<h4 style='color: #333; margin: 10px 0;'>Détail par qualité</h4>";

    QStringList ordreQualite = {"Médiocre", "Moyenne", "Bonne", "Excellente"};
    for (const QString &qualite : ordreQualite)
    {
        if (dechetParQualite.contains(qualite) && dechetParQualite[qualite] > 0)
        {
            QString couleur;
            if (qualite == "Excellente")
                couleur = "#4CAF50";
            else if (qualite == "Bonne")
                couleur = "#8BC34A";
            else if (qualite == "Moyenne")
                couleur = "#FFC107";
            else if (qualite == "Médiocre")
                couleur = "#F44336";

            double pourcentage = (totalDechetPrevu > 0) ? (dechetParQualite[qualite] / totalDechetPrevu) * 100 : 0;

            html += "<div style='display: flex; align-items: center; margin-bottom: 8px;'>";
            html += "<div style='width: 10px; height: 10px; background-color: " + couleur + "; margin-right: 10px; border-radius: 2px;'></div>";
            html += "<div style='width: 80px;'>" + qualite + "</div>";
            html += "<div style='flex: 1;'>";
            html += "<div style='width: 100%; background-color: #f0f0f0; border-radius: 5px; height: 15px; overflow: hidden;'>";
            html += "<div style='width: " + QString::number(pourcentage) + "%; height: 100%; background-color: " + couleur + ";'></div>";
            html += "</div>";
            html += "</div>";
            html += "<div style='width: 80px; text-align: right; font-weight: bold;'>" + QString::number(dechetParQualite[qualite], 'f', 0) + " KG</div>";
            html += "</div>";
        }
    }

    html += "</div>";
    html += "</div>";

    html += finGraphique();
    return creerLabelGraphique(html);
}

// -------------------------
// Métiers avancés - Gestion de la qualité
// -------------------------
QLabel *Stocks::creerGestionQualite()
{
    QSqlDatabase db = stockDb();
    QSqlQuery query(db);
    query.exec("SELECT nom_stock, COUNT(*) as nb, SUM(qt_stock) as total FROM STOCK GROUP BY nom_stock");

    QMap<QString, int> nbParQualite;
    QMap<QString, double> quantiteParQualite;

    while (query.next())
    {
        QString qualite = query.value(0).toString();
        nbParQualite[qualite] = query.value(1).toInt();
        quantiteParQualite[qualite] = query.value(2).toDouble();
    }

    QString html = debutGraphique("Gestion de la qualité des olives", "qualité");

    // Indicateurs principaux
    html += "<div style='display: grid; grid-template-columns: repeat(4, 1fr); gap: 10px; margin-bottom: 20px;'>";

    QStringList couleurs = {"#4CAF50", "#8BC34A", "#FFC107", "#F44336"};
    QStringList qualites = {"Excellente", "Bonne", "Moyenne", "Médiocre"};

    for (int i = 0; i < qualites.size(); i++)
    {
        QString qualite = qualites[i];
        QString couleur = couleurs[i];
        int nb = nbParQualite.value(qualite, 0);
        double qte = quantiteParQualite.value(qualite, 0);

        html += "<div style='background: " + couleur + "; color: white; padding: 15px; border-radius: 8px; text-align: center;'>";
        html += "<div style='font-size: 12px; opacity: 0.9;'>" + qualite + "</div>";
        html += "<div style='font-size: 18px; font-weight: bold; margin: 5px 0;'>" + QString::number(qte, 'f', 0) + " KG</div>";
        html += "<div style='font-size: 11px;'>" + QString::number(nb) + " lots</div>";
        html += "</div>";
    }

    html += "</div>";

    // Recommandations de traitement
    html += "<div style='margin: 20px 0;'>";
    html += "<h4 style='color: #333;'>Recommandations par lot</h4>";

    // Récupérer les lots les plus anciens par qualité
    QSqlQuery lotsQuery(db);
    if (!lotsQuery.exec("SELECT id_stock, categ_stock, nom_stock, qt_stock, dateajt_stock, "
                        "TRUNC(SYSDATE) - TRUNC(dateajt_stock) as age_jours "
                        "FROM STOCK WHERE nom_stock IN ('Moyenne', 'Médiocre') "
                        "ORDER BY age_jours DESC FETCH FIRST 5 ROWS ONLY"))
    {
        qDebug() << "[GestionQualite] Erreur lots:" << lotsQuery.lastError().text();
    }

    if (lotsQuery.next())
    {
        html += "<table style='width: 100%; border-collapse: collapse; margin-top: 10px;'>";
        html += "<thead><tr style='background: #f8f9fa;'>";
        html += "<th style='padding: 8px; text-align: left;'>ID</th>";
        html += "<th style='padding: 8px; text-align: left;'>Catégorie</th>";
        html += "<th style='padding: 8px; text-align: left;'>Qualité</th>";
        html += "<th style='padding: 8px; text-align: right;'>Quantité</th>";
        html += "<th style='padding: 8px; text-align: right;'>Âge (jours)</th>";
        html += "<th style='padding: 8px; text-align: left;'>Action</th>";
        html += "</tr></thead><tbody>";

        do
        {
            QString qualite = lotsQuery.value(2).toString();
            QString couleur = (qualite == "Médiocre") ? "#F44336" : "#FFC107";
            int age = lotsQuery.value(5).toInt();

            html += "<tr style='border-bottom: 1px solid #dee2e6;'>";
            html += "<td style='padding: 8px;'>" + lotsQuery.value(0).toString() + "</td>";
            html += "<td style='padding: 8px;'>" + lotsQuery.value(1).toString() + "</td>";
            html += "<td style='padding: 8px; color: " + couleur + "; font-weight: bold;'>" + qualite + "</td>";
            html += "<td style='padding: 8px; text-align: right;'>" + QString::number(lotsQuery.value(3).toDouble(), 'f', 0) + " KG</td>";
            html += "<td style='padding: 8px; text-align: right;'>" + QString::number(age) + "</td>";

            // Action recommandée
            html += "<td style='padding: 8px;'>";
            if (qualite == "Médiocre" || age > 30)
            {
                html += "<span style='background: #ffebee; color: #c62828; padding: 3px 8px; border-radius: 3px; font-size: 11px;'>Traitement urgent</span>";
            }
            else if (qualite == "Moyenne")
            {
                html += "<span style='background: #fff3e0; color: #ef6c00; padding: 3px 8px; border-radius: 3px; font-size: 11px;'>Surveiller</span>";
            }
            html += "</td>";
            html += "</tr>";

        } while (lotsQuery.next());

        html += "</tbody></table>";
    }
    else
    {
        html += "<div style='padding: 20px; text-align: center; color: #666;'>Aucun lot nécessitant une attention particulière</div>";
    }

    html += "</div>";

    // Conseils de conservation
    html += "<div style='margin-top: 20px; padding: 20px; background: #f8f9fa; border-radius: 10px;'>";
    html += "<h4 style='color: #333; margin-top: 0;'>📋 Conseils de conservation</h4>";

    html += "<div style='display: grid; grid-template-columns: repeat(2, 1fr); gap: 15px;'>";

    html += "<div style='background: white; padding: 15px; border-radius: 8px;'>";
    html += "<div style='font-weight: bold; color: #2196F3; margin-bottom: 5px;'>🌡️ Température idéale</div>";
    html += "<div>10-15°C pour olives vertes<br>15-20°C pour olives noires</div>";
    html += "</div>";

    html += "<div style='background: white; padding: 15px; border-radius: 8px;'>";
    html += "<div style='font-weight: bold; color: #4CAF50; margin-bottom: 5px;'>💧 Humidité</div>";
    html += "<div>Maintenir 85-90% d'humidité<br>Éviter la condensation</div>";
    html += "</div>";

    html += "<div style='background: white; padding: 15px; border-radius: 8px;'>";
    html += "<div style='font-weight: bold; color: #FF9800; margin-bottom: 5px;'>⏱️ Durée de conservation</div>";
    html += "<div>Excellente: 6-8 mois<br>Bonne: 4-6 mois<br>Moyenne: 2-4 mois</div>";
    html += "</div>";

    html += "<div style='background: white; padding: 15px; border-radius: 8px;'>";
    html += "<div style='font-weight: bold; color: #F44336; margin-bottom: 5px;'>⚠️ Signes d'alerte</div>";
    html += "<div>Odeur anormale<br>Changement de couleur<br>Moisissures</div>";
    html += "</div>";

    html += "</div>";
    html += "</div>";

    html += finGraphique();
    return creerLabelGraphique(html);
}

// -------------------------
// Afficher les statistiques
// -------------------------
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
    {
        totalLots = query.value(0).toInt();
    }

    if (totalLots == 0)
    {
        afficherMessageInfo("Aucun lot dans la base de données");
        return;
    }

    QString choix = ui->X->currentText();
    QLabel *graphiqueLabel = nullptr;

    // Palette de couleurs
    QList<QString> couleurs = {"#4CAF50", "#2196F3", "#FF9800", "#F44336", "#9C27B0", "#00BCD4"};

    if (choix == "Statistiques générales" || choix == "Par catégorie")
    {
        graphiqueLabel = creerStatistiquesCategorie(couleurs);
    }
    else if (choix == "Par quantité")
    {
        graphiqueLabel = creerStatistiquesQuantite(couleurs);
    }
    else if (choix == "Par qualité")
    {
        graphiqueLabel = creerStatistiquesQualite(couleurs);
    }
    else if (choix == "Évolution temporelle")
    {
        graphiqueLabel = creerStatistiquesTemporal(couleurs);
    }

    if (graphiqueLabel)
    {
        remplacerContenuAvecAnimation(graphiqueLabel);
    }
}

void Stocks::on_X_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (ui->metiersStock->currentWidget() == ui->statStock)
    {
        afficherStatistiques();
    }
}

// -------------------------
// Métiers avancés - Slots
// -------------------------
void Stocks::on_PredictOutput_clicked()
{
    ui->metiersStock->setCurrentWidget(ui->statStock);
    QLabel *graphique = creerPredictionOutput();
    remplacerContenuAvecAnimation(graphique);
}

void Stocks::on_OptimiserReappro_clicked()
{
    ui->metiersStock->setCurrentWidget(ui->statStock);
    QLabel *graphique = creerOptimisationReappro();
    remplacerContenuAvecAnimation(graphique);
}

void Stocks::on_PredictDechet_clicked()
{
    ui->metiersStock->setCurrentWidget(ui->statStock);
    QLabel *graphique = creerPredictionDechet();
    remplacerContenuAvecAnimation(graphique);
}

void Stocks::on_GestionQualite_clicked()
{
    ui->metiersStock->setCurrentWidget(ui->statStock);
    QLabel *graphique = creerGestionQualite();
    remplacerContenuAvecAnimation(graphique);
}