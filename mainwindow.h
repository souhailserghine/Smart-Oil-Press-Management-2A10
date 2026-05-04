#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QTimer>       // ← ajout pour la connexion différée non-bloquante
#include "arduino.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
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

    void on_btnmod1_clicked();
    void on_btnmod2_clicked();
    void on_btnmod3_clicked();
    void on_btnmod4_clicked();
    void on_btnmod5_clicked();
    void on_btnmod6_clicked();

    // ← Slot déclenché après le délai de stabilisation Arduino
    void finishArduinoConnect();

private:
    Ui::MainWindow *ui;

    Arduino* m_arduino          = nullptr;
    bool     m_sidebarCollapsed = false;

    void setupPersonnelChart();
    void setupPersonnelTable();
    void addActionButtonsToRow(int row);
    int  findRowForButton(QObject* button) const;
    void setActiveModuleButton(int index);
    void crossFadeToIndex(QStackedWidget* stack, int newIndex);
    void animateSidebarToggle(bool collapse);
    void setupInteractiveHooks();
    void filterPersonnelTable();

    // ← Lance la détection Arduino sans bloquer l'UI
    void connectArduinoAsync();
};

#endif // MAINWINDOW_H
