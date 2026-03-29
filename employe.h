#ifndef EMPLOYE_H
#define EMPLOYE_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QString>
#include <QDate>
#include <QByteArray>

class Employe
{
public:
    // --- Constructors ---
    Employe();
    Employe(int id_emp, const QString& nom_emp, const QString& prenom_emp,
            const QString& email, const QString& role, const QString& mdp,
            const QDate& dateEmbauche      = QDate(),
            const QByteArray& photo        = QByteArray(),
            const QByteArray& empreinte    = QByteArray(),
            const QByteArray& modeleFaciale = QByteArray());

    // --- Getters ---
    int        getIdEmp()         const;
    QString    getNomEmp()        const;
    QString    getPrenomEmp()     const;
    QString    getEmail()         const;
    QString    getRole()          const;
    QString    getMdp()           const;
    QDate      getDateEmbauche()  const;
    QByteArray getPhoto()         const;
    QByteArray getEmpreinte()     const;
    QByteArray getModeleFaciale() const;

    // --- Setters ---
    void setIdEmp(int id);
    void setNomEmp(const QString& nom);
    void setPrenomEmp(const QString& prenom);
    void setEmail(const QString& email);
    void setRole(const QString& role);
    void setMdp(const QString& mdp);
    void setDateEmbauche(const QDate& date);
    void setPhoto(const QByteArray& photo);
    void setEmpreinte(const QByteArray& empreinte);
    void setModeleFaciale(const QByteArray& modele);

    // --- CRUD operations ---


    bool ajouter();


    bool supprimer(int id_emp);


    bool modifier();

    /**
     * @brief Loads all employees from the database into a QSqlQueryModel.
     * @return A heap-allocated model populated with all employee rows.
     *         Caller takes ownership.
     */
    QSqlQueryModel* afficher();

    /**
     * @brief Searches employees by a given column.
     * @param critere  Column to filter on: "nom_emp", "prenom_emp", "email", "role", "date_embauche".
     * @param valeur   Partial value to match (case-insensitive LIKE search).
     * @return A heap-allocated model with matching rows. Caller takes ownership.
     */
    QSqlQueryModel* rechercher(const QString& critere, const QString& valeur);


    int authenticate(const QString& email, const QString& mdp);

    // --- Additional DB helpers used by MainWindow (UI stays in MainWindow) ---
    /** Returns {"Nom Prénom", true} for an existing id_emp, otherwise {"", false}. */
    static bool getFullNameById(int id_emp, QString* outFullName, QSqlError* outError = nullptr);

    /** Returns counts by role: role -> count. */
    static bool getCountByRole(QMap<QString, int>* outRoleCounts, int* outTotal = nullptr, QSqlError* outError = nullptr);

    /** Returns all stored face embeddings: id_emp -> raw blob (128 floats). */
    static bool getAllFaceModels(QMap<int, QByteArray>* outModels, QSqlError* outError = nullptr);

    /**
     * @brief List employees as (id_emp, "Nom Prénom") for combo boxes.
     * @return A heap-allocated model. Caller takes ownership.
     */
    static QSqlQueryModel* modelIdFullName(QSqlError* outError = nullptr);


    QSqlError lastError() const;

private:
    int        m_idEmp;
    QString    m_nomEmp;
    QString    m_prenomEmp;
    QString    m_email;
    QString    m_role;
    QString    m_mdp;
    QDate      m_dateEmbauche;
    QByteArray m_photo;
    QByteArray m_empreinte;
    QByteArray m_modeleFaciale;

    mutable QSqlError m_lastError;
};

#endif // EMPLOYE_H
