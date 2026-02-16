#include "mainwindow.h"
#include "connection.h"          // ← ajoute cet include

#include <QApplication>
#include <QFile>
#include <QMessageBox>           // ← ajoute cet include

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Load and apply stylesheet
    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleFile.readAll());
        a.setStyleSheet(style);
        styleFile.close();
    }

    // ── Connexion à la base de données ──────────────────────────
    if (!Connection::createInstance().createConnection()) {
        QMessageBox::critical(nullptr, "Erreur de connexion",
                              "Impossible de se connecter à la base de données.\n"
                              "Vérifiez votre DSN ODBC et vos identifiants.");
        return -1;
    }
    // ────────────────────────────────────────────────────────────

    MainWindow w;
    Connection c;
    bool test = c.createconnect();
    if (test) {
        w.show();
        QMessageBox::information(nullptr, QObject::tr("database is open"),
                                 QObject::tr("connection successful.\n"
                                             "Click Cancel to exit."), QMessageBox::Cancel);
    } else {
        QMessageBox::critical(nullptr, QObject::tr("database is not open"),
                              QObject::tr("connection failed.\n"
                                          "Click Cancel to exit."), QMessageBox::Cancel);
    }
    return a.exec();
}





