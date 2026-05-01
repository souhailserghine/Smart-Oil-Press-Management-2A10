#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QMainWindow>
#include <QStackedWidget>
#include <QTableWidget>
#include <QToolButton>
#include <QPushButton>
#include <QByteArray>
#include <QMap>
#include "fingerprintservice.h"

class FaceRecognitionService;

class QLabel;
class QWidget;
class QTimer;
class QCheckBox;
class QComboBox;
class QSqlQuery;
class HoverShadowFilter; // Forward declaration for HoverShadowFilter

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
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
    void on_exportEmpBtn_clicked();     // export employee list → PDF or Excel

    // ── Affectation slots ────────────────────────────────────────────────────
    void on_affNewBtn_clicked();        // show the affectation form (affStack → 0)
    void on_affSaveBtn_clicked();       // INSERT affectation into EMP_MACH
    void on_affCancelBtn_clicked();     // back to table (affStack → 1)
    void on_affRefreshBtn_clicked();    // reload affectation table
    void on_affSearchEdit_textChanged(const QString& text);

    // Sidebar module navigation
    void on_btnmod1_clicked(); // Personnel (module1)
    void on_btnmod2_clicked(); // Module 2
    void on_btnmod3_clicked(); // Module 3
    void on_btnmod4_clicked(); // Module 4
    void on_btnmod5_clicked(); // Module 5
    void on_btnmod6_clicked(); // Module 6

    void on_btnSettings_clicked(); // Settings (module7)
    void on_settingsSaveBtn_clicked();

    // Module 2 (Stocks) toolbar actions
    void on_btnConsulterstc_clicked();
    void on_btnAjouterstc_clicked();
    void on_btnStatstc_clicked();
    void on_toolButton_5_clicked();
    void on_ajouterqtoliveBtn_clicked();

    // Module 3 (Citernes) toolbar actions
    void on_AjoutCiterne_clicked();
    void on_ConsulterCiterne_clicked();
    void on_StatistiqueCiterne_clicked();
    void on_MetierAvanceCiterne_clicked();

    // Module 4 (Qualité) toolbar actions
    void on_btnConsulterQualite_clicked();
    void on_btnAjouterQualite_clicked();
    void on_btnStatQualite_clicked();
    void on_btnAdvEmp_2_clicked();

    // Module 5 (Machines) toolbar actions
    void on_btnConsulterMachines_clicked();
    void on_btnAjouterMachines_clicked();
    void on_btnStatMachines_clicked();
    void on_btnAvanceMachines_clicked();

    // Module 6 (Agriculteurs) toolbar actions
    void on_btnConsulterAgr_clicked();
    void on_btnAjouterAgr_clicked();
    void on_btnStatAgr_clicked();
    void on_btnAvanceAgr_clicked();
    void onFingerprintTerminalReadyRead();


private:
    Ui::MainWindow *ui;
    
    // Helper methods
    void setupPersonnelChart();
    void setupPersonnelTable();
    void loadEmployeeTable();          // populate tableWidget from DB
    void loadEmployeeStats();          // populate stats charts from DB
    void exportEmployeesToPdf(const QString& filePath);
    void exportEmployeesToCsv(const QString& filePath);
    void setupCiterneChart();
    void setupStocksChart();
    void setupQualiteChart();
    void addActionButtonsToRow(QTableWidget* table, int row);
    void addActionsColumnTo(QTableWidget* table);
    int findRowForButton(QTableWidget* table, QObject* button) const;
    QTableWidget* findOwningTable(QObject* child) const;
    void setupActionsForAllTables();
    void setActiveModuleButton(int index);
    int moduleIndex(QWidget* moduleWidget) const;
    void ensureModuleIndex(int moduleIndex);
    void openModulePage(QStackedWidget* modulePages, int moduleIndex, int pageIndex);
    void openSidebarModule(int moduleIndex, QStackedWidget* modulePages, int pageIndex,
                           int activeButtonIndex, bool refreshStockChoices = false);

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

    // Top-right user info positioning
    void repositionUserInfo();
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

    // Avatar rendering
    void makeAvatarCircular();

    // Chat launcher positioning
    void repositionChatLauncher();

    // System clock update
    void updateClock();

    // ── Facial recognition (moved out to a service) ───────────────────────
    QByteArray encodeFaceFromFile(const QString& imagePath);

    // ── Fingerprint service initialization and slots ──────────────────────
    void initFingerprintService();
    void onFingerprintMatched(int fingerprintId);
    void onEnrollmentResult(bool success, int fingerprintId, const QString &reason);
    void onFingerprintDeletionResult(int fingerprintId, bool success);
    void onFingerprintError(const QString &message);
    void onFingerprintScanningStateChanged(bool scanning);
    void onFingerprintServiceReady();

    void startFingerprintEnrollmentFromForm();

    FaceRecognitionService* m_faceService = nullptr;
    FingerprintService *m_fingerprintService = nullptr;
    int m_pendingFingerprintId = -1;

private:
    // ── Fingerprint UI helpers ──────────────────────────────────────────────
    void setFingerprintStatus(const QString& text, const QString& style = QString());

    bool m_sidebarCollapsed = false;
    int  m_loggedInId = -1;            // id_emp of the currently authenticated user
    QByteArray m_selectedPhoto;
    QByteArray m_capturedFaceBlob;     // face embedding captured via webcam for new employee
    // Composite PK tracking for EMP_MACH edit mode
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
