#ifndef CITERNE_H
#define CITERNE_H

#include <QMainWindow>
#include <QLabel>
#include <QStringList>
#include <QWidget>
#include <QList>
#include <QTimer>
#include <QDateTime>
#include <QMap>
#include <QProgressBar>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>

QT_BEGIN_NAMESPACE
namespace Ui { class Citernes; }
QT_END_NAMESPACE

struct CiterneData {
    int     id;
    double  capaciteMax;
    double  niveauActuel;
    QString typeHuile;
    double  temperature;
    QString etat;
};

class Citernes : public QMainWindow
{
    Q_OBJECT

public:
    explicit Citernes(QWidget *parent = nullptr);
    ~Citernes();

    QLabel* creerGraphiqueBarres(const QStringList &labels,
                                 const QList<double> &valeurs,
                                 const QString &titre,
                                 const QString &couleur,
                                 double maxValeur);
    QLabel* creerGraphiqueCirculaire(const QStringList &labels,
                                     const QList<double> &valeurs,
                                     const QString &titre);

private slots:
    void chargerListeCiternes();
    void rafraichirListe();
    void onModifier(int row);
    void onSupprimer(int row);
    void on_ConAjout_clicked();
    void on_AjoutCiterne_clicked();
    void on_ConsulterCiterne_clicked();
    void on_Recherche_clicked();
    void on_exporterListeCiterne_clicked();
    void on_StatistiqueCiterne_clicked();
    void on_X_currentIndexChanged(int index);
    void afficherStatistiques();
    void on_MetierAvanceCiterne_clicked();

    // Nouveaux métiers avancés
    void on_mesure_clicked();
    void on_remplissageCi_clicked();

private:
    Ui::Citernes *ui;

    QLabel* m_errCapa  = nullptr;
    QLabel* m_errNiv   = nullptr;
    QLabel* m_errEtat  = nullptr;

    QChartView* m_currentChart = nullptr;
    QTimer* m_timerMesures = nullptr;

    QList<CiterneData> lireDonneesCiternes();
    QChart*            makeChart(const QString &title);
    QChartView*        makeChartView(QChart *chart, int minHeight = 340);
    void               afficherGraphique(QChartView *view);

    QString debutGraphique(const QString &titre, const QString &type);
    QString finGraphique();
    QLabel* creerLabelGraphique(const QString &html);
    void    remplacerContenuAvecAnimation(QLabel *nouveauGraphique);
    void    afficherMessageErreur(const QString &message);
    void    afficherMessageInfo(const QString &message);
    QLabel* creerStatistiquesHuile(const QList<QString> &couleurs);
    QLabel* creerStatistiquesTypeHuile(const QList<QString> &couleurs);
    QLabel* creerStatistiquesEtat(const QList<QString> &couleurs);
    QLabel* creerStatistiquesCapacite(const QList<QString> &couleurs);
    QLabel* creerStatistiquesNiveau(const QList<QString> &couleurs);
    QLabel* creerStatistiquesTemperature(const QList<QString> &couleurs);
    QLabel* creerStatistiquesComparatives(const QList<QString> &couleurs);

    // Nouvelles méthodes pour les métiers avancés
    void creerTableMesure();
    void insererMesure(int idCiterne, double niveau, double temperature);
    void enregistrementPeriodiqueMesures();

    // Nouvelles méthodes pour les alertes avancées
    struct MesurePrecedente {
        int idCiterne;
        double niveau;
        double temperature;
        QDateTime date;
        bool existe;
    };

    QMap<int, MesurePrecedente> m_dernieresMesures;

    void verifierSeuilsEtAlerter(int idCiterne, double niveau, double temperature, const QString& typeHuile);
    void verifierFuite(int idCiterne, double niveauActuel, const QDateTime& date);
    void verifierTemperatureAnormale(int idCiterne, double temperature, const QString& typeHuile);
    void verifierNiveauCritique(int idCiterne, double niveau, double capaciteMax);

    // Structure pour le remplissage
    struct ResultatRemplissage {
        int idCiterne;
        double quantiteAttribuee;
        double capaciteMax;
    };
    QList<ResultatRemplissage> repartirQuantiteDansCiternesVides(const QString& typeHuile, double quantiteDemandee);
};

#endif // CITERNE_H
