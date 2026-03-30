#include "stocks.h"
#include "connection.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Connection c;
    if (!c.createconnect()) {
        // Afficher le vrai message d'erreur ODBC pour faciliter le diagnostic
        QMessageBox::critical(nullptr, "Erreur de connexion",
                              "Impossible de se connecter à la base de données.\n\n"
                              "Détail de l'erreur :\n" + c.lastError() +
                                  "\n\nVérifiez :\n"
                                  "• La source ODBC 'Source_Projet2A' existe dans le Panneau ODBC Windows\n"
                                  "• Le service Oracle est démarré\n"
                                  "• Le nom d'utilisateur et le mot de passe sont corrects");
        return -1;
    }

    Stocks w;
    w.show();
    return a.exec();
}