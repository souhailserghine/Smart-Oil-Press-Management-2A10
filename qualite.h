#ifndef QUALITE_H
#define QUALITE_H

#include <QMainWindow>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDate>
#include <QDebug>
#include <QHeaderView>  // ← AJOUTER CETTE LIGNE

namespace Ui {
class qualite;
}

class qualite : public QMainWindow
{
    Q_OBJECT

public:
    explicit qualite(QWidget *parent = nullptr);
    ~qualite();

private slots:
    void on_ajouterHuileBtn_clicked();
    void on_btnAjouterHuile_clicked();
    void on_btnConsulterHuile_clicked();
    void on_btnAjouterHuile_triggered(QAction *arg1);
    void on_trihuileButton_clicked();

private:
    Ui::qualite *ui;
    void afficherHuile();
    void clearFields();
    void ajusterLargeurColonnes();  // ← AJOUTER CETTE LIGNE (DÉCLARATION)
};

#endif // QUALITE_H
