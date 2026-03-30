#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");
    db.setDatabaseName("source_projet2A");
    db.setUserName("abdelmajid");
    db.setPassword("123");

    if(db.open())
        qDebug() << "Connected to Oracle";
    else
        qDebug() << db.lastError().text();
    qDebug() << QSqlDatabase::drivers();

    MainWindow w;
    w.show();

    return a.exec();
}