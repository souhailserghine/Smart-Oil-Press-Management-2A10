#include "agriculteur.h"

Agriculteur::Agriculteur()
{
}
Agriculteur::Agriculteur(int id, QString nom, QString prenom, QString adresse,
                         QString numero, QString mail, QString region,
                         int nbArbres, QString typeOlives,
                         float qt, float qtPrec, float rendement,
                         QString date, float note)
{
    this->idAgri = id;
    this->nom = nom;
    this->prenom = prenom;
    this->adresse = adresse;
    this->numero = numero;
    this->mail = mail;
    this->region = region;
    this->nombreArbres = nbArbres;
    this->typeOlives = typeOlives;
    this->qtOlives = qt;
    this->qtOlivesAnneePrecedente = qtPrec;
    this->rendementMoyen = rendement;
    this->datePremiereVisite = date;
    this->noteQualiteMoyenne = note;
}

Agriculteur::~Agriculteur()
{
}

int Agriculteur::getId() { return idAgri; }
QString Agriculteur::getNom() { return nom; }
QString Agriculteur::getPrenom() { return prenom; }
float Agriculteur::getRendement() { return rendementMoyen; }
void Agriculteur::setNom(QString n) { nom = n; }
void Agriculteur::setPrenom(QString p) { prenom = p; }
void Agriculteur::setNumero(QString n) { numero = n; }
void Agriculteur::setAdresse(QString a) { adresse = a; }
void Agriculteur::setNbArbres(int nb) { nombreArbres = nb; }
void Agriculteur::setType(QString t) { typeOlives = t; }
void Agriculteur::setMail(QString m) { mail = m; }
void Agriculteur::setRegion(QString r) { region = r; }
QString Agriculteur::getNumero() { return numero; }
QString Agriculteur::getAdresse() { return adresse; }
int Agriculteur::getNbArbres() { return nombreArbres; }
QString Agriculteur::getType() { return typeOlives; }
QString Agriculteur::getMail() { return mail; }
QString Agriculteur::getRegion() { return region; }
float Agriculteur::calculPrediction()
{
    return (qtOlives + qtOlivesAnneePrecedente) / 2;
}

float Agriculteur::calculScore()
{
    return (noteQualiteMoyenne * 0.5) + (rendementMoyen * 0.3);
}

bool Agriculteur::estEnRisque()
{
    if(noteQualiteMoyenne < 5 && rendementMoyen < 50)
        return true;
    return false;
}