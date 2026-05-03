#include "mainwindow.h"
#include "connection.h"

#include <QApplication>
#include <QFile>
#include <QMessageBox>

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

    if (!Connection::createInstance().createConnection()) {
        QMessageBox::critical(nullptr, "Erreur de connexion",
                              "Impossible de se connecter à la base de données.\n"
                              "Vérifiez votre DSN ODBC et vos identifiants.");
        return -1;
    }

    MainWindow w;
    w.show();
    return a.exec();
}

