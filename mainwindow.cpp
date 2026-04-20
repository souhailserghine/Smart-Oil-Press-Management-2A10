#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QRegularExpressionValidator>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->tableau->setHorizontalHeaderLabels({
        "ID","Nom","Prenom","Numero","Adresse",
        "Nb Arbres","Type","Mail","Region","Action"
    });
    // NOM / PRENOM / REGION (lettres seulement)
    QRegularExpression rxNom("^[A-Za-z ]+$");
    ui->nom->setValidator(new QRegularExpressionValidator(rxNom, this));
    ui->Prenom->setValidator(new QRegularExpressionValidator(rxNom, this));
    ui->Region->setValidator(new QRegularExpressionValidator(rxNom, this));
    ui->MnbArbre->setReadOnly(true);
    ui->Mtypeolive->setReadOnly(true);

    // NUMERO (chiffres)
    QRegularExpression rxNum("^[0-9]+$");
    ui->Numero->setValidator(new QRegularExpressionValidator(rxNum, this));
    afficherTableau();
}

void MainWindow::on_ajouterEmpBtn_clicked()
{
    QString nom = ui->nom->text();
    QString prenom = ui->Prenom->text();
    QString numero = ui->Numero->text();
    QString adresse = ui->adresse->text();
    QString mail = ui->Mail->text();
    QString region = ui->Region->text();

    bool valid = true;

    // RESET erreurs
    ui->errorNom->setText("");
    ui->errorPrenom->setText("");
    ui->errorNumero->setText("");
    ui->errorMail->setText("");
    ui->errorRegion->setText("");
    ui->errorAdresse->setText("");

    // NOM
    if(nom.isEmpty())
    {
        ui->errorNom->setText("Nom obligatoire");
        valid = false;
    }

    // PRENOM
    if(prenom.isEmpty())
    {
        ui->errorPrenom->setText("Prenom obligatoire");
        valid = false;
    }

    // NUMERO
    if(numero.isEmpty())
    {
        ui->errorNumero->setText("Numero obligatoire");
        valid = false;
    }
    else if(numero.length() < 8)
    {
        ui->errorNumero->setText("Numero invalide");
        valid = false;
    }

    // MAIL
    if(!mail.contains("@"))
    {
        ui->errorMail->setText("Email invalide");
        valid = false;
    }
    // ADRESSE
    if(adresse.isEmpty())
    {
        ui->errorAdresse->setText("Adresse obligatoire");
        valid = false;
    }

    // REGION
    if(region.isEmpty())
    {
        ui->errorRegion->setText("Region obligatoire");
        valid = false;
    }

    if(!valid)
        return;

    // INSERT SQL
    QSqlQuery query;
    query.prepare("INSERT INTO AGRICULTEUR "
                  "(nom_agri, prenom_agri, adresse_agri, num_agri, mail_agri, region_agri) "
                  "VALUES (:nom,:prenom,:adresse,:num,:mail,:region)");

    query.bindValue(":nom", nom);
    query.bindValue(":prenom", prenom);
    query.bindValue(":adresse", adresse);
    query.bindValue(":num", numero);
    query.bindValue(":mail", mail);
    query.bindValue(":region", region);

    if(!query.exec())
        qDebug() << "Erreur INSERT:" << query.lastError().text();
    else
        QMessageBox::information(this, "Succès", "Agriculteur ajouté !");

    afficherTableau();
    ui->metierspersonnel->setCurrentIndex(1);
}
void MainWindow::on_btn_valider_modif_clicked()
{
    QString nom = ui->Mnom->text();
    QString prenom = ui->Mprenom->text();
    QString numero = ui->Mnumero->text();
    QString adresse = ui->Madresse->text();
    QString mail = ui->Mmail->text();
    QString region = ui->Mregion->text();

    bool valid = true;

    // RESET erreurs
    ui->errorMNom->setText("");
    ui->errorMPrenom->setText("");
    ui->errorMNumero->setText("");
    ui->errorMAdresse->setText("");
    ui->errorMMail->setText("");
    ui->errorMRegion->setText("");

    // NOM
    if(nom.isEmpty())
    {
        ui->errorMNom->setText("Nom obligatoire");
        valid = false;
    }

    // PRENOM
    if(prenom.isEmpty())
    {
        ui->errorMPrenom->setText("Prenom obligatoire");
        valid = false;
    }

    // NUMERO
    if(numero.isEmpty())
    {
        ui->errorMNumero->setText("Numero obligatoire");
        valid = false;
    }
    else if(numero.length() < 8)
    {
        ui->errorMNumero->setText("Numero invalide");
        valid = false;
    }

    // ADRESSE
    if(adresse.isEmpty())
    {
        ui->errorMAdresse->setText("Adresse obligatoire");
        valid = false;
    }

    // MAIL
    if(!mail.contains("@"))
    {
        ui->errorMMail->setText("Email invalide");
        valid = false;
    }

    // REGION
    if(region.isEmpty())
    {
        ui->errorMRegion->setText("Region obligatoire");
        valid = false;
    }

    if(!valid)
        return;

    // UPDATE SQL
    QSqlQuery query;
    query.prepare("UPDATE AGRICULTEUR SET "
                  "nom_agri=:nom, "
                  "prenom_agri=:prenom, "
                  "num_agri=:num, "
                  "adresse_agri=:adresse, "
                  "mail_agri=:mail, "
                  "region_agri=:region "
                  "WHERE id_agri=:id");

    query.bindValue(":nom", nom);
    query.bindValue(":prenom", prenom);
    query.bindValue(":num", numero);
    query.bindValue(":adresse", adresse);
    query.bindValue(":mail", mail);
    query.bindValue(":region", region);
    query.bindValue(":id", idSelectionne);

    if(!query.exec())
        qDebug() << query.lastError().text();
    else
        QMessageBox::information(this, "Succès", "Modification réussie !");

    afficherTableau();
    ui->metierspersonnel->setCurrentIndex(1);
}
void MainWindow::on_quitter_clicked()
{
    QApplication::quit();
}
void MainWindow::afficherTableauAvecQuery(QString queryStr)
{
    ui->tableau->setRowCount(0);
    ui->tableau->setColumnCount(12);

    ui->tableau->setHorizontalHeaderLabels({
        "ID","Nom","Prenom","Numero","Adresse",
        "Nb Arbres","Type","Mail","Region",
        "Quantité Stock","Date Stock","Action"
    });

    QSqlQuery query;
    if(!query.exec(queryStr))
    {
        qDebug() << "ERREUR SQL :" << query.lastError().text();
        qDebug() << "REQUETE :" << queryStr;
        return;
    }

    int i = 0;
    while(query.next())
    {
        ui->tableau->insertRow(i);

        int id = query.value("ID_AGRI").toInt();

        ui->tableau->setItem(i, 0, new QTableWidgetItem(QString::number(id)));
        ui->tableau->setItem(i, 1, new QTableWidgetItem(query.value("NOM_AGRI").toString()));
        ui->tableau->setItem(i, 2, new QTableWidgetItem(query.value("PRENOM_AGRI").toString()));
        ui->tableau->setItem(i, 3, new QTableWidgetItem(query.value("NUM_AGRI").toString()));
        ui->tableau->setItem(i, 4, new QTableWidgetItem(query.value("ADRESSE_AGRI").toString()));
        QString nbArbres = query.value("NB_ARBRES").isNull()
                               ? "0"
                               : query.value("NB_ARBRES").toString();

        QString typeOlives = query.value("TYPE_OLIVES").isNull()
                                 ? "-"
                                 : query.value("TYPE_OLIVES").toString();
        ui->tableau->setItem(i, 5, new QTableWidgetItem(nbArbres));
        ui->tableau->setItem(i, 6, new QTableWidgetItem(typeOlives));
        ui->tableau->setItem(i, 7, new QTableWidgetItem(query.value("MAIL_AGRI").toString()));
        ui->tableau->setItem(i, 8, new QTableWidgetItem(query.value("REGION_AGRI").toString()));
        QString qtStock = query.value("QT_STOCK").isNull()
                              ? "0"
                              : query.value("QT_STOCK").toString();

        QString dateStock = query.value("DATEMAJ_STOCK").isNull()
                                ? "-"
                                : query.value("DATEMAJ_STOCK").toDate().toString("dd/MM/yyyy");

        ui->tableau->setItem(i, 9, new QTableWidgetItem(qtStock));
        ui->tableau->setItem(i, 10, new QTableWidgetItem(dateStock));

        QPushButton *btnSupprimer = new QPushButton("Supprimer");
        QPushButton *btnModifier = new QPushButton("Modifier");
        QPushButton *btnHistorique = new QPushButton("Historique");

        QWidget *widget = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(widget);

        layout->addWidget(btnSupprimer);
        layout->addWidget(btnModifier);
        layout->addWidget(btnHistorique);
        layout->setContentsMargins(0,0,0,0);

        ui->tableau->setCellWidget(i, 11, widget);
        connect(btnHistorique, &QPushButton::clicked, this, [=]() {
            idSelectionne = id;

            ui->anneeH->clear();
            ui->quantiteH->clear();
            ui->nbArbreH->clear();
            ui->typeH->clear();
            ui->noteH->clear();
            ui->dateRecolteH->setDate(QDate::currentDate());

            afficherHistorique(id);
            ui->metierspersonnel->setCurrentIndex(3);
        });

        // DELETE
        connect(btnSupprimer, &QPushButton::clicked, this, [=]() {
            QSqlQuery q;
            q.prepare("DELETE FROM HISTORIQUE_OLIVES WHERE ID_AGRI=:id");
            q.bindValue(":id", id);
            q.exec();

            q.prepare("DELETE FROM STOCK WHERE ID_AGRI=:id");
            q.bindValue(":id", id);
            q.exec();

            q.prepare("DELETE FROM AGRICULTEUR WHERE ID_AGRI=:id");
            q.bindValue(":id", id);
            q.exec();
            afficherTableau();
        });

        // UPDATE
        QString nom = query.value("nom_agri").toString();
        QString prenom = query.value("prenom_agri").toString();
        QString numero = query.value("num_agri").toString();
        QString adresse = query.value("adresse_agri").toString();
        QString type = query.value("type_olives").toString();
        QString mail = query.value("mail_agri").toString();
        QString region = query.value("region_agri").toString();

        connect(btnModifier, &QPushButton::clicked, this, [=]() {

            idSelectionne = id;

            ui->Mnom->setText(nom);
            ui->Mprenom->setText(prenom);
            ui->Mnumero->setText(numero);
            ui->Madresse->setText(adresse);
            ui->MnbArbre->setText(nbArbres);
            ui->Mtypeolive->setText(type);
            ui->Mmail->setText(mail);
            ui->Mregion->setText(region);

            ui->metierspersonnel->setCurrentIndex(2);
        });


        i++;
    }
}
void MainWindow::afficherTableau()
{
    QString query =
        "SELECT A.ID_AGRI, A.NOM_AGRI, A.PRENOM_AGRI, A.NUM_AGRI, A.ADRESSE_AGRI, "
        "A.NB_ARBRES, A.TYPE_OLIVES, A.MAIL_AGRI, A.REGION_AGRI, "
        "S.QT_STOCK, S.DATEMAJ_STOCK "
        "FROM AGRICULTEUR A "
        "LEFT JOIN ("
        "    SELECT ID_AGRI, QT_STOCK, DATEMAJ_STOCK "
        "    FROM STOCK "
        "    WHERE (ID_AGRI, DATEMAJ_STOCK) IN ("
        "        SELECT ID_AGRI, MAX(DATEMAJ_STOCK) "
        "        FROM STOCK "
        "        GROUP BY ID_AGRI"
        "    )"
        ") S ON A.ID_AGRI = S.ID_AGRI";

    afficherTableauAvecQuery(query);
}
void MainWindow::afficherHistorique(int idAgri)
{
    ui->tableHistorique->setRowCount(0);
    QSqlQuery q;
    q.prepare("SELECT NB_ARBRES, TYPE_OLIVES FROM AGRICULTEUR WHERE ID_AGRI=:id");
    q.bindValue(":id", idAgri);
    q.exec();

    if(q.next())
    {
        ui->nbArbreH->setText(q.value(0).toString());
        ui->typeH->setText(q.value(1).toString());
    }

    QSqlQuery query;

    query.prepare(
        "SELECT H.ID_HIST, H.ANNEE, H.QUANTITE, H.NB_ARBRES, H.TYPE_OLIVES, H.NOTE, "
        "(SELECT MAX(DATEMAJ_STOCK) FROM STOCK S WHERE S.ID_AGRI = H.ID_AGRI) AS DATE_STOCK "
        "FROM HISTORIQUE_OLIVES H "
        "WHERE H.ID_AGRI = :id "
        "ORDER BY H.ANNEE DESC"
        );
    query.bindValue(":id", idAgri);
    query.exec();
    ui->tableHistorique->setColumnCount(7);

    ui->tableHistorique->setHorizontalHeaderLabels({
        "Année","Quantité","Nb arbres","Type","Note","Date","Action"
    });


    int i = 0;
    while(query.next())
    {
        ui->tableHistorique->insertRow(i);

        int idHist = query.value(0).toInt(); // on garde mais on affiche pas

        QString annee = query.value(1).toString();
        QString quantite = query.value(2).toString();
        QString nbArbres = query.value(3).toString();
        QString type = query.value(4).toString();
        QString note = query.value(5).toString();
        QString dateStock = query.value(6).toDate().toString("dd/MM/yyyy");

        ui->tableHistorique->setItem(i, 0, new QTableWidgetItem(annee));
        ui->tableHistorique->setItem(i, 1, new QTableWidgetItem(quantite));
        ui->tableHistorique->setItem(i, 2, new QTableWidgetItem(nbArbres));
        ui->tableHistorique->setItem(i, 3, new QTableWidgetItem(type));
        ui->tableHistorique->setItem(i, 4, new QTableWidgetItem(note));
        ui->tableHistorique->setItem(i, 5, new QTableWidgetItem(dateStock));

        QPushButton *btnSupprimer = new QPushButton("Supprimer");
        QPushButton *btnModifier = new QPushButton("Modifier");

        QWidget *widget = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(widget);

        layout->addWidget(btnModifier);
        layout->addWidget(btnSupprimer);
        layout->setContentsMargins(0,0,0,0);

        ui->tableHistorique->setCellWidget(i, 6, widget);

        connect(btnSupprimer, &QPushButton::clicked, this, [=]() {
            QSqlQuery q;
            q.prepare("DELETE FROM HISTORIQUE_OLIVES WHERE ID_HIST=:id");
            q.bindValue(":id", idHist);
            q.exec();

            afficherHistorique(idSelectionne);
        });

        connect(btnModifier, &QPushButton::clicked, this, [=]() {
            ui->anneeH_2->setText(annee);
            ui->quantiteH_2->setText(quantite);
            ui->nbArbreH_2->setText(nbArbres);
            ui->typeH_2->setText(type);
            ui->noteH_2->setText(note);

            // stocker l'id pour update
            idSelectionneHistorique = idHist;

            ui->metierspersonnel->setCurrentIndex(4);
        });

        i++;
    }
    QSqlQuery qStock;
    qStock.prepare(
        "SELECT QT_STOCK, DATEMAJ_STOCK FROM STOCK "
        "WHERE ID_AGRI=:id "
        "AND DATEMAJ_STOCK = (SELECT MAX(DATEMAJ_STOCK) FROM STOCK WHERE ID_AGRI=:id)"
        );
    qStock.bindValue(":id", idAgri);
    qStock.exec();

    if(qStock.next())
    {
        int row = ui->tableHistorique->rowCount();
        ui->tableHistorique->insertRow(row);

        ui->tableHistorique->setItem(row, 0, new QTableWidgetItem("ACTUEL"));
        ui->tableHistorique->setItem(row, 1, new QTableWidgetItem(qStock.value(0).toString()));
        ui->tableHistorique->setItem(row, 2, new QTableWidgetItem("-"));
        ui->tableHistorique->setItem(row, 3, new QTableWidgetItem("-"));
        ui->tableHistorique->setItem(row, 4, new QTableWidgetItem("-"));
        ui->tableHistorique->setItem(row, 5,
                                     new QTableWidgetItem(qStock.value(1).toDate().toString("dd/MM/yyyy")));
    }
}
void MainWindow::on_retour_2_clicked()
{
    afficherHistorique(idSelectionne);
    ui->metierspersonnel->setCurrentIndex(3);
}
void MainWindow::on_modifier_2_clicked()
{
    QSqlQuery q;
    q.prepare("UPDATE HISTORIQUE_OLIVES SET "
              "ANNEE=:annee, QUANTITE=:qt, NB_ARBRES=:nb, TYPE_OLIVES=:type, NOTE=:note "
              "WHERE ID_HIST=:id");

    q.bindValue(":annee", ui->anneeH_2->text());
    q.bindValue(":qt", ui->quantiteH_2->text());
    q.bindValue(":nb", ui->nbArbreH_2->text());
    q.bindValue(":type", ui->typeH_2->text());
    q.bindValue(":note", ui->noteH_2->text());
    q.bindValue(":id", idSelectionneHistorique);

    if(!q.exec())
    {
        qDebug() << q.lastError().text();
        return;
    }

    afficherHistorique(idSelectionne);
    ui->metierspersonnel->setCurrentIndex(3);

    QMessageBox::information(this, "Succès", "Modification effectuée !");
}

void MainWindow::on_btnAjouterHistorique_clicked()
{
    QString anneeStr = ui->anneeH->text();
    QString qtStr = ui->quantiteH->text();
    QString nbStr = ui->nbArbreH->text();
    QString type = ui->typeH->text();
    QString noteStr = ui->noteH->text();
    QDate dateRec = ui->dateRecolteH->date();

    // 🔴 CONTROLE DE SAISIE
    if(anneeStr.isEmpty() || qtStr.isEmpty() || nbStr.isEmpty() || type.isEmpty() || noteStr.isEmpty())
    {
        QMessageBox::warning(this, "Erreur", "Tous les champs sont obligatoires !");
        return;
    }

    int annee = anneeStr.toInt();
    float qt = qtStr.toFloat();
    int nb = nbStr.toInt();
    float note = noteStr.toFloat();

    if(annee < 2000 || annee > 2100)
    {
        QMessageBox::warning(this, "Erreur", "Année invalide !");
        return;
    }

    if(qt <= 0 || nb <= 0 || note < 0 || note > 10)
    {
        QMessageBox::warning(this, "Erreur", "Valeurs invalides !");
        return;
    }

    // 🔵 1. INSERT HISTORIQUE
    QSqlQuery queryHist;
    queryHist.prepare("INSERT INTO HISTORIQUE_OLIVES "
                      "(ID_AGRI, ANNEE, NB_ARBRES, TYPE_OLIVES, QUANTITE, NOTE, DATERECOLTE) "
                      "VALUES (:id, :annee, :nb, :type, :qt, :note, :dateRec)");
    queryHist.bindValue(":id", idSelectionne);
    queryHist.bindValue(":annee", annee);
    queryHist.bindValue(":nb", nb);
    queryHist.bindValue(":type", type);
    queryHist.bindValue(":qt", qt);
    queryHist.bindValue(":note", note);
    queryHist.bindValue(":dateRec", dateRec);

    if(!queryHist.exec())
    {
        qDebug() << "Erreur HISTORIQUE:" << queryHist.lastError().text();
        return;
    }

    // 🟢 2. INSERT STOCK
    QSqlQuery queryStock;
    queryStock.prepare(
        "INSERT INTO STOCK "
        "(ID_STOCK, NOM_STOCK, CATEG_STOCK, DESCRIPT_STOCK, QT_STOCK, ID_AGRI, DATEMAJ_STOCK) "
        "VALUES (seq_stock.NEXTVAL, 'Stock Olives', 'Olives', 'Ajout', :qt, :id, SYSDATE)"
        );

    queryStock.bindValue(":qt", qt);
    queryStock.bindValue(":id", idSelectionne);

    if(!queryStock.exec())
    {
        qDebug() << "Erreur STOCK:" << queryStock.lastError().text();
        return;
    }
    QSqlQuery updateAgri;
    updateAgri.prepare(
        "UPDATE AGRICULTEUR SET "
        "NB_ARBRES = :nb, "
        "TYPE_OLIVES = :type, "
        "DATERECOLTE = :dateRec "
        "WHERE ID_AGRI = :id"
        );

    updateAgri.bindValue(":nb", nb);
    updateAgri.bindValue(":type", type);
    updateAgri.bindValue(":dateRec", dateRec);
    updateAgri.bindValue(":id", idSelectionne);

    updateAgri.exec();

    QMessageBox::information(this, "Succès", "Historique + Stock ajoutés !");

    // 🔄 REFRESH
    afficherHistorique(idSelectionne);
    afficherTableau();
    ui->anneeH->clear();
    ui->quantiteH->clear();
    ui->nbArbreH->clear();
    ui->typeH->clear();
    ui->noteH->clear();
    ui->dateRecolteH->setDate(QDate::currentDate());
}

void MainWindow::on_btnTrier_clicked()
{
    QString choix = ui->comboTri->currentText();
    QString ordre = ui->comboOrdre->currentText();

    QString orderSql = (ordre == "Ascendant") ? "ASC" : "DESC";

    QString queryStr = "SELECT * FROM AGRICULTEUR";

    if(choix == "Rendement moyen")
        queryStr += " ORDER BY RENDE_MOY " + orderSql;
    else if(choix == "Quantité olives")
        queryStr += " ORDER BY QTOLIVES_ANNEEPREC " + orderSql;
    else if(choix == "Région")
        queryStr += " ORDER BY REGION_AGRI " + orderSql;

    afficherTableauAvecQuery(queryStr);
}
void MainWindow::on_retour_clicked()
{
    afficherTableau(); // refresh
    ui->metierspersonnel->setCurrentIndex(1); // revenir à consulter
}
void MainWindow::on_btn_retour_clicked()
{
    afficherTableau();
    ui->metierspersonnel->setCurrentIndex(1);
}
MainWindow::~MainWindow()
{
    delete ui;
}
