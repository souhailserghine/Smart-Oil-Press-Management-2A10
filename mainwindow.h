#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>

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

private:
    Ui::MainWindow *ui;
    
    // Helper methods
    void setupPersonnelChart();
    void setupPersonnelTable();
    void addActionButtonsToRow(int row);
    int findRowForButton(QObject* button) const;
    void setActiveModuleButton(int index);

    // UX enhancements
    void crossFadeToIndex(QStackedWidget* stack, int newIndex);
    void animateSidebarToggle(bool collapse);
    void setupInteractiveHooks();
    void filterPersonnelTable();

    bool m_sidebarCollapsed = false;
};
#endif // MAINWINDOW_H
