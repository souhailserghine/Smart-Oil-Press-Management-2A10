#ifndef STOCKS_H
#define STOCKS_H

#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QDialog>
#include <QScrollArea>
#include <QWidget>

class QToolBar;
class QComboBox;

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
    void on_btnMetiersAvances_clicked();

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

    // Métiers avancés — chaque bouton ouvre une popup IA
    void on_PredictOutput_clicked();
    void on_OptimiserReappro_clicked();
    void on_PredictDechet_clicked();
    void on_GestionQualite_clicked();

private:
    Ui::Stocks *ui;

    // Zone scrollable dédiée à l'affichage des graphiques dans statStock
    QScrollArea *m_graphiqueScroll = nullptr;

    // Core functions
    void setupUnifiedToolbar();
    void applyUnifiedVisualStyle();

    void chargerListeOlives();
    void rafraichirListe();

    // Statistics functions
    void afficherStatistiques();

    // Ces fonctions retournent un QWidget* (pas QLabel*)
    // pour pouvoir utiliser QPainter et des layouts Qt natifs
    QWidget *creerStatistiquesCategorie(const QList<QString> &couleurs);
    QWidget *creerStatistiquesQuantite(const QList<QString> &couleurs);
    QWidget *creerStatistiquesQualite(const QList<QString> &couleurs);
    QWidget *creerStatistiquesTemporal(const QList<QString> &couleurs);

    // Métiers avancés — fonctions qui construisent le HTML pour les popups
    QString buildPredictionOutputHtml();
    QString buildOptimisationReapproHtml();
    QString buildPredictionDechetHtml();
    QString buildGestionQualiteHtml();

    // Helper : crée et affiche une popup IA standardisée
    void afficherPopupIA(const QString &titre,
                         const QString &icone,
                         const QString &couleurGradient1,
                         const QString &couleurGradient2,
                         const QString &htmlContenu);

    // Métiers avancés (anciens — gardés pour compatibilité interne)
    QLabel *creerPredictionOutput();
    QLabel *creerOptimisationReappro();
    QLabel *creerPredictionDechet();
    QLabel *creerGestionQualite();

    // Utility functions
    void remplacerContenuAvecAnimation(QWidget *nouveauWidget);
    void afficherMessageErreur(const QString &message);
    void afficherMessageInfo(const QString &message);
    QLabel *creerLabelGraphique(const QString &html);
    QString debutGraphique(const QString &titre, const QString &type);
    QString finGraphique();


    // Auto-affectation / machine-series integration
    // Added to the stock module itself so adding from stocks.ui triggers the same
    // affectation behavior as the integrated MainWindow stock page.
    void ensureStockSerieSelector();
    void refreshStockSerieChoices();
    void loadAffectationSettings();
    bool tryAutoAssignForSerie(int serieId, QString& detailMessage);
    bool tableColumnExists(const QString& tableName, const QString& columnName) const;

    // Data validation
    bool validerDonneesAjout();
    bool validerQuantite(double quantite);

    QComboBox* m_stockSerieCombo = nullptr;
    int m_maxAffectationsPerEmployee = 3;
    bool m_autoAssignFromStock = false;
};

#endif // STOCKS_H