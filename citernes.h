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
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QPushButton>
#include <QComboBox>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>

#include "arduino.h"

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
    void on_acheter_clicked();
    void on_remplissageCi_clicked();

    void onDonneesArduinoRecues();
    void on_btnConnecterArduino_clicked();
    void finishArduinoConnect();

private:
    Ui::Citernes *ui;

    QPushButton* m_btnArduino  = nullptr;
    QComboBox*   m_comboPorts  = nullptr;
    QLabel*      m_labelStatut = nullptr;

    QLabel* m_errCapa = nullptr;
    QLabel* m_errNiv  = nullptr;
    QLabel* m_errEtat = nullptr;

    QChartView* m_currentChart = nullptr;
    QList<CiterneData> lireDonneesCiternes();
    QChart*     makeChart(const QString &title);
    QChartView* makeChartView(QChart *chart, int minHeight = 340);
    void        afficherGraphique(QChartView *view);

    struct BesoinAchat {
        int       id;
        QString   typeHuile;
        double    quantiteManquante;
        double    capaciteSuggeree;
        int       nombreCiternesRequises;
        QString   statut;
        QDateTime dateAjout;
    };
    QList<BesoinAchat> m_listeBesoins;

    void afficherListeAchats();
    void exporterListeAchatsPDF();
    void ajouterBesoinAchat(const QString& typeHuile, double quantiteManquante,
                            double capaciteSuggeree, int nombreRequis);
    void supprimerBesoinAchat(int id);
    void mettreAJourStatutBesoin(int id, const QString& nouveauStatut);
    void chargerBesoinsAchat();
    void sauvegarderBesoinAchat(int id, const QString& typeHuile, double quantiteManquante,
                                double capaciteSuggeree, int nombreRequis,
                                const QString& statut, const QDateTime& date);
    void supprimerBesoinAchatBD(int id);
    void mettreAJourStatutBesoinBD(int id, const QString& nouveauStatut);

    struct ResultatRemplissage {
        int    idCiterne;
        double quantiteAttribuee;
        double capaciteMax;
    };
    QList<ResultatRemplissage> repartirQuantiteDansCiternesVides(
        const QString &typeHuile, double quantiteDemandee);

    // ── Arduino ──────────────────────────────────────────────────────────
    Arduino* m_arduino         = nullptr;
    QTimer*  m_timerNiveaux    = nullptr;
    bool     m_arduinoConnecte = false;

    int    m_idCiterneArduino   = 0;
    double m_temperatureArduino = 20.0;

    // front montant : insertion BDD uniquement au passage sec → eau
    bool m_capteurActifPrecedent = false;

    // anti-spam alerte : true = alerte déjà affichée pour ce débordement
    // remis à false dès que le niveau repasse sous 90%
    bool m_alerteDebordementActive = false;

    void envoyerCommandeLED(bool allumer);
    void rafraichirListePorts();

    void creerSequenceMesure();
    void sauvegarderMesure(int idCiterne, double niveauActuel, double temperature);
    int  getIdCiterneArduino();
};

#endif // CITERNE_H
