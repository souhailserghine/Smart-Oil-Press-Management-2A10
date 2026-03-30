#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextDocument>
#include <QPrinter>
#include <QPageSize>
#include <QPageLayout>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->metiersstocks->setCurrentWidget(ui->ajoutqtolives);

    // BUG FIX #2 : colonnes mises à jour pour correspondre à la table STOCK
    ui->tableWidget->setColumnCount(6);
    ui->tableWidget->setHorizontalHeaderLabels(
        {"ID", "Catégorie", "Description", "Date d'ajout", "Quantité (KG)", "Qualité"});
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setAlternatingRowColors(true);
    ui->tableWidget->verticalHeader()->setVisible(false);

    // Connexion manuelle pour la recherche en temps réel
    connect(ui->lineEdit, &QLineEdit::textChanged, this, &MainWindow::chargerListeOlives);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnConsulterstc_clicked()
{
    ui->metiersstocks->setCurrentWidget(ui->consulterqtolives);
    chargerListeOlives();
}

void MainWindow::on_btnAjouterstc_clicked()
{
    ui->metiersstocks->setCurrentWidget(ui->ajoutqtolives);
}

void MainWindow::on_btnStatstc_clicked()
{
    ui->metiersstocks->setCurrentWidget(ui->statqtolives);
}

// BUG FIX #2 : slot renommé de on_toolButton_5_clicked → on_btnmetieravance_clicked
void MainWindow::on_btnmetieravance_clicked()
{
    ui->metiersstocks->setCurrentWidget(ui->metieravancee);
}

void MainWindow::chargerListeOlives()
{
    QString recherche = ui->lineEdit->text().trimmed();

    QSqlDatabase db = QSqlDatabase::database("ConnexionPrincipale");
    QSqlQuery query(db);

    if (recherche.isEmpty())
    {
        query.prepare("SELECT id_olive, categorie, description, date_ajout, quantite, qualite"
                      " FROM STOCK"
                      " ORDER BY id_olive DESC");
    }
    else
    {
        // Paramètres liés pour éviter l'injection SQL
        query.prepare("SELECT id_olive, categorie, description, date_ajout, quantite, qualite"
                      " FROM STOCK"
                      " WHERE UPPER(categorie)   LIKE UPPER(:s1)"
                      "    OR UPPER(description) LIKE UPPER(:s2)"
                      "    OR UPPER(qualite)     LIKE UPPER(:s3)"
                      " ORDER BY id_olive DESC");
        QString param = "%" + recherche + "%";
        query.bindValue(":s1", param);
        query.bindValue(":s2", param);
        query.bindValue(":s3", param);
    }

    if (!query.exec())
    {
        QMessageBox::critical(this, "Erreur SQL", query.lastError().text());
        return;
    }

    ui->tableWidget->setRowCount(0);

    int row = 0;
    while (query.next()) {
        ui->tableWidget->insertRow(row);
        for (int col = 0; col < 6; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(query.value(col).toString());
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            ui->tableWidget->setItem(row, col, item);
        }
        row++;
    }

    statusBar()->showMessage(QString("%1 lots chargés").arg(row), 3000);
}

void MainWindow::on_ajouterqtoliveBtn_clicked()
{
    // BUG FIX #2 : le formulaire mainwindow.ui n'a pas de champ "qualité".
    // Le champ "Nom" et "Prénom" sont fusionnés dans la description.
    // La qualité est insérée avec la valeur par défaut 'Bonne'.
    QString nom         = ui->nomLineEdit->text().trimmed();
    QString prenom      = ui->prNomLineEdit->text().trimmed();
    QString categorie   = ui->Categchoix->currentText();
    QDate   dateAjout   = ui->dateDEmbaucheDateEdit->date();
    QString quantiteStr = ui->prNomLineEdit_2->text().trimmed();
    QString description = ui->description->text().trimmed();

    // Fusionner nom et prénom dans la description si description est vide
    QString descriptionFinale = description;
    if (descriptionFinale.isEmpty()) {
        descriptionFinale = (nom + " " + prenom).trimmed();
    }

    if (descriptionFinale.isEmpty() || quantiteStr.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez remplir au moins la Description et la Quantité.");
        return;
    }

    bool ok = false;
    double quantite = quantiteStr.toDouble(&ok);
    if (!ok || quantite <= 0 || quantite > 100000) {
        QMessageBox::warning(this, "Erreur", "Quantité invalide (doit être entre 0 et 100000 KG).");
        return;
    }

    // BUG FIX : INSERT dans STOCK avec les bonnes colonnes.
    // Les dates sont liées comme QDate (pas toString) pour compatibilité ODBC/Oracle.
    QSqlDatabase db = QSqlDatabase::database("ConnexionPrincipale");
    QSqlQuery q(db);
    q.prepare("INSERT INTO STOCK (categorie, description, date_ajout, date_maj, quantite, qualite) "
              "VALUES (:categ, :descript, :dateajt, :datemaj, :qt, 'Bonne')");

    q.bindValue(":categ",    categorie);
    q.bindValue(":descript", descriptionFinale);
    q.bindValue(":dateajt",  dateAjout);               // QDate lié directement via ODBC
    q.bindValue(":datemaj",  QDate::currentDate());
    q.bindValue(":qt",       quantite);

    if (q.exec()) {
        QMessageBox::information(this, "Succès", "Lot ajouté avec succès !");
        ui->nomLineEdit->clear();
        ui->prNomLineEdit->clear();
        ui->prNomLineEdit_2->clear();
        ui->description->clear();
        ui->dateDEmbaucheDateEdit->setDate(QDate::currentDate());
        ui->Categchoix->setCurrentIndex(0);
        on_btnConsulterstc_clicked();
    } else {
        QMessageBox::critical(this, "Erreur SQL", q.lastError().text());
    }
}

// BUG FIX #2 : slot renommé de on_toolButton_2_clicked → on_modifierButton_clicked
void MainWindow::on_modifierButton_clicked()
{
    ui->lineEdit->clear();
    chargerListeOlives();
}

// BUG FIX #2 : slot renommé de on_toolButton_clicked → on_RechercheButton_clicked
void MainWindow::on_RechercheButton_clicked()
{
    chargerListeOlives();
}

void MainWindow::on_pushButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(
        this, "Exporter en PDF",
        QString("STOCK_%1.pdf").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "Fichiers PDF (*.pdf)");

    if (fileName.isEmpty()) return;

    // BUG FIX : requête sur STOCK avec les bonnes colonnes
    QSqlDatabase db = QSqlDatabase::database("ConnexionPrincipale");
    QSqlQuery query(db);
    query.exec("SELECT id_olive, categorie, description, date_ajout, quantite, qualite "
               "FROM STOCK ORDER BY date_ajout DESC");

    QString html =
        "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
        "<style>body{font-family:Arial;margin:20px;}h1{color:#4CAF50;text-align:center;}"
        "table{width:100%;border-collapse:collapse;margin-top:20px;}"
        "th{background-color:#4CAF50;color:white;padding:10px;text-align:left;}"
        "td{border:1px solid #ddd;padding:8px;}tr:nth-child(even){background-color:#f2f2f2;}"
        "</style></head><body>"
        "<h1>Stock d'Olives - Smart Press</h1>"
        "<p>Exporté le : " + QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm") + "</p>"
                                                                      "<table><thead><tr>"
                                                                      "<th>ID</th><th>Catégorie</th><th>Description</th>"
                                                                      "<th>Date d'ajout</th><th>Quantité (KG)</th><th>Qualité</th>"
                                                                      "</tr></thead><tbody>";

    // BUG FIX (warning #6) : ne pas utiliser query.size() qui retourne -1 en ODBC.
    int count = 0;
    while (query.next()) {
        html += "<tr>";
        for (int i = 0; i < 6; i++)
            html += "<td>" + query.value(i).toString() + "</td>";
        html += "</tr>";
        count++;
    }
    html += "</tbody></table>";
    html += "<p><b>Total : " + QString::number(count) + " lots</b></p>";
    html += "</body></html>";

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