#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QHeaderView>

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
    // Navigation
    void on_btnConsulterstc_clicked();
    void on_btnAjouterstc_clicked();
    void on_btnStatstc_clicked();
    // BUG FIX #2 : le bouton s'appelle btnmetieravance dans le .ui,
    // pas toolButton_5. Slot renommé pour que l'auto-connexion fonctionne.
    void on_btnmetieravance_clicked();

    // Ajouter un lot
    void on_ajouterqtoliveBtn_clicked();

    // Consulter
    // BUG FIX #2 : boutons renommés selon les noms réels dans mainwindow.ui
    void on_modifierButton_clicked();   // Rafraîchir (était on_toolButton_2_clicked)
    void on_RechercheButton_clicked();  // Rechercher  (était on_toolButton_clicked)
    void on_pushButton_clicked();       // Exporter PDF

private:
    Ui::MainWindow *ui;

    void chargerListeOlives();
};

#endif // MAINWINDOW_H