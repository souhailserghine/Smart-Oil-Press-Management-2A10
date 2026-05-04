#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "agriculteur.h"
#include <QVector>
#include <QStack>
#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

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
    void on_btn_retour_prediction_clicked();
    void afficherCourbe(int idAgri, QString type);
    void on_btnRech_clicked();
    void afficherTableauDetection();
    void on_btnAjouterHistorique_clicked();
    void afficherPrediction(int id);
    void on_retourdet_clicked();
    void on_retour_clicked();
    void on_retourstat_clicked();
    void on_quitter_clicked();
    void on_btnok_clicked();
    void on_btnDetection_clicked();
    void on_btn_retour_clicked();


};
#endif // MAINWINDOW_H
