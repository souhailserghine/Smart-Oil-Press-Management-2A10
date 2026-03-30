#include "connection.h"
<<<<<<< HEAD
#include <QDebug>

Connection::Connection()
{
}

bool Connection::createconnect()
{
    bool test = false;

    // Pour voir quels drivers Qt voit
    qDebug() << "Drivers SQL disponibles:" << QSqlDatabase::drivers();

    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");
    db.setDatabaseName("Source_Projet2A"); // nom de la source de données ODBC
    db.setUserName("imen");
    db.setPassword("Imen2005");

    if (db.open()) {
        test = true;
    } else {
        qDebug() << "Erreur ouverture BD:" << db.lastError().text();
    }

    return test;
=======

Connection::Connection()
{

}

bool Connection::createconnect()
{bool test=false;
QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");
db.setDatabaseName("Source_Projet2A");//inserer le nom de la source de données
db.setUserName("souhail");//inserer nom de l'utilisateur
db.setPassword("0000");//inserer mot de passe de cet utilisateur

if (db.open())
test=true;





    return  test;
>>>>>>> parent of 7b797a2 (Merge pull request #1 from souhailserghine/gestionmachines)
}
