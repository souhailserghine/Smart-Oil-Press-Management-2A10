#ifndef AGRICULTEUR_H
#define AGRICULTEUR_H

#include <QString>

class Agriculteur
{
private:
    int idAgri;
    QString nom;
    QString prenom;
    QString adresse;
    QString numero;
    QString mail;
    QString region;
    int nombreArbres;
    QString typeOlives;
    float qtOlives;
    float qtOlivesAnneePrecedente;
    float rendementMoyen;
    QString datePremiereVisite;
    float noteQualiteMoyenne;
public:
    Agriculteur();
    Agriculteur(int, QString, QString, QString, QString, QString,
                QString, int, QString, float, float, float, QString, float);
    ~Agriculteur();
    int getId();
    QString getNom();
    QString getPrenom();
    float getRendement();
    QString getNumero();
    QString getAdresse();
    int getNbArbres();
    QString getType();
    QString getMail();
    QString getRegion();
    float calculRisque();
    QString recommandation();
    void setNumero(QString);
    void setAdresse(QString);
    float calculNoteGlobale(float moyenneHistorique);
    void setNbArbres(int);
    void setType(QString);
    void setMail(QString);
    void setRegion(QString);
    void setNom(QString);
    void setPrenom(QString);
    float calculPrediction();
    float calculScore();
    QString prioriteTraitement();
    bool productiviteAnormale(float moyenneHistorique);
    bool estEnRisque();
    float getQtOlives();
    QString detectionIntelligente(float moyenneHistorique);
    float getQtOlivesPrec();
    float getNote();
};

#endif
