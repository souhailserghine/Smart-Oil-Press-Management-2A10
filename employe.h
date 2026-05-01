#ifndef EMPLOYE_H
#define EMPLOYE_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QString>
#include <QDate>
#include <QByteArray>
// SerialPort module (qmake: QT += serialport) must be linked in CMake via Qt6::SerialPort (or Qt5::SerialPort).

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
            const QByteArray& modeleFaciale = QByteArray(),
            const QString& fingerid        = QString());

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
    QString    getFingerId()      const;

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
    void setFingerId(const QString& fingerId);

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


    /**
     * @brief Checks whether a fingerprint ID exists in EMPLOYE table.
     * @param fingerId Fingerprint ID returned by sensor.
     * @return true if at least one employee has this finger_id.
     */
    bool existsByFingerId(const QString& fingerId);

    /**
     * @brief Returns "prenom nom" for a given fingerprint ID.
     * @param fingerId Fingerprint ID returned by sensor.
     * @return Full name if found, empty string otherwise.
     */
    QString fullNameByFingerId(const QString& fingerId);

    /**
     * @brief Finds an employee by fingerprint ID.
     * @param fingerId Fingerprint sensor ID (template slot).
     * @param outEmployeeId Output parameter: employee ID if found.
     * @param outFullName Output parameter: full name if found.
     * @return true if employee found, false otherwise.
     */
    bool findByFingerprintId(const QString& fingerId, int &outEmployeeId, QString &outFullName);

    /**
     * @brief Updates an employee's fingerprint ID in the database.
     * @param employeeId Employee ID.
     * @param fingerprintId New fingerprint ID from sensor.
     * @return true on success, false on database error.
     */
    bool updateFingerprintId(int employeeId, const QString &fingerprintId);

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
    QString    m_fingerId;

    mutable QSqlError m_lastError;
};

#endif // EMPLOYE_H
