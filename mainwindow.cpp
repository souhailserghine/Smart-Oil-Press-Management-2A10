#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tableau->setColumnCount(10);

    ui->tableau->setHorizontalHeaderLabels({
        "ID","Nom","Prenom","Numero","Adresse",
        "Nb Arbres","Type","Mail","Region","Action"
    });
}
void MainWindow::on_ajouterEmpBtn_clicked()
{
    QString nom = ui->nom->text();
    QString prenom = ui->Prenom->text();
    QString numero = ui->Numero->text();
    QString adresse = ui->adresse->text();
    int nb = ui->nbArbre->text().toInt();
    QString type = ui->typeolive->text();
    QString mail = ui->Mail->text();
    QString region = ui->Region->text();

    Agriculteur a(0, nom, prenom, adresse, numero, mail, region,
                  nb, type, 0, 0, 0, "", 0);
    //listeAgriculteurs.push_back(a);
    QSqlQuery query;
    query.prepare("INSERT INTO AGRICULTEUR (nom_agri, prenom_agri, adresse_agri, num_agri, mail_agri, region_agri, nb_arbres, type_olives) "
                  "VALUES (:nom,:prenom,:adresse,:num,:mail,:region,:nb,:type)");

    query.bindValue(":nom", nom);
    query.bindValue(":prenom", prenom);
    query.bindValue(":adresse", adresse);
    query.bindValue(":num", numero);
    query.bindValue(":mail", mail);
    query.bindValue(":region", region);
    query.bindValue(":nb", nb);
    query.bindValue(":type", type);
    if(!query.exec())
        qDebug() << "Erreur INSERT:" << query.lastError().text();
    else
        qDebug() << "Insertion réussie";
    afficherTableau();
    ui->metierspersonnel->setCurrentIndex(1);
}
void MainWindow::on_quitter_clicked()
{
    QApplication::quit();
}
/*void MainWindow::afficherTableau()
{
    ui->tableau->setRowCount(0);
    for(int i = 0; i < listeAgriculteurs.size(); i++)
    {
        ui->tableau->insertRow(i);

        ui->tableau->setItem(i, 0, new QTableWidgetItem(QString::number(i+1)));
        ui->tableau->setItem(i, 1, new QTableWidgetItem(listeAgriculteurs[i].getNom()));
        ui->tableau->setItem(i, 2, new QTableWidgetItem(listeAgriculteurs[i].getPrenom()));
        ui->tableau->setItem(i, 3, new QTableWidgetItem(listeAgriculteurs[i].getNumero()));
        ui->tableau->setItem(i, 4, new QTableWidgetItem(listeAgriculteurs[i].getAdresse()));
        ui->tableau->setItem(i, 5, new QTableWidgetItem(QString::number(listeAgriculteurs[i].getNbArbres())));
        ui->tableau->setItem(i, 6, new QTableWidgetItem(listeAgriculteurs[i].getType()));
        ui->tableau->setItem(i, 7, new QTableWidgetItem(listeAgriculteurs[i].getMail()));
        ui->tableau->setItem(i, 8, new QTableWidgetItem(listeAgriculteurs[i].getRegion()));
        QPushButton *btnSupprimer = new QPushButton("Supprimer");
        QPushButton *btnModifier = new QPushButton("Modifier");

        QWidget *widget = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(widget);

        layout->addWidget(btnSupprimer);
        layout->addWidget(btnModifier);
        layout->setContentsMargins(0,0,0,0);

        ui->tableau->setCellWidget(i, 9, widget);
        connect(btnSupprimer, &QPushButton::clicked, this, [=]() {
            listeAgriculteurs.remove(i);
            afficherTableau();
        });
        connect(btnModifier, &QPushButton::clicked, this, [=]() {

            indexModification = i;

            // remplir les champs page 3
            ui->Mnom->setText(listeAgriculteurs[i].getNom());
            ui->Mprenom->setText(listeAgriculteurs[i].getPrenom());
            ui->Mnumero->setText(listeAgriculteurs[i].getNumero());
            ui->Madresse->setText(listeAgriculteurs[i].getAdresse());
            ui->Mnom->setText(listeAgriculteurs[i].getNom());
            ui->Mprenom->setText(listeAgriculteurs[i].getPrenom());
            ui->Mnumero->setText(listeAgriculteurs[i].getNumero());
            ui->Madresse->setText(listeAgriculteurs[i].getAdresse());
            ui->MnbArbre->setText(QString::number(listeAgriculteurs[i].getNbArbres()));
            ui->Mtypeolive->setText(listeAgriculteurs[i].getType());
            ui->Mmail->setText(listeAgriculteurs[i].getMail());
            ui->Mregion->setText(listeAgriculteurs[i].getRegion());
            ui->metierspersonnel->setCurrentIndex(2);
        });
    }
}*/
void MainWindow::afficherTableau()
{
    ui->tableau->setRowCount(0);

    QSqlQuery query;
    query.exec("SELECT * FROM AGRICULTEUR");

    int i = 0;

    while(query.next())
    {
        ui->tableau->insertRow(i);

        int id = query.value("id_agri").toInt();

        ui->tableau->setItem(i, 0, new QTableWidgetItem(QString::number(id)));
        ui->tableau->setItem(i, 1, new QTableWidgetItem(query.value("nom_agri").toString()));
        ui->tableau->setItem(i, 2, new QTableWidgetItem(query.value("prenom_agri").toString()));
        ui->tableau->setItem(i, 3, new QTableWidgetItem(query.value("num_agri").toString()));
        ui->tableau->setItem(i, 4, new QTableWidgetItem(query.value("adresse_agri").toString()));
        ui->tableau->setItem(i, 5, new QTableWidgetItem(query.value("nb_arbres").toString()));
        ui->tableau->setItem(i, 6, new QTableWidgetItem(query.value("type_olives").toString()));
        ui->tableau->setItem(i, 7, new QTableWidgetItem(query.value("mail_agri").toString()));
        ui->tableau->setItem(i, 8, new QTableWidgetItem(query.value("region_agri").toString()));

        QPushButton *btnSupprimer = new QPushButton("Supprimer");
        QPushButton *btnModifier = new QPushButton("Modifier");

        // widget pour mettre les 2 boutons
        QWidget *widget = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(widget);

        layout->addWidget(btnSupprimer);
        layout->addWidget(btnModifier);
        layout->setContentsMargins(0,0,0,0);

        ui->tableau->setCellWidget(i, 9, widget);

        connect(btnSupprimer, &QPushButton::clicked, this, [=]() {
            QSqlQuery q;
            q.prepare("DELETE FROM AGRICULTEUR WHERE id_agri=:id");
            q.bindValue(":id", id);

            if(!q.exec())
                qDebug() << q.lastError().text();
            else
                qDebug() << "Supprimé";

            afficherTableau();
        });
        QString nom = query.value("nom_agri").toString();
        QString prenom = query.value("prenom_agri").toString();
        QString num = query.value("num_agri").toString();
        QString adresse = query.value("adresse_agri").toString();
        QString nb = query.value("nb_arbres").toString();
        QString type = query.value("type_olives").toString();
        QString mail = query.value("mail_agri").toString();
        QString region = query.value("region_agri").toString();
        connect(btnModifier, &QPushButton::clicked, this, [=]() {

            idSelectionne = id;

            ui->Mnom->setText(nom);
            ui->Mprenom->setText(prenom);
            ui->Mnumero->setText(num);
            ui->Madresse->setText(adresse);
            ui->MnbArbre->setText(nb);
            ui->Mtypeolive->setText(type);
            ui->Mmail->setText(mail);
            ui->Mregion->setText(region);

            ui->metierspersonnel->setCurrentIndex(2);
        });
        i++;
    }
}
void MainWindow::on_btn_valider_modif_clicked()
{
    QSqlQuery query;

    query.prepare("UPDATE AGRICULTEUR SET "
                  "nom_agri=:nom, "
                  "prenom_agri=:prenom, "
                  "num_agri=:num, "
                  "adresse_agri=:adresse, "
                  "nb_arbres=:nb, "
                  "type_olives=:type, "
                  "mail_agri=:mail, "
                  "region_agri=:region "
                  "WHERE id_agri=:id");

    query.bindValue(":nom", ui->Mnom->text());
    query.bindValue(":prenom", ui->Mprenom->text());
    query.bindValue(":num", ui->Mnumero->text());
    query.bindValue(":adresse", ui->Madresse->text());
    query.bindValue(":nb", ui->MnbArbre->text().toInt());
    query.bindValue(":type", ui->Mtypeolive->text());
    query.bindValue(":mail", ui->Mmail->text());
    query.bindValue(":region", ui->Mregion->text());
    query.bindValue(":id", idSelectionne);

    if(!query.exec())
        qDebug() << query.lastError().text();
    else
        qDebug() << "Modification réussie";

    afficherTableau();
    ui->metierspersonnel->setCurrentIndex(1);
}
void MainWindow::on_btn_retour_clicked()
{
    ui->metierspersonnel->setCurrentIndex(1);
}
MainWindow::~MainWindow()
{
    delete ui;
}
