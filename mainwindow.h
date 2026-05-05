#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>                        // ← AJOUTER CETTE LIGNE
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QHorizontalBarSeries>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QSqlRecord>
#include <QColor>
#include <QByteArray>
#include <QMap>
#include <QSerialPort>
#include <QSerialPortInfo>

#ifdef OPENCV_AVAILABLE
#include <opencv2/objdetect.hpp>
#endif

#include "arduino.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QComboBox;
class QTableWidget;
class QPushButton;
class QLineEdit;
class QDateEdit;
class QWidget;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QSqlQuery;
class QToolButton;
class QResizeEvent;
class Citernes;
class Stocks;
class HoverShadowFilter;
class FaceRecognitionService;
class FingerprintService;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    bool eventFilter(QObject* obj, QEvent* event) override;
    ~MainWindow();

private slots:
    void on_loginbtn_clicked();
    void on_btnAjouterEmp_clicked();   // toolbar button → navigate to form page
    void on_ajouterEmpBtn_clicked();   // form submit button → INSERT employee
    void on_parcourirPhotoBtn_clicked(); // photo browse button
    void on_captureFaceBtn_clicked();    // webcam capture for face model
    void on_btnConsulterEmp_clicked();
    void on_btnStatEmp_clicked();
    void on_btnAdvEmp_clicked();
    void on_faceBtn_clicked();
    void on_ajouterHuileBtn_clicked();
    void sauvegarderHuile();
    void on_btnAjouterQualite_clicked();
    void on_btnConsulterQualite_clicked();
    void on_amertureLineEdit_textChanged(const QString &text);
    void addHuileActionButtonsToRow(int row);
    int findRowForHuileButton(QObject* button) const;
    void on_pushButton_3_clicked();
    void on_recherchelot_textChanged(const QString &text);
    void on_comboBoxrecherche_currentTextChanged(const QString &text);
    void on_btnStatHuile_clicked();
    void on_trihuileButton_clicked();
    void on_reloadhuilebutton_clicked();
    void on_btnAdvHuile_clicked();

    // ── Affectation slots ────────────────────────────────────────────────────
    void on_affNewBtn_clicked();        // show the affectation form (affStack → 0)
    void on_affSaveBtn_clicked();       // INSERT affectation into EMP_MACH
    void on_affCancelBtn_clicked();     // back to table (affStack → 1)
    void on_affRefreshBtn_clicked();    // reload affectation table
    void on_affSearchEdit_textChanged(const QString& text);
    void on_settingsSaveBtn_clicked(); // save affectation settings from Paramètres

    // Sidebar module navigation
    void on_btnmod1_clicked();
    void on_btnmod2_clicked();
    void on_btnmod3_clicked();
    void on_btnmod4_clicked();
    void on_btnmod5_clicked();
    void on_btnmod6_clicked();

    // Stocks integration
    void on_btnAjouterstc_clicked();
    void on_btnConsulterstc_clicked();
    void on_btnStatstc_clicked();
    void on_toolButton_5_clicked();
    void on_ajouterqtoliveBtn_clicked();

// ===== Gestion des machines =====
void on_btnConsulterMachines_clicked();
void on_btnAjouterMachines_clicked();
void on_btnStatMachines_clicked();
void on_btnAvanceMachines_clicked();
void on_ajoutermachine_clicked();
void on_rechrchemahine_clicked();
void on_filtrer_clicked();
void on_oktrie_clicked();
void on_exportmachine_clicked();
void on_exportermachinne_clicked();
void on_ajouterlineseriemachine_2_clicked();
    void showMachineSortMenu(int logicalIndex);
    void refreshMachineSerialPorts();
    void connectMachineSensor();
    void disconnectMachineSensor();
    void readMachineSerialData();

    // Module 6 (Agriculteurs) toolbar actions
    void on_btnConsulterAgr_clicked();
    void on_btnAjouterAgr_clicked();
    void on_btnStatAgr_clicked();
    void on_btnAvanceAgr_clicked();
    void onFingerprintTerminalReadyRead();
    void onEnrollmentResult(bool success, int fingerprintId, const QString& reason);
    void onFingerprintDeletionResult(int fingerprintId, bool success);
    void onFingerprintError(const QString& message);
    void onFingerprintScanningStateChanged(bool scanning);
    void onFingerprintServiceReady();


private:
    Ui::MainWindow *ui;

    // Helper methods
    void setupPersonnelChart();
    void setupPersonnelTable();
    void loadEmployeeTable();
    void loadEmployeeStats();
    void loadFaceEmbeddings();
    void addActionButtonsToRow(int row);
    int findRowForButton(QObject* button) const;
    void setActiveModuleButton(int index);
    int moduleIndex(QWidget* moduleWidget) const;
    void ensureModuleIndex(int moduleIndex);
    void openModulePage(QStackedWidget* modulePages, int moduleIndex, int pageIndex);
    void openSidebarModule(int moduleIndex, QStackedWidget* modulePages, int pageIndex,
                           int activeButtonIndex, bool refreshStockChoices = false);
    int m_editingIdLot = -1;

    // Statistiques et métier avancé Huile
    void setupStatHuilePage();
    void chargerStatistiquesHuile();
    void setupAdvancedHuilePage();
    void chargerAnalyseAvanceeHuile();
    double computeHuileQualityScore(double ph, double acidite) const;
    QString huileScoreLabel(double score) const;
    QColor huileScoreColor(double score) const;
    void applyHuileScoreColorsRow(int row, double score);
    QString huileForecastTrendLabel(double currentScore, double futureScore) const;
    void applyHuileForecastColorsRow(int row, double score);
    QWidget* createStyledCard(const QString& title, const QString& accentColor);

    // Membres pour les graphiques et KPI huile
    QChartView* m_pieChartView = nullptr;
    QChartView* m_barChartView = nullptr;
    QChartView* m_lineChartView = nullptr;
    QLabel* m_huileTotalLotsValue = nullptr;
    QLabel* m_huileAcceptedValue = nullptr;
    QLabel* m_huileEnCoursValue = nullptr;
    QLabel* m_huileRejectedValue = nullptr;
    QLabel* m_huileTotalQuantiteValue = nullptr;
    QLabel* m_huileConformiteValue = nullptr;
    QLabel* m_huileAvgPhValue = nullptr;
    QLabel* m_huileAvgAciditeValue = nullptr;
    QLabel* m_huileAvgTempValue = nullptr;
    QLabel* m_huileInsightLabel = nullptr;

    QLabel* m_huileScoreMoyenValue = nullptr;
    QLabel* m_huilePremiumLotsValue = nullptr;
    QLabel* m_huileSurveillanceLotsValue = nullptr;
    QLabel* m_huileCritiqueLotsValue = nullptr;
    QLabel* m_huileAdvancedInsightLabel = nullptr;
    QDoubleSpinBox* m_huilePhIdealMinSpin = nullptr;
    QDoubleSpinBox* m_huilePhIdealMaxSpin = nullptr;
    QDoubleSpinBox* m_huileAcidIdealMaxSpin = nullptr;
    QDoubleSpinBox* m_huilePhWeightSpin = nullptr;
    QDoubleSpinBox* m_huileAcidWeightSpin = nullptr;
    QDoubleSpinBox* m_huileBalanceBonusSpin = nullptr;
    QDoubleSpinBox* m_huileSynergyPenaltySpin = nullptr;
    QTableWidget* m_huileAdvancedTable = nullptr;
    QPushButton* m_huileAdvancedRefreshBtn = nullptr;
    QPushButton* m_huileAdvancedExportBtn = nullptr;
    QChartView* m_huileAdvancedClassChart = nullptr;
    QChartView* m_huileAdvancedTopChart = nullptr;
    QLabel* m_huileForecastScoreMoyenValue = nullptr;
    QLabel* m_huileForecastPremiumValue = nullptr;
    QLabel* m_huileForecastVigilanceValue = nullptr;
    QLabel* m_huileForecastCritiqueValue = nullptr;
    QLabel* m_huileForecastInsightLabel = nullptr;
    QDoubleSpinBox* m_huileForecastPhDriftSpin = nullptr;
    QDoubleSpinBox* m_huileForecastAcidDriftSpin = nullptr;
    QDoubleSpinBox* m_huileForecastOxidationSpin = nullptr;
    QDoubleSpinBox* m_huileForecastStabilityBonusSpin = nullptr;
    QPushButton* m_huileForecastRefreshBtn = nullptr;
    QPushButton* m_huileForecastExportBtn = nullptr;
    QChartView* m_huileForecastClassChart = nullptr;
    QChartView* m_huileForecastTrendChart = nullptr;
    QTableWidget* m_huileForecastTable = nullptr;

    // UX enhancements
    void crossFadeToIndex(QStackedWidget* stack, int newIndex);
    void animateSidebarToggle(bool collapse);
    void setupInteractiveHooks();
    void setupToolbarsTweaks();
    void filterPersonnelTable();
    void setupEmployeeFormValidation();
    bool validateEmployeeForm(bool showFeedbackText = true);
    void loadAffectationSettings();
    bool saveAffectationSettings();
    void setupSettingsAutoAssignOption();
    void setupAffectationStatusFilter();
    void setupAffectationOpenEndedOption();
    void refreshStockSerieChoices();
    bool tryAutoAssignForSerie(int serieId, QString& detailMessage);
    void loadStocksTable();
    void repositionUserInfo();
    void resizeEvent(QResizeEvent* event) override;
    void makeAvatarCircular();
    void updateLoggedInUserInfo(int empId, const QString& fallbackName = QString());
    void repositionChatLauncher();
    void updateClock();
    QByteArray encodeFaceFromFile(const QString& imagePath);
    void initFingerprintService();
    void onFingerprintMatched(int fingerprintId);
    void startFingerprintEnrollmentFromForm();
    void setFingerprintStatus(const QString& text, const QString& style = QString());

// ===== Module machines =====
QComboBox* cbSerieMachine = nullptr;
int editMachineId = -1;
bool busyMachine = false;
bool busySerie = false;
QString orderByMachine = "m.id_machine";
Qt::SortOrder machineSortOrder = Qt::AscendingOrder;
int machineSortColumn = 0;
QString lastMachineWhereSql;
QVariantList lastMachineBinds;

QLabel* advTotalValue = nullptr;
QLabel* advActiveValue = nullptr;
QLabel* advCriticalValue = nullptr;
QLabel* advAvgPerfValue = nullptr;
QTableWidget* advMachineTable = nullptr;
QPushButton* btnRefreshAdvanced = nullptr;
QPushButton* btnExportAdvanced = nullptr;
QPushButton* btnApplyRiskFormula = nullptr;
QPushButton* btnCorrectiveAdvanced = nullptr;
QDoubleSpinBox* advTempReferenceSpin = nullptr;
QDoubleSpinBox* advTempCoeffSpin = nullptr;
QDoubleSpinBox* advHoursCoeffSpin = nullptr;
QDoubleSpinBox* advAgeCoeffSpin = nullptr;
QSpinBox* advMaintenancePenaltySpin = nullptr;
QSpinBox* advPannePenaltySpin = nullptr;
QSpinBox* advOrangeMinSpin = nullptr;
QSpinBox* advRedMinSpin = nullptr;
QLabel* advFormulaSummaryLabel = nullptr;
QChartView* chartStatusView = nullptr;
QChartView* chartTypeView = nullptr;
QChartView* chartHoursView = nullptr;
QLabel* statMachineTotalValue = nullptr;
QLabel* statMachineActiveValue = nullptr;
QLabel* statMachineMaintenanceValue = nullptr;
QLabel* statMachinePanneValue = nullptr;
QLabel* statMachineInsightLabel = nullptr;
QComboBox* machineSensorTargetCombo = nullptr;
QComboBox* machineSensorPortCombo = nullptr;
QPushButton* btnRefreshMachinePorts = nullptr;
QPushButton* btnConnectMachineSensor = nullptr;
QPushButton* btnDisconnectMachineSensor = nullptr;
QLabel* machineSensorStatusLabel = nullptr;
QLabel* machineSensorLastValueLabel = nullptr;
QSerialPort* machineSerial = nullptr;
QByteArray machineSerialBuffer;
int linkedMachineId = -1;

void setupModule5();
void ensureSerieComboInMachineForm();
void ensureSerieUiInAdvanced();
void loadMachineSensorTargets();
QString inferSensorState(double temperature) const;
void updateMachineSensorUiState(const QString& statusText, const QString& color = QString("#556B2F"));
void applyTemperatureToMachine(int machineId, double temperature, const QString& state);
void ensureMachineTopBarVisible();
void ensureMachineTableColumns();
void setupMachineHeaderSorting();
QString machineColumnToSql(int logicalIndex) const;
QString machineColumnTitle(int logicalIndex) const;
void ensureMachineExtraFields();
void ensureMachineSearchModes();
void setupValidatorsModule5();
void refreshAdvancedAnalytics();
void showMachineCorrectiveDialog();
int correctiveDaysEstimate(const QString& type, double temp, double hours, int ageDays) const;
QString correctiveEquipmentSummary(const QString& type, double temp, double hours, int ageDays) const;
QString correctiveActionSummary(const QString& type, double temp, double hours, int ageDays) const;
int computeMachineRiskScore(const QString& etat, double temp, double hours, int ageDays) const;
int computeMachinePerformanceScore(int risk) const;
QString riskBandLabel(int risk) const;
void applyRiskColorsToAdvancedRow(int row, int risk);
bool dbOpen() const;
bool tableHasColumn(const QString& table, const QString& column) const;
bool machineSupportsSeries() const;
int nextId(const QString& table, const QString& col) const;
QString mapMachineEtatToDb(const QString& uiText) const;
QDateEdit* findMachineDateEdit() const;
QLineEdit* findMachineNameEdit() const;
QLineEdit* findMachineHoursEdit() const;
QLineEdit* findMachineTempEdit() const;
QComboBox* findMachineTypeCombo() const;
QComboBox* findMachineEtatCombo() const;
void fillSeriesCombo();
void loadMachines(const QString& whereSql = QString(), const QVariantList& binds = {});
void addActionsToMachineRow(int row, int idMachine);
void updateMachineCharts();
void saveMachineFromForm();
void handleEditMachine(int idMachine);
void handleDeleteMachine(int idMachine);
void addSerieFromAddPage();
void exportTableToCsv(QTableWidget* t, const QString& defaultName) const;
void exportMachineTableToPdf(QTableWidget* t, const QString& title, const QString& defaultName) const;
void openCiternesWindow(int pageIndex = -1);
void openStocksWindow(int pageIndex = -1);

    Citernes* m_citernesWindow = nullptr;
    Stocks* m_stocksWindow = nullptr;
    FaceRecognitionService* m_faceService = nullptr;
    FingerprintService* m_fingerprintService = nullptr;
    Arduino m_fingerprintTerminal;
    bool m_sidebarCollapsed = false;
    int  m_loggedInId = -1;
    QByteArray m_selectedPhoto;
    QByteArray m_capturedFaceBlob;
#ifdef OPENCV_AVAILABLE
    cv::Ptr<cv::FaceDetectorYN> m_faceDetector;
    cv::Ptr<cv::FaceRecognizerSF> m_faceRecognizer;
    QMap<int, cv::Mat> m_faceEmbeddings;
#endif
    int m_pendingFingerprintId = -1;
    bool m_fingerprintEnrollInProgress = false;
    // Composite PK tracking for EMP_MACH edit mode
    int  m_editingAffId = -1;          // id_affectation being edited (-1 = insert mode)
    int  m_editingAffIdEmp   = -1;     // id_emp being edited (-1 = insert mode)
    int  m_editingAffIdSerie = -1;     // id_serie being edited (-1 = insert mode)
    int  m_maxAffectationsPerEmployee = 3;
    bool m_autoAssignFromStock = false;
    QCheckBox* m_settingsAutoAssignCheck = nullptr;

    // ── Affectation helpers ──────────────────────────────────────────────────
    void loadAffectationTable();
    void populateAffCombos();
    void filterAffTable();
    void updateAffectationRemainingInfo();
    bool hasDuplicateAffectation(int empId, int serieId) const;
    void prepareInsertAffectationQuery(QSqlQuery& query,
                                       int empId,
                                       int serieId,
                                       const QString& poste,
                                       const QDate& dateDeb,
                                       const QVariant& dateFinValue) const;
    void resetAffectationEditState();
    QToolButton* m_chatLauncher = nullptr;
    // Bottom-center clock in status bar
    QLabel* m_clockLabel = nullptr;
    QWidget* m_clockLeftSpacer = nullptr;
    QWidget* m_clockRightSpacer = nullptr;
    class QTimer* m_clockTimer = nullptr;
    HoverShadowFilter* m_hoverShadowFilter = nullptr;
};

#endif // MAINWINDOW_H
