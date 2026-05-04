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
/*float Agriculteur::calculPrediction()
{
    float base = qtOlives;

    float tendance = qtOlives - qtOlivesAnneePrecedente;

    float facteurRendement = rendementMoyen / 100.0;

    float prediction = base + (tendance * 0.7) + (facteurRendement * 50);

    return prediction;
}*/
float Agriculteur::calculPrediction()
{
    float tendance = qtOlives - qtOlivesAnneePrecedente;

    float facteurQualite = noteQualiteMoyenne / 10.0;
    float facteurRendement = rendementMoyen / 100.0;

    float prediction =
        qtOlives
        + (tendance * 0.6)
        + (facteurRendement * 40)
        + (facteurQualite * 30);

    return prediction;
}
QString Agriculteur::prioriteTraitement()
{
    if(estEnRisque() && qtOlives > 500)
        return "🔴 URGENT";

    if(noteQualiteMoyenne < 6)
        return "🟡 NORMAL";

    return "🟢 FAIBLE";
}
float Agriculteur::calculScore()
{
    float production = log10(qtOlives + 1);
    float productionScore = qMin(production * 2.5f, 10.0f);
    float croissance = 0;
    if(qtOlivesAnneePrecedente > 0)
        croissance = (qtOlives - qtOlivesAnneePrecedente) / qtOlivesAnneePrecedente;

    float croissanceScore = 5 + (croissance * 5);
    croissanceScore = qBound(0.0f, croissanceScore, 10.0f);

    float qualiteScore = noteQualiteMoyenne;
    float score =
        (productionScore * 0.5f) +
        (croissanceScore * 0.3f) +
        (qualiteScore * 0.2f);

    return qBound(0.0f, score, 10.0f);
}
bool Agriculteur::productiviteAnormale(float moyenneHistorique)
{
    if(moyenneHistorique == 0)
        return false;

    float ratio = qtOlives / moyenneHistorique;

    if(ratio < 0.7 || ratio > 1.3)
        return true;

    return false;
}
float Agriculteur::getQtOlives() { return qtOlives; }
float Agriculteur::getQtOlivesPrec() { return qtOlivesAnneePrecedente; }
float Agriculteur::getNote() { return noteQualiteMoyenne; }
float Agriculteur::calculRisque()
{
    float score = calculScore();
    float risque = (10.0f - score) * 6.0f;
    if(qtOlives > 5000)
        risque *= 0.6f;
    if(productiviteAnormale(qtOlivesAnneePrecedente))
        risque += 20;

    return qBound(0.0f, risque, 100.0f);
}
QString Agriculteur::recommandation()
{
    if(calculRisque() > 70)
        return "⚠️ Irrigation + contrôle urgent";

    if(calculRisque() > 40)
        return "🟡 Surveillance régulière";

    return "🟢 Situation stable";
}
QString Agriculteur::detectionIntelligente(float moyenneHistorique)
{
    int risque = 0;

    if(qtOlives < qtOlivesAnneePrecedente * 0.7)
        risque += 30;

    if(noteQualiteMoyenne < 5)
        risque += 25;

    if(rendementMoyen < 50)
        risque += 20;

    if(productiviteAnormale(moyenneHistorique))
        risque += 25;

    // 🔥 NORMALISATION
    risque = qBound(0, risque, 100);

    if(risque >= 70)
        return "🔴 CLIENT CRITIQUE";

    if(risque >= 40)
        return "🟡 CLIENT POTENTIEL";

    return "🟢 CLIENT STRATÉGIQUE";
}
float Agriculteur::calculNoteGlobale(float moyenneHistorique)
{

    float qualite = noteQualiteMoyenne;
    float rendement10 = qMin(rendementMoyen / 10.0f, 10.0f);
    float croissance = 0;
    if(moyenneHistorique > 0)
        croissance = (qtOlives - moyenneHistorique) / moyenneHistorique;

    float croissanceScore = 5 + (croissance * 5);
    if(croissanceScore > 10) croissanceScore = 10;
    if(croissanceScore < 0) croissanceScore = 0;
    float stabilite = 8.0;
    float score =
        0.35 * qualite +
        0.25 * rendement10 +
        0.25 * croissanceScore +
        0.15 * stabilite;

    if(score > 10) score = 10;
    if(score < 0) score = 0;

    return score;
}
bool Agriculteur::estEnRisque()
{
    if(calculScore() > 7.5)
        return "🟢 PRIORITÉ PREMIUM";

    if(calculScore() > 5)
        return "🟡 CLIENT À DÉVELOPPER";

    return "🔴 FAIBLE POTENTIEL";
}