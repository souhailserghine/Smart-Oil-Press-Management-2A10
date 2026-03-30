#ifndef CONNECTION_H
#define CONNECTION_H

#include <QSqlDatabase>
#include <QDebug>
#include <QSqlError>


class Connection {
public:


    static Connection& createInstance(); // Méthode qui retourne une référence unique
    bool createConnection();  // Méthode utilisée pour établir la connexion à la base
    ~Connection(); //Destructeur public pour fermer proprement la connexion et pour empêcher toute destruction non contrôlée de l’instance Singleton


private:

    // attribut Connexion persistante: attribut d’une instance qui vit longtemps (le Singleton)
    QSqlDatabase db;

    /******Méthodes privées*****/
    /**************************/

    Connection(); //Constructeur privé pour eviter la création d'autres instances

    // Interdire la copie & l'affectation ==> Suppression pour garantir l’unicité de l'instance de connexion
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
};

#endif // CONNECTION_H


