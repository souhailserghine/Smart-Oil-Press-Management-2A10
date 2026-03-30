#ifndef CONNECTION_H
#define CONNECTION_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

class Connection
{
public:
    Connection();
    bool createconnect();

    // Retourne le vrai message d'erreur ODBC/Oracle en cas d'échec
    QString lastError() const;

private:
    QString m_lastError;
};

#endif // CONNECTION_H