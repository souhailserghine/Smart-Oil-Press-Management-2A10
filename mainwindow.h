#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "agriculteur.h"
#include <QVector>
#include <QStack>
#include <QMainWindow>

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
    int idSelectionne = -1;
    int idSelectionneHistorique = -1;
    QStack<int> historiquePages;

private:
    Ui::MainWindow *ui;

private slots:
    void on_ajouterEmpBtn_clicked();
    void afficherTableau();
    void afficherTableauAvecQuery(QString queryStr);
    void on_btn_valider_modif_clicked();
    void on_btnTrier_clicked();
    void afficherHistorique(int idAgri);
    void on_retour_2_clicked();
    void on_modifier_2_clicked();
    void on_btnAjouterHistorique_clicked();
    void on_retour_clicked();
    void on_quitter_clicked();
    void on_btn_retour_clicked();


};
#endif // MAINWINDOW_H
