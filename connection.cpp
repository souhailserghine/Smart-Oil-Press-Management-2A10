#include "connection.h"
#include <QSqlDatabase>

bool Connection::create()
{
    // DSN QODBC اللي عاملها في ODBC Data Sources
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");
    db.setDatabaseName("Source_Projet2A");   // <-- بدّلها لاسم DSN عندك
    db.setUserName("ayoubbouzidi");            // <-- user oracle
    db.setPassword("99533203");        // <-- password

    return db.open();
}
