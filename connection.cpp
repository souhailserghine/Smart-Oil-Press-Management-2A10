#include "connection.h"
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
}
