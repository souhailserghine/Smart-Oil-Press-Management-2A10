#include "login.h"
#include "./ui_login.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QPixmap pix(":/images/logo.png");

    ui->label->setPixmap(
        pix.scaled(
            ui->label->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
            )
        );
}

MainWindow::~MainWindow()
{
    delete ui;
}
