#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>                        // ← AJOUTER CETTE LIGNE
#include <QtCharts/QChartView>
#include <QtCharts/QChart>

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
    bool eventFilter(QObject* obj, QEvent* event) override;
    ~MainWindow();

private slots:
    void on_loginbtn_clicked();
    void on_btnAjouterEmp_clicked();
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

    // Sidebar module navigation
    void on_btnmod1_clicked();
    void on_btnmod2_clicked();
    void on_btnmod3_clicked();
    void on_btnmod4_clicked();
    void on_btnmod5_clicked();
    void on_btnmod6_clicked();

private:
    Ui::MainWindow *ui;

    // Helper methods
    void setupPersonnelChart();
    void setupPersonnelTable();
    void addActionButtonsToRow(int row);
    int findRowForButton(QObject* button) const;
    void setActiveModuleButton(int index);
    int m_editingIdLot = -1;

    // Statistiques Huile
    void setupStatHuilePage();
    void chargerStatistiquesHuile();
    QWidget* createStyledCard(const QString& title, const QString& accentColor);  // ← AJOUTER CETTE LIGNE

    // Membres pour les graphiques
    QChartView* m_pieChartView = nullptr;
    QChartView* m_barChartView = nullptr;
    QChartView* m_lineChartView = nullptr;

    // UX enhancements
    void crossFadeToIndex(QStackedWidget* stack, int newIndex);
    void animateSidebarToggle(bool collapse);
    void setupInteractiveHooks();
    void filterPersonnelTable();

    bool m_sidebarCollapsed = false;
};

#endif // MAINWINDOW_H
