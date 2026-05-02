#ifndef STOCKS_H
#define STOCKS_H

#include <QLabel>
#include <QMainWindow>
#include <QMap>

class QToolBar;

QT_BEGIN_NAMESPACE
namespace Ui
{
class Stocks;
}
QT_END_NAMESPACE

class Stocks : public QMainWindow
{
    Q_OBJECT

public:
    Stocks(QWidget *parent = nullptr);
    ~Stocks();
    void navigateToSection(int section);


private slots:
    // Navigation
    void on_AjoutStock_clicked();
    void on_btnConsulterstc_clicked();
    void on_StatistiqueStock_clicked();

    // CRUD Operations
    void on_ConAjout_clicked();
    void onModifier(const QString &id);
    void onSupprimer(const QString &id);

    // Search and Sort
    void on_Recherche_clicked();
    void on_Trier_currentIndexChanged(int index);

    // Export
    void on_exporterListeStock_clicked();

    // Statistics
    void on_X_currentIndexChanged(int index);

    // Métiers avancés
    void on_PredictOutput_clicked();
    void on_OptimiserReappro_clicked();
    void on_PredictDechet_clicked();
    void on_GestionQualite_clicked();

private:
    Ui::Stocks *ui;

    // Core functions
    void setupUnifiedToolbar();
    void applyUnifiedVisualStyle();

    void chargerListeOlives();
    void rafraichirListe();

    // Statistics functions
    void afficherStatistiques();
    QLabel *creerStatistiquesCategorie(const QList<QString> &couleurs);
    QLabel *creerStatistiquesQuantite(const QList<QString> &couleurs);
    QLabel *creerStatistiquesQualite(const QList<QString> &couleurs);
    QLabel *creerStatistiquesTemporal(const QList<QString> &couleurs);

    // Métiers avancés
    QLabel *creerPredictionOutput();
    QLabel *creerOptimisationReappro();
    QLabel *creerPredictionDechet();
    QLabel *creerGestionQualite();

    // Utility functions
    void remplacerContenuAvecAnimation(QLabel *nouveauGraphique);
    void afficherMessageErreur(const QString &message);
    void afficherMessageInfo(const QString &message);
    QLabel *creerLabelGraphique(const QString &html);
    QString debutGraphique(const QString &titre, const QString &type);
    QString finGraphique();

    // Data validation
    bool validerDonneesAjout();
    bool validerQuantite(double quantite);
};

#endif // STOCKS_H