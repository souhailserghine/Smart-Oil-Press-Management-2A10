#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QVariantList>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#include <QtCharts/QChartView>

class QComboBox;
class QTableWidget;
class QPushButton;
class QLineEdit;
class QDateEdit;
class QWidget;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // ===== Module 5 navigation (objectNames in UI) =====
    void on_btnConsulterMachines_clicked();
    void on_btnAjouterMachines_clicked();
    void on_btnStatMachines_clicked();
    void on_btnAvanceMachines_clicked();

    // ===== Module 5 actions (objectNames in UI) =====
    void on_ajoutermachine_clicked();
    void on_rechrchemahine_clicked();
    void on_filtrer_clicked();
    void on_oktrie_clicked();
    void on_exportmachine_clicked();
    void on_exportermachinne_clicked();

    // Ajout série machine (bouton موجود في UI: ajouterlineseriemachine_2)
    void on_ajouterlineseriemachine_2_clicked();

private:
    Ui::MainWindow *ui;

    // runtime combo added to machine form
    QComboBox* cbSerieMachine = nullptr; // objectName: cbSerieMachine

    // edit mode
    int editMachineId = -1;
    bool busyMachine = false;
    bool busySerie   = false;
    QString orderByMachine = "m.id_machine";

    // Série UI created runtime in metieravancee_4
    QLineEdit* serieNom = nullptr;
    QLineEdit* serieCap = nullptr;
    QDateEdit* serieDate = nullptr;
    QComboBox* serieEtat = nullptr;
    QLineEdit* serieResp = nullptr;
    QLineEdit* serieDesc = nullptr;
    QPushButton* btnAddSerie = nullptr;

    // advanced analytics UI (metieravancee_4)
    QLabel* advTotalValue = nullptr;
    QLabel* advActiveValue = nullptr;
    QLabel* advCriticalValue = nullptr;
    QLabel* advAvgPerfValue = nullptr;
    QTableWidget* advMachineTable = nullptr;
    QPushButton* btnRefreshAdvanced = nullptr;
    QPushButton* btnExportAdvanced = nullptr;

    // chart view on Stat page
    QChartView* chartStatusView = nullptr;

private:
    // setup
    void setupModule5();
    void ensureSerieComboInMachineForm();
    void ensureSerieUiInAdvanced();
    void ensureMachineTopBarVisible();
    void ensureMachineTableColumns();
    void refreshAdvancedAnalytics();

    // DB helpers
    bool dbOpen() const;
    int nextId(const QString& table, const QString& col) const;
    QString mapMachineEtatToDb(const QString& uiText) const;

    // find helpers (robust against UI name changes)
    QDateEdit* findMachineDateEdit() const;
    QLineEdit* findMachineNameEdit() const;
    QComboBox* findMachineTypeCombo() const;
    QComboBox* findMachineEtatCombo() const;

    // load/refresh
    void fillSeriesCombo();
    void loadMachines(const QString& whereSql = QString(), const QVariantList& binds = {});
    void addActionsToMachineRow(int row, int idMachine);
    void updateMachineCharts();

    // machine CRUD
    void saveMachineFromForm();             // insert/update
    void handleEditMachine(int idMachine);  // fill form
    void handleDeleteMachine(int idMachine);

    // series
    void addSerieFromForm();
    void addSerieFromAddPage();

    // validators / UI fixes
    void setupValidatorsModule5();

    // export
    void exportTableToCsv(QTableWidget* t, const QString& defaultName) const;
};

#endif // MAINWINDOW_H
