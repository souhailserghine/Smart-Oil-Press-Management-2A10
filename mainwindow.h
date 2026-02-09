#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QTableWidget>

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
    void on_btnAjouterEmp_clicked();
    void on_btnConsulterEmp_clicked();
    void on_btnStatEmp_clicked();
    void on_btnAdvEmp_clicked();
    void on_faceBtn_clicked();

    // Sidebar module navigation
    void on_btnmod1_clicked(); // Personnel (module1)
    void on_btnmod2_clicked(); // Module 2
    void on_btnmod3_clicked(); // Module 3
    void on_btnmod4_clicked(); // Module 4
    void on_btnmod5_clicked(); // Module 5
    void on_btnmod6_clicked(); // Module 6

    // Module 2 (Stocks) toolbar actions
    void on_btnConsulterstc_clicked();
    void on_btnAjouterstc_clicked();
    void on_btnStatstc_clicked();
    void on_toolButton_5_clicked();

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

private:
    Ui::MainWindow *ui;
    
    // Helper methods
    void setupPersonnelChart();
    void setupPersonnelTable();
    void setupCiterneChart();
    void setupStocksChart();
    void setupQualiteChart();
    void addActionButtonsToRow(QTableWidget* table, int row);
    void addActionsColumnTo(QTableWidget* table);
    int findRowForButton(QTableWidget* table, QObject* button) const;
    QTableWidget* findOwningTable(QObject* child) const;
    void setupActionsForAllTables();
    void setActiveModuleButton(int index);

    // UX enhancements
    void crossFadeToIndex(QStackedWidget* stack, int newIndex);
    void animateSidebarToggle(bool collapse);
    void setupInteractiveHooks();
    void filterPersonnelTable();

    // Top-right user info positioning
    void repositionUserInfo();
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

    bool m_sidebarCollapsed = false;
};
#endif // MAINWINDOW_H
