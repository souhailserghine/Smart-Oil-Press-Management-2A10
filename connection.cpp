#include "connection.h"
#include <QSqlError>
#include <QDebug>

Connection::Connection()
{
}

bool Connection::createconnect()
{
    // Vérifier que le driver QODBC est disponible
    if (!QSqlDatabase::isDriverAvailable("QODBC")) {
        m_lastError = "Le driver QODBC n'est pas disponible sur cette machine.\n"
                      "Installez le package Qt SQL ODBC.";
        return false;
    }

    // Éviter les doublons si la connexion existe déjà
    if (QSqlDatabase::contains("ConnexionPrincipale")) {
        QSqlDatabase::removeDatabase("ConnexionPrincipale");
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", "ConnexionPrincipale");

    // Nom de la source ODBC telle que configurée dans le panneau ODBC Windows
    db.setDatabaseName("Source_Projet2A");
    db.setUserName("mohamedbarrak");
    db.setPassword("Moniabarrak2001");
    db.setConnectOptions("SQL_ATTR_CONNECTION_TIMEOUT=10");

    if (db.open()) {
        return true;
    }

    // Capturer le vrai message d'erreur Oracle/ODBC pour faciliter le diagnostic
    m_lastError = db.lastError().driverText() + "\n" + db.lastError().databaseText();
    qDebug() << "[Connexion] Erreur:" << m_lastError;
    return false;
}

QString Connection::lastError() const
{
    return m_lastError;
}