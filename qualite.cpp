#include "qualite.h"
#include "ui_qualite.h"
#include "connection.h"

qualite::qualite(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::qualite)
{
    ui->setupUi(this);
    ui->metiersqualite->setCurrentIndex(1); // Consulter par défaut
    afficherHuile();

    // Connexion manuelle du bouton de tri - CORRIGÉE
    connect(ui->trihuileButton, &QToolButton::clicked, this, &qualite::on_trihuileButton_clicked);
}
qualite::~qualite()
{
    delete ui;
}

// ── Bouton "Ajouter" dans la navbar ────────────────────────────────────────
void qualite::on_btnAjouterHuile_clicked()
{
    clearFields();
    ui->metiersqualite->setCurrentIndex(0); // Affiche le formulaire
}

// ── Bouton "Consulter" dans la navbar ──────────────────────────────────────
void qualite::on_btnConsulterHuile_clicked()
{
    ui->metiersqualite->setCurrentIndex(1); // Retour au tableau
    afficherHuile();
}

// ── INSERT ──────────────────────────────────────────────────────────────────
void qualite::on_ajouterHuileBtn_clicked()
{
    QString nomResp     = ui->nomresponsableLineEdit->text().trimmed();
    QString dateStr     = ui->dateprodDateEdit->date().toString("yyyy-MM-dd");
    QString statut      = ui->statutComboBox->currentText();
    QString quantite    = ui->quantiteLineEdit->text().trimmed();
    QString maxqt       = ui->maxqtLineEdit->text().trimmed();
    QString ph          = ui->phLineEdit->text().trimmed();
    QString temp        = ui->idstockLineEdit->text().trimmed();
    QString acidite     = ui->aciditeComboBox->currentText();
    QString codeCouleur = ui->codecouleurLineEdit->text().trimmed();
    QString amerture    = ui->amertureLineEdit->text().trimmed();

    // Validation
    if (nomResp.isEmpty() || quantite.isEmpty() || maxqt.isEmpty() ||
        ph.isEmpty() || temp.isEmpty() || codeCouleur.isEmpty() || amerture.isEmpty())
    {
        QMessageBox::warning(this, "Champs vides", "Veuillez remplir tous les champs.");
        return;
    }

    // ODBC utilise ? comme placeholder
    QSqlQuery query;
    query.prepare("INSERT INTO QUALITE "
                  "(date_production, quantite_produite, temperature_production, "
                  "ph, acidite, amerture, code_couleur, statut_qualite, "
                  "responsable_controle, max_quantite) "
                  "VALUES "
                  "(TO_DATE(?, 'YYYY-MM-DD'), ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    query.addBindValue(dateStr);
    query.addBindValue(quantite.toDouble());
    query.addBindValue(temp.toDouble());
    query.addBindValue(ph.toDouble());
    query.addBindValue(acidite);
    query.addBindValue(amerture.toDouble());
    query.addBindValue(codeCouleur);
    query.addBindValue(statut);
    query.addBindValue(nomResp);
    query.addBindValue(maxqt.toDouble());

    if (query.exec()) {
        QMessageBox::information(this, "Succès", "Huile ajoutée avec succès !");
        clearFields();
        ui->metiersqualite->setCurrentIndex(1); // Retour au tableau après ajout
        afficherHuile();
    } else {
        QMessageBox::critical(this, "Erreur",
                              "Échec de l'ajout :\n" + query.lastError().text());
        qDebug() << "SQL Error:" << query.lastError().text();
    }
}

// ── READ sans tri ──────────────────────────────────────────────────────────
void qualite::afficherHuile()
{
    QSqlQuery query;
    query.prepare("SELECT id_lot, responsable_controle, date_production, "
                  "quantite_produite, ph, acidite, amerture, statut_qualite, "
                  "temperature_production, code_couleur "
                  "FROM QUALITE ORDER BY id_lot");

    ui->huiletableWidget->setRowCount(0);

    if (!query.exec()) {
        qDebug() << "Select error:" << query.lastError().text();
        return;
    }

    int row = 0;
    while (query.next()) {
        ui->huiletableWidget->insertRow(row);
        ui->huiletableWidget->setItem(row, 0, new QTableWidgetItem(query.value("id_lot").toString()));
        ui->huiletableWidget->setItem(row, 1, new QTableWidgetItem(query.value("responsable_controle").toString()));
        ui->huiletableWidget->setItem(row, 2, new QTableWidgetItem(query.value("date_production").toString()));
        ui->huiletableWidget->setItem(row, 3, new QTableWidgetItem(query.value("quantite_produite").toString()));
        ui->huiletableWidget->setItem(row, 4, new QTableWidgetItem(query.value("ph").toString()));
        ui->huiletableWidget->setItem(row, 5, new QTableWidgetItem(query.value("acidite").toString()));
        ui->huiletableWidget->setItem(row, 6, new QTableWidgetItem(query.value("amerture").toString()));
        ui->huiletableWidget->setItem(row, 7, new QTableWidgetItem(query.value("statut_qualite").toString()));
        ui->huiletableWidget->setItem(row, 8, new QTableWidgetItem(query.value("temperature_production").toString()));
        ui->huiletableWidget->setItem(row, 9, new QTableWidgetItem(query.value("code_couleur").toString()));
        row++;
    }
    ajusterLargeurColonnes();
}

// ── TRI PAR DATE DÉCROISSANTE ──────────────────────────────────────────────
void qualite::on_trihuileButton_clicked()
{
    // Requête SQL avec ORDER BY date_production DESC
    QSqlQuery query;
    query.prepare("SELECT id_lot, responsable_controle, date_production, "
                  "quantite_produite, ph, acidite, amerture, statut_qualite, "
                  "temperature_production, code_couleur "
                  "FROM QUALITE ORDER BY date_production DESC");

    // Vider le tableau
    ui->huiletableWidget->setRowCount(0);

    // Exécuter la requête
    if (!query.exec()) {
        qDebug() << "Erreur SQL:" << query.lastError().text();
        QMessageBox::warning(this, "Erreur", "Erreur lors du tri: " + query.lastError().text());
        return;
    }

    // Remplir le tableau avec les résultats triés
    int row = 0;
    while (query.next()) {
        ui->huiletableWidget->insertRow(row);
        ui->huiletableWidget->setItem(row, 0, new QTableWidgetItem(query.value("id_lot").toString()));
        ui->huiletableWidget->setItem(row, 1, new QTableWidgetItem(query.value("responsable_controle").toString()));
        ui->huiletableWidget->setItem(row, 2, new QTableWidgetItem(query.value("date_production").toString()));
        ui->huiletableWidget->setItem(row, 3, new QTableWidgetItem(query.value("quantite_produite").toString()));
        ui->huiletableWidget->setItem(row, 4, new QTableWidgetItem(query.value("ph").toString()));
        ui->huiletableWidget->setItem(row, 5, new QTableWidgetItem(query.value("acidite").toString()));
        ui->huiletableWidget->setItem(row, 6, new QTableWidgetItem(query.value("amerture").toString()));
        ui->huiletableWidget->setItem(row, 7, new QTableWidgetItem(query.value("statut_qualite").toString()));
        ui->huiletableWidget->setItem(row, 8, new QTableWidgetItem(query.value("temperature_production").toString()));
        ui->huiletableWidget->setItem(row, 9, new QTableWidgetItem(query.value("code_couleur").toString()));
        row++;
    }

    // Afficher un message de confirmation (optionnel)
    QMessageBox::information(this, "Tri", QString("%1 lignes triées par date (plus récentes en premier)").arg(row));
    ajusterLargeurColonnes();
}

// ── Clear fields ────────────────────────────────────────────────────────────
void qualite::clearFields()
{
    ui->nomresponsableLineEdit->clear();
    ui->quantiteLineEdit->clear();
    ui->phLineEdit->clear();
    ui->idstockLineEdit->clear();
    ui->codecouleurLineEdit->clear();
    ui->amertureLineEdit->clear();
    ui->maxqtLineEdit->clear();
    ui->dateprodDateEdit->setDate(QDate(2000, 1, 1));
    ui->statutComboBox->setCurrentIndex(0);
    ui->aciditeComboBox->setCurrentIndex(0);
}

void qualite::on_btnAjouterHuile_triggered(QAction *arg1)
{
    Q_UNUSED(arg1)
}
void qualite::ajusterLargeurColonnes()
{
    // Ajuster la largeur des colonnes pour que le contenu soit lisible
    ui->huiletableWidget->resizeColumnsToContents();

    // Ou définir des largeurs fixes pour chaque colonne
    ui->huiletableWidget->setColumnWidth(0, 60);   // ID lot
    ui->huiletableWidget->setColumnWidth(1, 120);  // Nom resp
    ui->huiletableWidget->setColumnWidth(2, 100);  // Date prod
    ui->huiletableWidget->setColumnWidth(3, 80);   // Qt prod
    ui->huiletableWidget->setColumnWidth(4, 50);   // pH
    ui->huiletableWidget->setColumnWidth(5, 80);   // Acidité
    ui->huiletableWidget->setColumnWidth(6, 80);   // Amerture
    ui->huiletableWidget->setColumnWidth(7, 80);   // Statut
    ui->huiletableWidget->setColumnWidth(8, 100);  // Temp de prod
    ui->huiletableWidget->setColumnWidth(9, 100);  // Code Couleur
    ui->huiletableWidget->setColumnWidth(10, 80);  // Actions

    // Permettre aux colonnes de s'adapter à la largeur de la fenêtre
    ui->huiletableWidget->horizontalHeader()->setStretchLastSection(true);

    // Ou permettre à toutes les colonnes de s'étendre
    // ui->huiletableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}
