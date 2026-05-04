#include "citernes.h"
#include <QMessageBox>
#include <QApplication>
#include <QFile>
#include "connection.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Charger et appliquer le style
    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleFile.readAll());
        a.setStyleSheet(style);
        styleFile.close();
    }

    // 1) Ouvrir la connexion BD AVANT de créer la fenêtre
    Connection c;
    bool test = c.createconnect();

    if (test) {
        Citernes w;   // 2) Créer et afficher la fenêtre Citernes
        w.show();
        QMessageBox::information(
            nullptr,
            QObject::tr("database is open"),
            QObject::tr("connection successful.\n"
                        "Click Cancel to exit."),
            QMessageBox::Cancel
            );
        return a.exec();
    } else {
        QMessageBox::critical(
            nullptr,
            QObject::tr("database is not open"),
            QObject::tr("connection failed.\n"
                        "Click Cancel to exit."),
            QMessageBox::Cancel
            );
        return 0; // pas de boucle d'événements si la BD est fermée
    }
}
