#ifndef AGRICULTEURMODULE_H
#define AGRICULTEURMODULE_H

#include <QWidget>
#include <QStack>
#include <QString>

class QStackedWidget;
class QTableWidget;
class QLineEdit;
class QComboBox;
class QDateEdit;
class QLabel;
class QPushButton;
class QChartView;
class QResizeEvent;

class AgriculteurModule : public QWidget
{
    Q_OBJECT
public:
    explicit AgriculteurModule(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

public slots:
    void showAddPage();
    void showConsultPage();
    void showStatsPage();
    void showAdvancedPage();
    void refreshAll();

private slots:
    void addAgriculteur();
    void saveEdit();
    void addHistorique();
    void saveHistoriqueEdit();
    void searchAgriculteurs();
    void sortAgriculteurs();
    void renderSelectedStat();
    void showDetectionPage();
    void backToConsult();
    void backToHistory();

private:
    void buildUi();
    void buildAddPage();
    void buildConsultPage();
    void buildEditPage();
    void buildHistoryPage();
    void buildHistoryEditPage();
    void buildStatsPage();
    void buildDetectionPage();
    void buildPredictionPage();
    void buildAdvancedPage();
    void setupFloatingSubmitButton();
    void updateFloatingSubmitButton();

    void afficherTableau();
    void afficherTableauAvecQuery(const QString &queryStr);
    bool tableExists(const QString &tableName) const;
    void ensureAgriculteurSupportTables();
    QString buildAgriculteurListQuery(const QString &whereClause = QString(), const QString &orderClause = QString()) const;
    void afficherHistorique(int idAgri);
    void afficherCourbe(int idAgri, const QString &type);
    void afficherPrediction(int idAgri);
    void afficherTableauDetection();
    void clearAddErrors();
    bool validateAddForm();
    bool validateEditForm();
    void setTableBasics(QTableWidget *table);
    QPushButton* makeTableButton(const QString &text, const QString &type, QWidget *parent);

    QStackedWidget *m_stack = nullptr;
    int m_idSelectionne = -1;
    int m_idSelectionneHistorique = -1;
    QStack<int> m_historiquePages;

    // Add page
    QLineEdit *m_nom = nullptr;
    QLineEdit *m_prenom = nullptr;
    QLineEdit *m_numero = nullptr;
    QLineEdit *m_adresse = nullptr;
    QLineEdit *m_mail = nullptr;
    QLineEdit *m_region = nullptr;
    QWidget *m_addPage = nullptr;
    QPushButton *m_floatingSubmitButton = nullptr;
    QLabel *m_errorNom = nullptr;
    QLabel *m_errorPrenom = nullptr;
    QLabel *m_errorNumero = nullptr;
    QLabel *m_errorAdresse = nullptr;
    QLabel *m_errorMail = nullptr;
    QLabel *m_errorRegion = nullptr;

    // Consult page
    QTableWidget *m_tableau = nullptr;
    QLineEdit *m_rech = nullptr;
    QComboBox *m_comboRech = nullptr;
    QComboBox *m_comboTri = nullptr;
    QComboBox *m_comboOrdre = nullptr;

    // Edit page
    QLineEdit *m_mNom = nullptr;
    QLineEdit *m_mPrenom = nullptr;
    QLineEdit *m_mNumero = nullptr;
    QLineEdit *m_mAdresse = nullptr;
    QLineEdit *m_mNbArbre = nullptr;
    QLineEdit *m_mTypeOlive = nullptr;
    QLineEdit *m_mMail = nullptr;
    QLineEdit *m_mRegion = nullptr;
    QLabel *m_errorMNom = nullptr;
    QLabel *m_errorMPrenom = nullptr;
    QLabel *m_errorMNumero = nullptr;
    QLabel *m_errorMAdresse = nullptr;
    QLabel *m_errorMMail = nullptr;
    QLabel *m_errorMRegion = nullptr;

    // History page
    QTableWidget *m_tableHistorique = nullptr;
    QLabel *m_labelMoyenne = nullptr;
    QLabel *m_labelTotal = nullptr;
    QLabel *m_labelPerformance = nullptr;
    QLineEdit *m_anneeH = nullptr;
    QLineEdit *m_quantiteH = nullptr;
    QLineEdit *m_nbArbreH = nullptr;
    QLineEdit *m_typeH = nullptr;
    QLineEdit *m_noteH = nullptr;
    QDateEdit *m_dateRecolteH = nullptr;

    // History edit page
    QLineEdit *m_anneeH2 = nullptr;
    QLineEdit *m_quantiteH2 = nullptr;
    QLineEdit *m_nbArbreH2 = nullptr;
    QLineEdit *m_typeH2 = nullptr;
    QLineEdit *m_noteH2 = nullptr;

    // Stats / detection / prediction
    QComboBox *m_choixTri = nullptr;
    QChartView *m_chartView = nullptr;
    QTableWidget *m_tableDetection = nullptr;
    QChartView *m_chartPrediction = nullptr;
    QLabel *m_labelPrediction = nullptr;
};

#endif // AGRICULTEURMODULE_H
